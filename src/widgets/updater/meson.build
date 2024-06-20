name = 'updater'

sources = files(
  'updater.cpp',
  'updater.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
