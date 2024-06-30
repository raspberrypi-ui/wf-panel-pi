name = 'clock'

sources = files(
  'clock.cpp',
  'clock.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(name + '.xml', install_dir: metadata_dir)
