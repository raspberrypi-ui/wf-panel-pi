name = 'smenu'

sources = files(
  'smenu.cpp',
  'menu.c',
  'gtk-run.c'
)

menu_cache = dependency('libmenu-cache')
libfm = dependency('libfm-gtk3')
smenu_deps = [ menu_cache, libfm ]

shared_module(name, sources,
        dependencies: deps + smenu_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(name + '.xml', install_dir: metadata_dir)
