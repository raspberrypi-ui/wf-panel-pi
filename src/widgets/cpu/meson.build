name = 'cpu'

sources = files(
  'cpu.cpp',
  'cpu.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data('cpu.xml', install_dir: metadata_dir)
