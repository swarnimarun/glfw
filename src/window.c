//========================================================================
// GLFW - An OpenGL framework
// Platform:    Any
// API version: 3.0
// WWW:         http://www.glfw.org/
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>


//========================================================================
// Return the maxiumum of the specified values
//========================================================================

static int Max(int a, int b)
{
    return (a > b) ? a : b;
}


//========================================================================
// Close all GLFW windows with the closed flag set
//========================================================================

static void closeFlaggedWindows(void)
{
    _GLFWwindow* window;

    for (window = _glfwLibrary.windowListHead;  window; )
    {
        if (window->closeRequested && _glfwLibrary.windowCloseCallback)
            window->closeRequested = _glfwLibrary.windowCloseCallback(window);

        if (window->closeRequested)
        {
            _GLFWwindow* next = window->next;
            glfwCloseWindow(window);
            window = next;
        }
        else
            window = window->next;
    }
}


//========================================================================
// Clear scroll offsets for all windows
//========================================================================

static void clearScrollOffsets(void)
{
    _GLFWwindow* window;

    for (window = _glfwLibrary.windowListHead;  window;  window = window->next)
    {
        window->scrollX = 0;
        window->scrollY = 0;
    }
}


//========================================================================
// Clear all input state
//========================================================================

static void clearInputState(_GLFWwindow* window)
{
    int i;

    // Release all keyboard keys
    for (i = 0;  i <= GLFW_KEY_LAST;  i++)
        window->key[i] = GLFW_RELEASE;

    // Release all mouse buttons
    for (i = 0;  i <= GLFW_MOUSE_BUTTON_LAST;  i++)
        window->mouseButton[i] = GLFW_RELEASE;

    // Set mouse position to (0,0)
    window->mousePosX = 0;
    window->mousePosY = 0;

    // Set scroll offsets to (0,0)
    window->scrollX = 0;
    window->scrollY = 0;

    // The default is to use non sticky keys and mouse buttons
    window->stickyKeys = GL_FALSE;
    window->stickyMouseButtons = GL_FALSE;

    // The default is to disable key repeat
    window->keyRepeat = GL_FALSE;
}


//========================================================================
// Checks whether the OpenGL part of the window config is sane
//========================================================================

