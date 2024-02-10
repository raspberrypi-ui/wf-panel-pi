gnome = import('gnome')

widget_sources = files(
  'widgets/clock.cpp',
  'widgets/kbdlayout.cpp',
  'widgets/launchers.cpp',
  'widgets/spacing.cpp',
  'widgets/volumepulse.cpp',
  'widgets/volumepulse/volumepulse.c',
  'widgets/volumepulse/micpulse.c',
  'widgets/volumepulse/commongui.c',
  'widgets/volumepulse/pulse.c',
  'widgets/volumepulse/bluetooth.c',
  'widgets/smenu.cpp',
  'widgets/smenu/menu.c',
  'widgets/smenu/gtk-run.c',
  'widgets/netman.cpp',
  'widgets/netman/netman.c',
  'widgets/netman/ap-menu-item.c',
  'widgets/netman/applet.c',
  'widgets/netman/applet-device-bt.c',
  'widgets/netman/applet-device-wifi.c',
  'widgets/netman/applet-vpn-request.c',
  'widgets/netman/mb-menu-item.c',
  'widgets/netman/applet-agent.c',
  'widgets/netman/applet-device-ethernet.c',
  'widgets/netman/applet-dialogs.c',
  'widgets/netman/ethernet-dialog.c',
  'widgets/netman/mobile-helpers.c',
  'widgets/netman/utils.c',
  'widgets/netman/eap-method.c',
  'widgets/netman/nm-compat.c',
  'widgets/netman/nm-shared-utils.c',
  'widgets/bluetooth.cpp',
  'widgets/bluetooth/bluetooth.c',
  'widgets/ejecter.cpp',
  'widgets/ejecter/ejecter.c',
  'widgets/updater.cpp',
  'widgets/updater/updater.c',
  'widgets/cpu.cpp',
  'widgets/cpu/cpu.c',
  'widgets/cputemp.cpp',
  'widgets/cputemp/cputemp.c',
  'widgets/gpu.cpp',
  'widgets/gpu/gpu.c',
  'widgets/power.cpp',
  'widgets/power/power.c',
  'widgets/batt.cpp',
  'widgets/batt/batt.c',
  'widgets/batt/batt_sys.c',
  'widgets/window-list/window-list.cpp',
  'widgets/window-list/toplevel.cpp',
  'widgets/launcher.c',
  'widgets/tray/watcher.cpp',
  'widgets/tray/tray.cpp',
  'widgets/tray/item.cpp',
  'widgets/tray/host.cpp'
)

resource_data = files(
  'widgets/netman/8021x.ui',
  'widgets/netman/connection-editor/ce-page-dsl.ui',
  'widgets/netman/gsm-unlock.ui',
  'widgets/netman/info.ui'
)

widget_sources += gnome.compile_resources(
  'applet-resources',
  'widgets/netman/applet.gresource.xml',
  dependencies: resource_data,
  source_dir: 'widgets/netman'
)

util_sources = files(
  'gtk-utils.cpp',
  'wf-shell-app.cpp',
  'wf-autohide-window.cpp',
  'wf-popover.cpp',
  'conf-utils.cpp',
  'configure.c',
  'notification.c'
)

deps = [gtkmm, wayland_client, wf_protos, wfconfig, gtklayershell, menu_cache, libfm, netman, nma, secret, notify, packagekit, libpulse, udev, dbusmenu_gtk, xkbcommon]

executable('wf-panel-pi', ['panel.cpp', 'lxutils.c'] + widget_sources + util_sources,
        dependencies: deps,
        install: true
)

install_data('wfpanelctl', install_dir : '/usr/bin')
