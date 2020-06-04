test_link_args = []
if cc.get_id() == 'msvc'
    test_link_args += '/ENTRY:mainCRTStartup' # use main() instead of WinMain()
endif

src = []
if backend == 'win32'
    src += windows.compile_resources('glfw.rc')
elif backend == 'cocoa'
    # Test out all of the glfw MacOS stuff
    install_data('glfw.icns',
    install_dir : 'Contents/Resources')
    install_data('../Info.plist.in',
    install_dir : 'Contents')
    # not sure how to setup target properties in Mac
endif


getopt_src_deps = [
    '../deps/getopt.c',
]
tinycthread_src_deps = [
    '../deps/tinycthread.c',
]
gladvk_src_deps = [
    '../deps/glad_vulkan.c',
]

foreach t : [
            ['boing',           []],
            ['gears',           []],
            ['heightmap',       []],
            ['offscreen',       []],
            ['particles',       [getopt_src_deps, tinycthread_src_deps]],
            ['sharing',         []],
            ['splitview',       []],
            ['triangle-opengl', []],
            ['wave',            []],
            ]
    executable(t[0],
               [t[0]+'.c', '../deps/glad_gl.c'] + t[1] + src,
               link_args: test_link_args,
               dependencies: [glfw_dep, math_dep],
               include_directories: [include_directories('../deps')])
endforeach