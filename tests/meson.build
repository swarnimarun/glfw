test_link_args = []
if cc.get_id() == 'msvc'
    test_link_args += '/ENTRY:mainCRTStartup' # use main() instead of WinMain()
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
glfwinfosrc_dep = getopt_src_deps + gladvk_src_deps

foreach t : [
            ['clipboard', getopt_src_deps,       []],
            ['cursor',    [],                    []],
            ['empty',     tinycthread_src_deps,  [rt_dep]],
            ['events',    getopt_src_deps,       []],
            ['gamma',     [],                    []],
            ['glfwinfo',  glfwinfosrc_dep,       []],
            ['icon',      [],                    []],
            ['iconify',   getopt_src_deps,       []],
            ['inputlag',  getopt_src_deps,       []],
            ['joysticks', [],                    []],
            ['monitors',  getopt_src_deps,       []],
            ['msaa',      getopt_src_deps,       []],
            ['opacity',   [],                    []],
            ['reopen',    [],                    []],
            ['tearing',   getopt_src_deps,       []],
            ['threads',   tinycthread_src_deps,  [rt_dep]],
            ['timeout',   [],                    []],
            ['title',     [],                    []],
            ['windows',   getopt_src_deps,       []],
            ]
    test_exec = executable(t[0],
               [t[0]+'.c', '../deps/glad_gl.c'] + t[1],
               link_args: test_link_args,
               dependencies: [glfw_dep, math_dep] + t[2],
               include_directories: [include_directories('../deps')])
    test(t[0], test_exec, is_parallel : false)
endforeach


### Links 'dep' vulkan to the test build
## VULKAN test
test_vk = executable('triangle-vulkan',
           ['triangle-vulkan.c', '../deps/glad_vulkan.c'],
           link_args: test_link_args,
           dependencies: [glfw_dep, math_dep],
           include_directories: [include_directories('../deps')])

test('vulkan', test_vk, is_parallel : false)