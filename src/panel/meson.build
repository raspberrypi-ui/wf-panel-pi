panel_sources = files(
  'panel.cpp',
  'spacer.cpp'
)

util_sources = files(
  'gtk-utils.cpp',
  'wf-shell-app.cpp',
  'wf-autohide-window.cpp',
  'wf-popover.cpp',
  'conf-utils.cpp',
  'configure.c',
  'notification.c',
  'lxutils.c',
  'launcher.c'
)

config_sources = files(
   'config/types.cpp',
   'config/option.cpp',
   'config/section.cpp',
   'config/log.cpp',
   'config/xml.cpp',
   'config/config-manager.cpp',
   'config/file.cpp',
   'config/duration.cpp',
   'config/compound-option.cpp',
)

proj_args = '-DPLUGIN_PATH="' + get_option ('prefix') + '/' + get_option('libdir') + '/wf-panel-pi/"'

gtklayershell = dependency('gtk-layer-shell-0', version: '>= 0.1', fallback: ['gtk-layer-shell', 'gtk_layer_shell_dep'])
libfm = dependency('libfm-gtk3')
udev = dependency('libudev')
input = dependency('libinput')
evdev = dependency('libevdev')
libxml2 = dependency('libxml-2.0')
panel_deps = [ gtklayershell, libfm, udev, input, evdev, libxml2 ]

executable('wf-panel-pi', panel_sources + util_sources + config_sources,
        dependencies: deps + panel_deps,
        install: true,
        export_dynamic: true,
        c_args : proj_args,
        cpp_args : proj_args,
        include_directories : incdir
)

install_data('wfpanelctl', install_dir : '/usr/bin')
