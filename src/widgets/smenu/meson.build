name = 'smenu'

sources = files(
  'smenu.cpp',
  'menu.c',
  'gtk-run.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
