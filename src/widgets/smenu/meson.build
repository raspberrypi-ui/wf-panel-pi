name = 'smenu'

sources = files(
  'smenu.cpp',
  'menu.c',
  'gtk-run.c'
)

menu_cache     = dependency('libmenu-cache')
smenu_deps = [ menu_cache ]

shared_module(name, sources,
        dependencies: deps + smenu_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
