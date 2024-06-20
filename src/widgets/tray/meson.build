tray_name = 'tray'

tray_sources = files(
  'watcher.cpp',
  'tray.cpp',
  'item.cpp',
  'host.cpp'
)

shared_module(tray_name, tray_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + tray_name + '"',
        include_directories : incdir
)
