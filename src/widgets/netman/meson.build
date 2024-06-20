netman_name = 'netman'

netman_sources = files(
  'netman.cpp',
  'netman.c',
  'ap-menu-item.c',
  'applet.c',
  'applet-device-bt.c',
  'applet-device-wifi.c',
  'applet-vpn-request.c',
  'mb-menu-item.c',
  'applet-agent.c',
  'applet-device-ethernet.c',
  'applet-dialogs.c',
  'ethernet-dialog.c',
  'mobile-helpers.c',
  'utils.c',
  'eap-method.c',
  'nm-compat.c',
  'nm-shared-utils.c'
)

resource_data = files(
  '8021x.ui',
  'connection-editor/ce-page-dsl.ui',
  'gsm-unlock.ui',
  'info.ui'
)

gnome = import('gnome')

netman_sources += gnome.compile_resources(
  'applet-resources',
  'applet.gresource.xml',
  dependencies: resource_data,
  source_dir: 'netman'
)

shared_module(netman_name, netman_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + netman_name + '"',
        include_directories : incdir
)