static GLboolean isValidContextConfig(_GLFWwndconfig* wndconfig)
{
    if (wndconfig->glMajor < 1 || wndconfig->glMinor < 0)
    {
        // OpenGL 1.0 is the smallest valid version
        _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Invalid OpenGL version requested");
        return GL_FALSE;
    }
    if (wndconfig->glMajor == 1 && wndconfig->glMinor > 5)
    {
        // OpenGL 1.x series ended with version 1.5
        _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Invalid OpenGL version requested");
        return GL_FALSE;
    }
    else if (wndconfig->glMajor == 2 && wndconfig->glMinor > 1)
    {
        // OpenGL 2.x series ended with version 2.1
        _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Invalid OpenGL version requested");
        return GL_FALSE;
    }
    else if (wndconfig->glMajor == 3 && wndconfig->glMinor > 3)
    {
        // OpenGL 3.x series ended with version 3.3
        _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Invalid OpenGL version requested");
        return GL_FALSE;
    }
    else
    {
        // For now, let everything else through
    }

    if (wndconfig->glProfile == GLFW_OPENGL_ES2_PROFILE)
    {
        if (wndconfig->glMajor != 2 || wndconfig->glMinor < 0)
        {
            // The OpenGL ES 2.0 profile is currently only defined for version
            // 2.0 (see {WGL|GLX}_EXT_create_context_es2_profile), but for
            // compatibility with future updates to OpenGL ES, we allow
            // everything 2.x and let the driver report invalid 2.x versions

            _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Invalid OpenGL ES 2.x version requested");
            return GL_FALSE;
        }
    }
    else if (wndconfig->glProfile)
    {
        if (wndconfig->glMajor < 3 || (wndconfig->glMajor == 3 && wndconfig->glMinor < 2))
        {
            // Desktop OpenGL context profiles are only defined for version 3.2
            // and above

            _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Context profiles only exist for OpenGL version 3.2 and above");
            return GL_FALSE;
        }
    }

    if (wndconfig->glForward && wndconfig->glMajor < 3)
    {
        // Forward-compatible contexts are only defined for OpenGL version 3.0 and above
        _glfwSetError(GLFW_INVALID_VALUE, "glfwOpenWindow: Forward compatibility only exist for OpenGL version 3.0 and above");
        return GL_FALSE;
    }

    return GL_TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Clear all open window hints
//========================================================================

void _glfwClearWindowHints(void)
{
    memset(&_glfwLibrary.hints, 0, sizeof(_glfwLibrary.hints));

    // The default OpenGL version is 1.0
    _glfwLibrary.hints.glMajor = 1;
    _glfwLibrary.hints.glMinor = 0;
}


//========================================================================
// Register keyboard activity
//========================================================================

void _glfwInputKey(_GLFWwindow* window, int key, int action)
{
    GLboolean keyrepeat = GL_FALSE;

    if (key < 0 || key > GLFW_KEY_LAST)
        return;

    // Are we trying to release an already released key?
    if (action == GLFW_RELEASE && window->key[key] != GLFW_PRESS)
        return;

    // Register key action
    if(action == GLFW_RELEASE && window->stickyKeys)
        window->key[key] = GLFW_STICK;
    else
    {
        keyrepeat = (window->key[key] == GLFW_PRESS) && (action == GLFW_PRESS);
        window->key[key] = (char) action;
    }

    // Call user callback function
    if (_glfwLibrary.keyCallback && (window->keyRepeat || !keyrepeat))
        _glfwLibrary.keyCallback(window, key, action);
}


//========================================================================
// Register (keyboard) character activity
//========================================================================

void _glfwInputChar(_GLFWwindow* window, int character)
{
    // Valid Unicode (ISO 10646) character?
    if (!((character >= 32 && character <= 126) || character >= 160))
        return;

    if (_glfwLibrary.charCallback)
        _glfwLibrary.charCallback(window, character);
}


//========================================================================
// Register scroll events
//========================================================================

void _glfwInputScroll(_GLFWwindow* window, int xoffset, int yoffset)
{
    window->scrollX += xoffset;
    window->scrollY += yoffset;

    if (_glfwLibrary.scrollCallback)
        _glfwLibrary.scrollCallback(window, xoffset, yoffset);
}


//========================================================================
// Register mouse button clicks
//========================================================================

void _glfwInputMouseClick(_GLFWwindow* window, int button, int action)
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return;

    // Register mouse button action
    if (action == GLFW_RELEASE && window->stickyMouseButtons)
        window->mouseButton[button] = GLFW_STICK;
    else
        window->mouseButton[button] = (char) action;

    if (_glfwLibrary.mouseButtonCallback)
        _glfwLibrary.mouseButtonCallback(window, button, action);
}


//========================================================================
// Register window focus events
//========================================================================

void _glfwInputWindowFocus(_GLFWwindow* window, GLboolean activated)
{
    if (activated)
    {
        if (_glfwLibrary.activeWindow != window)
        {
            _glfwLibrary.activeWindow = window;

            if (_glfwLibrary.windowFocusCallback)
                _glfwLibrary.windowFocusCallback(window, activated);
        }
    }
    else
    {
        if (_glfwLibrary.activeWindow == window)
        {
            int i;

            // Release all pressed keyboard keys
            for (i = 0;  i <= GLFW_KEY_LAST;  i++)
            {
                if (window->key[i] == GLFW_PRESS)
                    _glfwInputKey(window, i, GLFW_RELEASE);
            }

            // Release all pressed mouse buttons
            for (i = 0;  i <= GLFW_MOUSE_BUTTON_LAST;  i++)
            {
                if (window->mouseButton[i] == GLFW_PRESS)
                    _glfwInputMouseClick(window, i, GLFW_RELEASE);
            }

            _glfwLibrary.activeWindow = NULL;

            if (_glfwLibrary.windowFocusCallback)
                _glfwLibrary.windowFocusCallback(window, activated);
        }
    }
}


//========================================================================
// Return the available framebuffer config closest to the desired values
// This is based on the manual GLX Visual selection from 2.6
//========================================================================

