window_list_name = 'window-list'

window_list_sources = files(
  'window-list.cpp',
  'toplevel.cpp'
)

shared_module(window_list_name, window_list_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + window_list_name + '"',
        include_directories : incdir
)
