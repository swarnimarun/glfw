doxygen = find_program('doxygen', required: true)
if doxygen.found()
  conf_data = configuration_data()
  conf_data.set('GLFW_SOURCE_DIR', meson.source_root())
  conf_data.set('GLFW_DOXYGEN_INPUT', meson.source_root() + '/include/GLFW' + ' ' + meson.source_root() + '/docs')
  conf_data.set('GLFW_BINARY_DIR', meson.current_build_dir())
  conf_data.set('GLFW_VERSION', '3.3.2')

  doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: conf_data
  )

  custom_target('doc',
    build_by_default: true,
    command: [doxygen, doxyfile],
    output: ['html']
  )
endif
