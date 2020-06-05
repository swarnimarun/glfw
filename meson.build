project('glfw', 'c',
    version: '3.3.2',
    license: 'zlib')

cc = meson.get_compiler('c')
compiler_id = cc.get_id()

math_dep = cc.find_library('m', required: false)
rt_dep = cc.find_library('rt', required: false)
threads_dep = dependency('threads')


#--------------------------------------------------------------------
# Set compiler specific flags
#--------------------------------------------------------------------

if compiler_id == 'msvc'
    if get_option('glfw_use_msvc_runtime_library_dll')
        # don't know of a way of changing `b_vscrt = mt` setting from the meson script
        # we could probably use a python script for `meson configure` if it's important
        # though preferrably I don't like the option of \MT in msvc
        # so I will just leave it alone
    endif
endif


#--------------------------------------------------------------------
# Detect and select backend APIs
#--------------------------------------------------------------------

host_system = host_machine.system()

backend = get_option('glfw_backend')
if backend == 'auto'
    if host_system == 'windows' or host_system == 'cygwin'
        backend = 'win32'
        add_languages('objc')
    elif host_system == 'darwin'
        backend = 'cocoa'
    elif host_system == 'linux' or host_system == 'bsd'
        backend = 'x11'
    else
        error('Unable to determine the backend automatically.')
    endif
endif


#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------

subdir('src')

if get_option('glfw_build_tests')
    subdir('tests')
endif

if get_option('glfw_build_examples')
    subdir('examples')
endif

if get_option('glfw_build_docs')
    subdir('docs') # TODO: Finish implementing this
endif

# option(GLFW_VULKAN_STATIC "Use the Vulkan loader statically linked into application" OFF)

# will have to make sure this is gated properly later
# option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)


#--------------------------------------------------------------------
# Install files
#--------------------------------------------------------------------
# The current option is to have the user
# Use the meson install options and configure the install manually