const _GLFWfbconfig* _glfwChooseFBConfig(const _GLFWfbconfig* desired,
                                         const _GLFWfbconfig* alternatives,
                                         unsigned int count)
{
    unsigned int i;
    unsigned int missing, leastMissing = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    unsigned int extraDiff, leastExtraDiff = UINT_MAX;
    const _GLFWfbconfig* current;
    const _GLFWfbconfig* closest = NULL;

    for (i = 0;  i < count;  i++)
    {
        current = alternatives + i;

        if (desired->stereo > 0 && current->stereo == 0)
        {
            // Stereo is a hard constraint
            continue;
        }

        // Count number of missing buffers
        {
            missing = 0;

            if (desired->alphaBits > 0 && current->alphaBits == 0)
                missing++;

            if (desired->depthBits > 0 && current->depthBits == 0)
                missing++;

            if (desired->stencilBits > 0 && current->stencilBits == 0)
                missing++;

            if (desired->auxBuffers > 0 && current->auxBuffers < desired->auxBuffers)
                missing += desired->auxBuffers - current->auxBuffers;

            if (desired->samples > 0 && current->samples == 0)
            {
                // Technically, several multisampling buffers could be
                // involved, but that's a lower level implementation detail and
                // not important to us here, so we count them as one
                missing++;
            }
        }

        // These polynomials make many small channel size differences matter
        // less than one large channel size difference

        // Calculate color channel size difference value
        {
            colorDiff = 0;

            if (desired->redBits > 0)
            {
                colorDiff += (desired->redBits - current->redBits) *
                             (desired->redBits - current->redBits);
            }

            if (desired->greenBits > 0)
            {
                colorDiff += (desired->greenBits - current->greenBits) *
                             (desired->greenBits - current->greenBits);
            }

            if (desired->blueBits > 0)
            {
                colorDiff += (desired->blueBits - current->blueBits) *
                             (desired->blueBits - current->blueBits);
            }
        }

        // Calculate non-color channel size difference value
        {
            extraDiff = 0;

            if (desired->alphaBits > 0)
            {
                extraDiff += (desired->alphaBits - current->alphaBits) *
                             (desired->alphaBits - current->alphaBits);
            }

            if (desired->depthBits > 0)
            {
                extraDiff += (desired->depthBits - current->depthBits) *
                             (desired->depthBits - current->depthBits);
            }

            if (desired->stencilBits > 0)
            {
                extraDiff += (desired->stencilBits - current->stencilBits) *
                             (desired->stencilBits - current->stencilBits);
            }

            if (desired->accumRedBits > 0)
            {
                extraDiff += (desired->accumRedBits - current->accumRedBits) *
                             (desired->accumRedBits - current->accumRedBits);
            }

            if (desired->accumGreenBits > 0)
            {
                extraDiff += (desired->accumGreenBits - current->accumGreenBits) *
                             (desired->accumGreenBits - current->accumGreenBits);
            }

            if (desired->accumBlueBits > 0)
            {
                extraDiff += (desired->accumBlueBits - current->accumBlueBits) *
                             (desired->accumBlueBits - current->accumBlueBits);
            }

            if (desired->accumAlphaBits > 0)
            {
                extraDiff += (desired->accumAlphaBits - current->accumAlphaBits) *
                             (desired->accumAlphaBits - current->accumAlphaBits);
            }

            if (desired->samples > 0)
            {
                extraDiff += (desired->samples - current->samples) *
                             (desired->samples - current->samples);
            }
        }

        // Figure out if the current one is better than the best one found so far
        // Least number of missing buffers is the most important heuristic,
        // then color buffer size match and lastly size match for other buffers

        if (missing < leastMissing)
            closest = current;
        else if (missing == leastMissing)
        {
            if ((colorDiff < leastColorDiff) ||
                (colorDiff == leastColorDiff && extraDiff < leastExtraDiff))
            {
                closest = current;
            }
        }

        if (current == closest)
        {
            leastMissing = missing;
            leastColorDiff = colorDiff;
            leastExtraDiff = extraDiff;
        }
    }

    return closest;
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Create the GLFW window and its associated context
//========================================================================

GLFWAPI GLFWwindow glfwOpenWindow(int width, int height,
                                  int mode, const char* title,
                                  GLFWwindow share)
{
    _GLFWfbconfig fbconfig;
    _GLFWwndconfig wndconfig;
    _GLFWwindow* window;

    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return NULL;
    }

    window = (_GLFWwindow*) malloc(sizeof(_GLFWwindow));
    if (!window)
    {
        _glfwSetError(GLFW_OUT_OF_MEMORY, "glfwOpenWindow: Failed to allocate window structure");
        return NULL;
    }

    memset(window, 0, sizeof(_GLFWwindow));

    window->next = _glfwLibrary.windowListHead;
    _glfwLibrary.windowListHead = window;

    // Set up desired framebuffer config
    fbconfig.redBits        = Max(_glfwLibrary.hints.redBits, 0);
    fbconfig.greenBits      = Max(_glfwLibrary.hints.greenBits, 0);
    fbconfig.blueBits       = Max(_glfwLibrary.hints.blueBits, 0);
    fbconfig.alphaBits      = Max(_glfwLibrary.hints.alphaBits, 0);
    fbconfig.depthBits      = Max(_glfwLibrary.hints.depthBits, 0);
    fbconfig.stencilBits    = Max(_glfwLibrary.hints.stencilBits, 0);
    fbconfig.accumRedBits   = Max(_glfwLibrary.hints.accumRedBits, 0);
    fbconfig.accumGreenBits = Max(_glfwLibrary.hints.accumGreenBits, 0);
    fbconfig.accumBlueBits  = Max(_glfwLibrary.hints.accumBlueBits, 0);
    fbconfig.accumAlphaBits = Max(_glfwLibrary.hints.accumAlphaBits, 0);
    fbconfig.auxBuffers     = Max(_glfwLibrary.hints.auxBuffers, 0);
    fbconfig.stereo         = _glfwLibrary.hints.stereo ? GL_TRUE : GL_FALSE;
    fbconfig.samples        = Max(_glfwLibrary.hints.samples, 0);

    // Set up desired window config
    wndconfig.mode           = mode;
    wndconfig.title          = title;
    wndconfig.refreshRate    = Max(_glfwLibrary.hints.refreshRate, 0);
    wndconfig.windowNoResize = _glfwLibrary.hints.windowNoResize ? GL_TRUE : GL_FALSE;
    wndconfig.glMajor        = _glfwLibrary.hints.glMajor;
    wndconfig.glMinor        = _glfwLibrary.hints.glMinor;
    wndconfig.glForward      = _glfwLibrary.hints.glForward ? GL_TRUE : GL_FALSE;
    wndconfig.glDebug        = _glfwLibrary.hints.glDebug ? GL_TRUE : GL_FALSE;
    wndconfig.glProfile      = _glfwLibrary.hints.glProfile;
    wndconfig.share          = share;

    // Clear for next open call
    _glfwClearWindowHints();

    // Check the OpenGL bits of the window config
    if (!isValidContextConfig(&wndconfig))
    {
        glfwCloseWindow(window);
        return GL_FALSE;
    }

    if (mode != GLFW_WINDOWED && mode != GLFW_FULLSCREEN)
    {
        glfwCloseWindow(window);
        _glfwSetError(GLFW_INVALID_ENUM, "glfwOpenWindow: Invalid enum for 'mode' parameter");
        return GL_FALSE;
    }

    clearInputState(window);

    // Check width & height
    if (width > 0 && height <= 0)
    {
        // Set the window aspect ratio to 4:3
        height = (width * 3) / 4;
    }
    else if (width <= 0 && height > 0)
    {
        // Set the window aspect ratio to 4:3
        width = (height * 4) / 3;
    }
    else if (width <= 0 && height <= 0)
    {
        // Default window size
        width  = 640;
        height = 480;
    }

    // Remember window settings
    window->width  = width;
    window->height = height;
    window->mode   = mode;

    // Open the actual window and create its context
    if (!_glfwPlatformOpenWindow(window, &wndconfig, &fbconfig))
    {
        glfwCloseWindow(window);
        return GL_FALSE;
    }

    // Cache the actual (as opposed to desired) window parameters
    glfwMakeWindowCurrent(window);
    _glfwPlatformRefreshWindowParams();

    // As these are hard constraints when non-zero, we can simply copy them
    window->glProfile = wndconfig.glProfile;
    window->glForward = wndconfig.glForward;

    _glfwParseGLVersion(&window->glMajor, &window->glMinor, &window->glRevision);

    if (window->glMajor < wndconfig.glMajor ||
        (window->glMajor == wndconfig.glMajor &&
         window->glMinor < wndconfig.glMinor))
    {
        // The desired OpenGL version is greater than the actual version
        // This only happens if the machine lacks {GLX|WGL}_ARB_create_context
        // /and/ the user has requested an OpenGL version greater than 1.0

        // For API consistency, we emulate the behavior of the
        // {GLX|WGL}_ARB_create_context extension and fail here

        glfwCloseWindow(window);
        _glfwSetError(GLFW_VERSION_UNAVAILABLE, "glfwOpenWindow: The requested OpenGL version is not available");
        return GL_FALSE;
    }

    if (window->glMajor > 2)
    {
        // OpenGL 3.0+ uses a different function for extension string retrieval

        window->GetStringi = (PFNGLGETSTRINGIPROC) glfwGetProcAddress("glGetStringi");
        if (!window->GetStringi)
        {
            // This is a very common problem among people who compile GLFW
            // on X11/GLX using custom build systems, as it needs explicit
            // configuration in order to work

            glfwCloseWindow(window);
            _glfwSetError(GLFW_PLATFORM_ERROR, "glfwOpenWindow: Entry point retrieval is broken; see the build documentation for your platform");
            return GL_FALSE;
        }
    }

    // The GLFW specification states that fullscreen windows have the cursor
    // locked by default
    if (mode == GLFW_FULLSCREEN)
        glfwDisable(window, GLFW_MOUSE_CURSOR);

    // Clearing the front buffer to black to avoid garbage pixels left over
    // from previous uses of our bit of VRAM
    glClear(GL_COLOR_BUFFER_BIT);
    _glfwPlatformSwapBuffers();

    return window;
}


