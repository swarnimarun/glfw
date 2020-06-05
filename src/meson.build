#--------------------------------------------------------------------
# Add common sources
#--------------------------------------------------------------------

dependencies = []

sources = ['internal.h',
           'mappings.h',
           '../include/GLFW/glfw3.h',
           '../include/GLFW/glfw3native.h',
           'context.c',
           'init.c',
           'input.c',
           'monitor.c',
           'vulkan.c',
           'window.c']

if host_system == 'linux' or host_system == 'bsd'
    dependencies += [rt_dep, math_dep, cc.find_library('dl', required: false)]
endif


#--------------------------------------------------------------------
# Create generated files
#--------------------------------------------------------------------

conf_data = configuration_data()
foreach b : ['win32',
             'x11',
             'wayland',
             'cocoa',
             'osmesa']
    conf_data.set('_GLFW_'+b.to_upper(), backend == b)
endforeach
conf_data.set('_GLFW_BUILD_DLL', get_option('default_library') == 'shared')
conf_data.set('_GLFW_VULKAN_STATIC', get_option('glfw_vulkan_static'))
conf_data.set('_GLFW_USE_HYBRID_HPG', get_option('glfw_use_hybrid_hpg'))
conf_file = configure_file(configuration: conf_data,
                           output: 'glfw_config.h')
sources += conf_file

# not sure if there is a way to setup pc files in meson need to check
#configure_file(input:  'src/glfw3.pc.in',
#               output: 'src/glfw3.pc',
#               configuration: conf_data)



#--------------------------------------------------------------------
# Add multithreading sources
#--------------------------------------------------------------------

dependencies += threads_dep

if host_system == 'windows'
    sources += ['win32_thread.c']
else
    sources += ['posix_thread.h',
                'posix_thread.c']
endif


#--------------------------------------------------------------------
# Add joystick sources
#--------------------------------------------------------------------

if host_system == 'windows'
    sources += ['win32_joystick.h',
                'win32_joystick.c']
elif host_system == 'darwin'
    sources += ['cocoa_joystick.h',
                'cocoa_joystick.m']
elif host_system == 'linux'
    sources += ['linux_joystick.h',
                'linux_joystick.c']
else
    sources += ['null_joystick.h',
                'null_joystick.c']
endif


#--------------------------------------------------------------------
# Add time sources
#--------------------------------------------------------------------

if host_system == 'windows'
    sources += ['win32_time.c']
elif host_system == 'darwin'
    sources += ['cocoa_time.c']
else
    sources += ['posix_time.h',
                'posix_time.c']
endif


#--------------------------------------------------------------------
# Add OpenGL context sources
#--------------------------------------------------------------------

sources += ['osmesa_context.h',
            'osmesa_context.c']

if backend != 'osmesa'
    sources += ['egl_context.h',
                'egl_context.c']
endif

if backend == 'win32'
    sources += ['wgl_context.h',
                'wgl_context.c']
elif backend == 'x11'
    sources += ['glx_context.h',
                'glx_context.c']
elif backend == 'cocoa'
    sources += ['nsgl_context.h',
                'nsgl_context.m']
endif


#--------------------------------------------------------------------
# Add Vulkan/MoltenVK sources
#--------------------------------------------------------------------
if get_option('glfw_vulkan_static')
    # not sure about this example could be broken needs more testing
    if backend == 'cocoa'
        # Dependencies required by the MoltenVK
        dependencies += [dependency('appleframeworks', modules : ['Metal', 'MoltenVK', 'QuartzCore'], required: true)]
    else
        dependencies += [dependency('vulkan', required: true)] # TODO: Add checking and stuff
    endif
endif


#--------------------------------------------------------------------
# Add backend sources
#--------------------------------------------------------------------

if backend == 'win32'
    dependencies += cc.find_library('gdi32')
    sources += ['win32_init.c',
                'win32_platform.h',
                'win32_monitor.c',
                'win32_window.c']
elif backend == 'x11'
    dependencies += [dependency('x11')
                    ,dependency('xrandr')
                    ,dependency('xinerama')
                    ,dependency('xcursor')]
    sources += ['x11_init.c',
                'x11_platform.h',
                'x11_monitor.c',
                'x11_window.c']
elif backend == 'wayland'
    dependencies += [dependency('ecm'),
                     dependency('wayland'),
                     dependency('wayland-scanner'),
                     dependency('wayland-protocols', version: '1.1'),
                     dependency('wayland-egl')]
    sources += ['wl_init.c',
                'wl_platform.h',
                'wl_monitor.c',
                'wl_window.c']
    # TODO: Test it, there might be issues here
elif backend == 'mir'
    dependencies += [dependency('mirclient')]
    sources += ['mir_init.c',
                'mir_platform.h',
                'mir_monitor.c',
                'mir_window.c']
elif backend == 'osmesa'
    dependencies += [dependency('osmesa')]
    sources += ['null_init.c',
                'null_platform.h',
                'null_monitor.c',
                'null_window.c']
elif backend == 'cocoa'
    # can't test but would likely blame the issue below for any bugs :P
    # https://github.com/mesonbuild/meson/issues/3207
    dependencies += [dependency('appleframeworks', modules : ['CoreFoundation', 'CoreVideo', 'Cocoa', 'IOKit'])]
    sources += ['cocoa_init.m',
                'cocoa_platform.h',
                'cocoa_monitor.m',
                'cocoa_window.m']
endif


#--------------------------------------------------------------------
# Add other sources
#--------------------------------------------------------------------

if host_system == 'linux' and backend != 'osmesa'
    if backend != 'x11' # ....
        dependencies += dependency('xkbcommon')
    endif
    sources += ['xkb_unicode.h',
                'xkb_unicode.c']
endif


#--------------------------------------------------------------------
# Declare library and its dependency object
#--------------------------------------------------------------------

glfw_library = library('glfw',
                       sources,
                       version: meson.project_version(),
                       c_args: ['-D_GLFW_USE_CONFIG_H'],
                       dependencies: dependencies,
                       include_directories: [include_directories('../include'),
                                             include_directories('.')])

glfw_dep = declare_dependency(link_with: glfw_library,
                              include_directories:
                                    [include_directories('../include'),
                                     include_directories('.')])



