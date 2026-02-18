name = 'window-list'
menu_cache = dependency('libmenu-cache')

sources = files(
  'winlist.c',
  'winlist.cpp'
)

module_deps = [ menu_cache ]

metadata = files(
  name + '.xml'
)

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : '-DPLUGIN_NAME="' + name + '"',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(metadata, install_dir: metadata_dir)