//========================================================================
// Make the OpenGL context associated with the specified window current
//========================================================================

GLFWAPI void glfwMakeWindowCurrent(GLFWwindow handle)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (_glfwLibrary.currentWindow == window)
        return;

    _glfwPlatformMakeWindowCurrent(window);
    _glfwLibrary.currentWindow = window;
}


//========================================================================
// Returns GL_TRUE if the specified window handle is an actual window
//========================================================================

GLFWAPI int glfwIsWindow(GLFWwindow handle)
{
    _GLFWwindow* entry;

    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return GL_FALSE;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (window == NULL)
        return GL_FALSE;

    for (entry = _glfwLibrary.windowListHead;  entry;  entry = entry->next)
    {
        if (entry == window)
            return GL_TRUE;
    }

    return GL_FALSE;
}


//========================================================================
// Returns GL_TRUE if the specified window handle is an actual window
//========================================================================

GLFWAPI GLFWwindow glfwGetCurrentWindow(void)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return GL_FALSE;
    }

    return _glfwLibrary.currentWindow;
}


//========================================================================
// Set hints for opening the window
//========================================================================

GLFWAPI void glfwOpenWindowHint(int target, int hint)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    switch (target)
    {
        case GLFW_RED_BITS:
            _glfwLibrary.hints.redBits = hint;
            break;
        case GLFW_GREEN_BITS:
            _glfwLibrary.hints.greenBits = hint;
            break;
        case GLFW_BLUE_BITS:
            _glfwLibrary.hints.blueBits = hint;
            break;
        case GLFW_ALPHA_BITS:
            _glfwLibrary.hints.alphaBits = hint;
            break;
        case GLFW_DEPTH_BITS:
            _glfwLibrary.hints.depthBits = hint;
            break;
        case GLFW_STENCIL_BITS:
            _glfwLibrary.hints.stencilBits = hint;
            break;
        case GLFW_REFRESH_RATE:
            _glfwLibrary.hints.refreshRate = hint;
            break;
        case GLFW_ACCUM_RED_BITS:
            _glfwLibrary.hints.accumRedBits = hint;
            break;
        case GLFW_ACCUM_GREEN_BITS:
            _glfwLibrary.hints.accumGreenBits = hint;
            break;
        case GLFW_ACCUM_BLUE_BITS:
            _glfwLibrary.hints.accumBlueBits = hint;
            break;
        case GLFW_ACCUM_ALPHA_BITS:
            _glfwLibrary.hints.accumAlphaBits = hint;
            break;
        case GLFW_AUX_BUFFERS:
            _glfwLibrary.hints.auxBuffers = hint;
            break;
        case GLFW_STEREO:
            _glfwLibrary.hints.stereo = hint;
            break;
        case GLFW_WINDOW_NO_RESIZE:
            _glfwLibrary.hints.windowNoResize = hint;
            break;
        case GLFW_FSAA_SAMPLES:
            _glfwLibrary.hints.samples = hint;
            break;
        case GLFW_OPENGL_VERSION_MAJOR:
            _glfwLibrary.hints.glMajor = hint;
            break;
        case GLFW_OPENGL_VERSION_MINOR:
            _glfwLibrary.hints.glMinor = hint;
            break;
        case GLFW_OPENGL_FORWARD_COMPAT:
            _glfwLibrary.hints.glForward = hint;
            break;
        case GLFW_OPENGL_DEBUG_CONTEXT:
            _glfwLibrary.hints.glDebug = hint;
            break;
        case GLFW_OPENGL_PROFILE:
            _glfwLibrary.hints.glProfile = hint;
            break;
        default:
            break;
    }
}


