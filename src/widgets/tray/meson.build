name = 'tray'

sources = files(
  'watcher.cpp',
  'tray.cpp',
  'item.cpp',
  'host.cpp'
)

dbusmenu_gtk   = dependency('dbusmenu-gtk3-0.4')
tray_deps = [ dbusmenu_gtk ]

shared_module(name, sources,
        dependencies: deps + tray_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
