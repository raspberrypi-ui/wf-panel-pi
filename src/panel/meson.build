cpp_sources = files(
  'panel.cpp',
  'panel-app.cpp',
  'autohide-window.cpp'
)

c_sources = files(
  'plug_conf.c',
  'notification.c',
  'plugin.c',
  'launcher.c'
)

proj_args = [ '-DPLUGIN_PATH="' + get_option ('prefix') + '/' + get_option('libdir') + '/wf-panel-pi/"', '-DUSES_MENUCACHE' ]

gtklayershell = dependency('gtk-layer-shell-0', version: '>= 0.1', fallback: ['gtk-layer-shell', 'gtk_layer_shell_dep'])
udev = dependency('libudev')
input = dependency('libinput')
evdev = dependency('libevdev')
libxml2 = dependency('libxml-2.0')
giomm = dependency('giomm-2.4')
menu_cache = dependency('libmenu-cache')
panel_deps = [ gtklayershell, udev, input, evdev, libxml2, giomm, menu_cache ]

executable('wf-panel-pi', cpp_sources + c_sources,
        dependencies: deps + panel_deps,
        install: true,
        export_dynamic: true,
        c_args : proj_args,
        cpp_args : proj_args,
        include_directories : incdir
)