//========================================================================
// Properly kill the window / video display
//========================================================================

GLFWAPI void glfwCloseWindow(GLFWwindow handle)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    // Show mouse pointer again (if hidden)
    if (window == _glfwLibrary.cursorLockWindow)
        glfwEnable(window, GLFW_MOUSE_CURSOR);

    // Clear the current context if this window's context is current
    if (window == _glfwLibrary.currentWindow)
        glfwMakeWindowCurrent(NULL);

    // Clear the active window pointer if this is the active window
    if (window == _glfwLibrary.activeWindow)
        _glfwLibrary.activeWindow = NULL;

    _glfwPlatformCloseWindow(window);

    // Unlink window from global linked list
    {
        _GLFWwindow** prev = &_glfwLibrary.windowListHead;

        while (*prev != window)
            prev = &((*prev)->next);

        *prev = window->next;
    }

    free(window);
}


//========================================================================
// Set the window title
//========================================================================

GLFWAPI void glfwSetWindowTitle(GLFWwindow handle, const char* title)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    _glfwPlatformSetWindowTitle(window, title);
}


//========================================================================
// Get the window size
//========================================================================

GLFWAPI void glfwGetWindowSize(GLFWwindow handle, int* width, int* height)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (width != NULL)
        *width = window->width;

    if (height != NULL)
        *height = window->height;
}


