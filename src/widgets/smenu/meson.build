smenu_name = 'smenu'

smenu_sources = files(
  'smenu.cpp',
  'menu.c',
  'gtk-run.c'
)

shared_module(smenu_name, smenu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + smenu_name + '"',
        include_directories : incdir
)
