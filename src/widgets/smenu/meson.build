name = 'smenu'

sources = files(
  'smenu.cpp',
  'menu.c',
  'gtk-run.c'
)

menu_cache = dependency('libmenu-cache')
libfm = dependency('libfm-gtk3')
module_deps = [ menu_cache, libfm ]

metadata = files(
  name + '.xml'
)

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(metadata, install_dir: metadata_dir)
