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
    dependencies += [dependency('wayland-client'),
                     dependency('wayland-cursor'),
                     dependency('wayland-server'),
                     dependency('xkbcommon'),
                     dependency('wayland-egl')]
    sources += ['wl_init.c',
                'wl_platform.h',
                'wl_monitor.c',
                'wl_window.c']
    # Wayland protocol generator
    dep_scanner = dependency('wayland-scanner', native: true)
    dep_wp = dependency('wayland-protocols')
    dir_wp_base = dep_wp.get_pkgconfig_variable('pkgdatadir')
    if meson.is_cross_build()
        prog_scanner = find_program('wayland-scanner', dep_scanner.get_pkgconfig_variable('wayland_scanner'))
    else
        prog_scanner = find_program(dep_scanner.get_pkgconfig_variable('wayland_scanner'))
    endif

    generated_protocols = [
        [ 'xdg-decoration', 'v1' ],
        [ 'idle-inhibit', 'v1' ],
        [ 'pointer-constraints', 'v1' ],
        [ 'relative-pointer', 'v1' ],
        [ 'viewporter', 'stable' ],
        [ 'xdg-shell', 'stable' ],
    ]

    foreach proto: generated_protocols
        proto_name = proto[0]
        if proto[1] == 'stable'
            base_file = proto_name
            xml_path = '@0@/stable/@1@/@1@.xml'.format(dir_wp_base, base_file)
        else
            base_file = '@0@-unstable-@1@'.format(proto_name, proto[1])
            xml_path = '@0@/unstable/@1@/@2@.xml'.format(dir_wp_base, proto_name, base_file)
        endif

        foreach output_type: [ 'client-header', 'server-header', 'private-code' ]
            if output_type == 'client-header'
                if proto_name == 'xdg-decoration'
                    output_file = 'wayland-@0@-client-protocol.h'.format(proto_name)
                else
                    output_file = 'wayland-@0@-client-protocol.h'.format(base_file)
                endif
            elif output_type == 'server-header'
                output_file = 'wayland-@0@-server-protocol.h'.format(base_file)
            else
                if proto_name == 'xdg-decoration'
                    output_file = 'wayland-@0@-protocol.c'.format(proto_name)
                else
                    output_file = 'wayland-@0@-protocol.c'.format(base_file)
                endif          
                if dep_scanner.version().version_compare('< 1.14.91')
                    output_type = 'code'
                endif
            endif

            var_name = output_file.underscorify()
            target = custom_target(
                '@0@ @1@'.format(base_file, output_type),
                command: [ prog_scanner, output_type, '@INPUT@', '@OUTPUT@' ],
                input: xml_path,
                output: output_file,
            )
            sources += target
        endforeach
    endforeach
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
                       objc_args: ['-D_GLFW_USE_CONFIG_H'],
                       dependencies: dependencies,
                       include_directories: [include_directories('../include'),
                                             include_directories('.')])

glfw_dep = declare_dependency(link_with: glfw_library,
                              include_directories:
                                    [include_directories('../include'),
                                     include_directories('.')])



