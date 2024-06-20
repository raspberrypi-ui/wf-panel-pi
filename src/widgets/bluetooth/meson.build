bluetooth_name = 'bluetooth'

bluetooth_sources = files(
  'bluetooth.cpp',
  'bluetooth.c'
)

shared_module(bluetooth_name, bluetooth_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + bluetooth_name + '"',
        include_directories : incdir
)
