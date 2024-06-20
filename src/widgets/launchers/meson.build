launchers_name = 'launchers'

launchers_sources = files(
  'launchers.cpp'
)

shared_module(launchers_name, launchers_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + launchers_name + '"',
        include_directories : incdir
)
