updater_name = 'updater'

updater_sources = files(
  'updater.cpp',
  'updater.c'
)

shared_module(updater_name, updater_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + updater_name + '"',
        include_directories : incdir
)
