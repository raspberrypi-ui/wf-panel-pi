name = 'window-list'
menu_cache = dependency('libmenu-cache')

sources = files(
  'winlist.c',
  'winlist.cpp'
)

module_deps = [ menu_cache ]

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : [ '-DPLUGIN_NAME="' + name + '"', '-DUSES_MENUCACHE' ],
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