//========================================================================
// Set the window size
//========================================================================

GLFWAPI void glfwSetWindowSize(GLFWwindow handle, int width, int height)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (window->iconified)
    {
        // TODO: Figure out if this is an error
        return;
    }

    // Don't do anything if the window size did not change
    if (width == window->width && height == window->height)
        return;

    _glfwPlatformSetWindowSize(window, width, height);

    if (window->mode == GLFW_FULLSCREEN)
    {
        // Refresh window parameters (may have changed due to changed video
        // modes)
        _glfwPlatformRefreshWindowParams();
    }
}


//========================================================================
// Get the window position
//========================================================================

GLFWAPI void glfwGetWindowPos(GLFWwindow handle, int* xpos, int* ypos)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (xpos != NULL)
        *xpos = window->positionX;

    if (ypos != NULL)
        *ypos = window->positionY;
}


//========================================================================
// Set the window position
//========================================================================

GLFWAPI void glfwSetWindowPos(GLFWwindow handle, int xpos, int ypos)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (window->mode == GLFW_FULLSCREEN || window->iconified)
    {
        // TODO: Figure out if this is an error
        return;
    }

    _glfwPlatformSetWindowPos(window, xpos, ypos);
}


//========================================================================
// Window iconification
//========================================================================

GLFWAPI void glfwIconifyWindow(GLFWwindow handle)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (window->iconified)
        return;

    _glfwPlatformIconifyWindow(window);
}


//========================================================================
// Window un-iconification
//========================================================================

GLFWAPI void glfwRestoreWindow(GLFWwindow handle)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    if (!window->iconified)
        return;

    // Restore iconified window
    _glfwPlatformRestoreWindow(window);

    if (window->mode == GLFW_FULLSCREEN)
        _glfwPlatformRefreshWindowParams();
}


//========================================================================
// Get window parameter
//========================================================================

