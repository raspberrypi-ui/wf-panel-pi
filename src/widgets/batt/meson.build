name = 'batt'

sources = files(
  'batt.cpp',
  'batt.c',
  'batt_sys.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

