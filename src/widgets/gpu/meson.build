name = 'gpu'

sources = files(
  'gpu.cpp',
  'gpu.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data('gpu.xml', install_dir: metadata_dir)
