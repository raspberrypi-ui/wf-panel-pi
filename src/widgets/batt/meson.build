batt_name = 'batt'

batt_sources = files(
  'batt.cpp',
  'batt.c',
  'batt_sys.c'
)

shared_module(batt_name, batt_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + batt_name + '"',
        include_directories : incdir
)

