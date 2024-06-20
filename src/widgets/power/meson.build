power_name = 'power'

power_sources = files(
  'power.cpp',
  'power.c'
)

shared_module(power_name, power_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + power_name + '"',
        include_directories : incdir
)