GLFWAPI int glfwGetWindowParam(GLFWwindow handle, int param)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return 0;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    switch (param)
    {
        case GLFW_ACTIVE:
            return window == _glfwLibrary.activeWindow;
        case GLFW_ICONIFIED:
            return window->iconified;
        case GLFW_ACCELERATED:
            return window->accelerated;
        case GLFW_RED_BITS:
            return window->redBits;
        case GLFW_GREEN_BITS:
            return window->greenBits;
        case GLFW_BLUE_BITS:
            return window->blueBits;
        case GLFW_ALPHA_BITS:
            return window->alphaBits;
        case GLFW_DEPTH_BITS:
            return window->depthBits;
        case GLFW_STENCIL_BITS:
            return window->stencilBits;
        case GLFW_ACCUM_RED_BITS:
            return window->accumRedBits;
        case GLFW_ACCUM_GREEN_BITS:
            return window->accumGreenBits;
        case GLFW_ACCUM_BLUE_BITS:
            return window->accumBlueBits;
        case GLFW_ACCUM_ALPHA_BITS:
            return window->accumAlphaBits;
        case GLFW_AUX_BUFFERS:
            return window->auxBuffers;
        case GLFW_STEREO:
            return window->stereo;
        case GLFW_REFRESH_RATE:
            return window->refreshRate;
        case GLFW_WINDOW_NO_RESIZE:
            return window->windowNoResize;
        case GLFW_FSAA_SAMPLES:
            return window->samples;
        case GLFW_OPENGL_VERSION_MAJOR:
            return window->glMajor;
        case GLFW_OPENGL_VERSION_MINOR:
            return window->glMinor;
        case GLFW_OPENGL_FORWARD_COMPAT:
            return window->glForward;
        case GLFW_OPENGL_DEBUG_CONTEXT:
            return window->glDebug;
        case GLFW_OPENGL_PROFILE:
            return window->glProfile;
        default:
            _glfwSetError(GLFW_INVALID_ENUM, "glfwGetWindowParam: Invalid enum value for 'param' parameter");
            return 0;
    }
}


//========================================================================
// Set the user pointer for the specified window
//========================================================================

GLFWAPI void glfwSetWindowUserPointer(GLFWwindow handle, void* pointer)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    window->userPointer = pointer;
}


//========================================================================
// Get the user pointer for the specified window
//========================================================================

GLFWAPI void* glfwGetWindowUserPointer(GLFWwindow handle)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return NULL;
    }

    _GLFWwindow* window = (_GLFWwindow*) handle;

    return window->userPointer;
}


//========================================================================
// Set callback function for window size changes
//========================================================================

GLFWAPI void glfwSetWindowSizeCallback(GLFWwindowsizefun cbfun)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _glfwLibrary.windowSizeCallback = cbfun;

    // Call the callback function to let the application know the current
    // window size
    if (cbfun)
    {
        _GLFWwindow* window;

        for (window = _glfwLibrary.windowListHead;  window;  window = window->next)
            cbfun(window, window->width, window->height);
    }
}

//========================================================================
// Set callback function for window close events
//========================================================================

GLFWAPI void glfwSetWindowCloseCallback(GLFWwindowclosefun cbfun)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _glfwLibrary.windowCloseCallback = cbfun;
}


//========================================================================
// Set callback function for window refresh events
//========================================================================

GLFWAPI void glfwSetWindowRefreshCallback(GLFWwindowrefreshfun cbfun)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _glfwLibrary.windowRefreshCallback = cbfun;
}


//========================================================================
// Set callback function for window focus events
//========================================================================

GLFWAPI void glfwSetWindowFocusCallback(GLFWwindowfocusfun cbfun)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _glfwLibrary.windowFocusCallback = cbfun;
}


//========================================================================
// Set callback function for window iconification events
//========================================================================

GLFWAPI void glfwSetWindowIconifyCallback(GLFWwindowiconifyfun cbfun)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    _glfwLibrary.windowIconifyCallback = cbfun;
}


//========================================================================
// Poll for new window and input events and close any flagged windows
//========================================================================

GLFWAPI void glfwPollEvents(void)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    clearScrollOffsets();

    _glfwPlatformPollEvents();

    closeFlaggedWindows();
}


//========================================================================
// Wait for new window and input events
//========================================================================

GLFWAPI void glfwWaitEvents(void)
{
    if (!_glfwInitialized)
    {
        _glfwSetError(GLFW_NOT_INITIALIZED, NULL);
        return;
    }

    clearScrollOffsets();

    _glfwPlatformWaitEvents();

    closeFlaggedWindows();
}

