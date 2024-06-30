name = 'window-list'

sources = files(
  'window-list.cpp',
  'toplevel.cpp'
)

module_deps = [ ]

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(name + '.xml', install_dir: metadata_dir)
