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

proj_args = '-DPLUGIN_PATH="' + get_option ('prefix') + '/' + get_option('libdir') + '/wfpanel/"'

deps = [gtkmm, wayland_client, wf_protos, gtklayershell, menu_cache, libfm, netman, nma, secret, notify, packagekit, libpulse, udev, dbusmenu_gtk, input, glm, evdev, libxml2]

executable('wf-panel-pi', panel_sources + util_sources + config_sources,
        dependencies: deps,
        install: true,
        export_dynamic: true,
        c_args : proj_args,
        cpp_args : proj_args,
        include_directories : incdir
)

install_data('wfpanelctl', install_dir : '/usr/bin')
