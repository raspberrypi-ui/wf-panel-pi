name = 'netman'

sources = files(
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
  'ce-page-dsl.ui',
  'gsm-unlock.ui',
  'info.ui'
)

gnome = import('gnome')

sources += gnome.compile_resources(
  'applet-resources',
  'applet.gresource.xml',
  dependencies: resource_data
)

netman         = dependency('libnm')
nma            = dependency('libnma')
secret         = dependency('libsecret-1')
notify         = dependency('libnotify')
netman_deps = [ netman, nma, secret, notify ]

shared_module(name, sources,
        dependencies: deps + netman_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
