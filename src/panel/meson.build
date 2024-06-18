gnome = import('gnome')

widget_sources = files(
  'widgets/spacing.cpp'
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
  'widgets/launcher.c'
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

deps = [gtkmm, wayland_client, wf_protos, gtklayershell, menu_cache, libfm, netman, nma, secret, notify, packagekit, libpulse, udev, dbusmenu_gtk, input, glm, evdev, libxml2]

executable('wf-panel-pi', ['panel.cpp'] + widget_sources + util_sources + config_sources,
        dependencies: deps,
        install: true,
        export_dynamic: true
)

clock_sources = files(
  'widgets/clock.cpp',
  'widgets/clock/clock.c'
)

shared_module('clock', clock_sources,
        dependencies: deps,
        install: true
)

launchers_sources = files(
  'widgets/launchers.cpp'
)

shared_module('launchers', launchers_sources,
        dependencies: deps,
        install: true
)

volumepulse_sources = files(
  'widgets/volumepulse.cpp',
  'widgets/volumepulse/volumepulse.c',
  'widgets/volumepulse/micpulse.c',
  'widgets/volumepulse/commongui.c',
  'widgets/volumepulse/pulse.c',
  'widgets/volumepulse/bluetooth.c'
)

shared_module('volumepulse', volumepulse_sources,
        dependencies: deps,
        install: true
)

smenu_sources = files(
  'widgets/smenu.cpp',
  'widgets/smenu/menu.c',
  'widgets/smenu/gtk-run.c'
)

shared_module('smenu', smenu_sources,
        dependencies: deps,
        install: true
)

netman_sources = files(
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
  'widgets/netman/nm-shared-utils.c'
)

resource_data = files(
  'widgets/netman/8021x.ui',
  'widgets/netman/connection-editor/ce-page-dsl.ui',
  'widgets/netman/gsm-unlock.ui',
  'widgets/netman/info.ui'
)

netman_sources += gnome.compile_resources(
  'applet-resources',
  'widgets/netman/applet.gresource.xml',
  dependencies: resource_data,
  source_dir: 'widgets/netman'
)

shared_module('netman', netman_sources,
        dependencies: deps,
        install: true
)

bluetooth_sources = files(
  'widgets/bluetooth.cpp',
  'widgets/bluetooth/bluetooth.c'
)

shared_module('bluetooth', bluetooth_sources,
        dependencies: deps,
        install: true
)

ejecter_sources = files(
  'widgets/ejecter.cpp',
  'widgets/ejecter/ejecter.c'
)

shared_module('ejecter', ejecter_sources,
        dependencies: deps,
        install: true
)

updater_sources = files(
  'widgets/updater.cpp',
  'widgets/updater/updater.c'
)

shared_module('updater', updater_sources,
        dependencies: deps,
        install: true
)

cpu_sources = files(
  'widgets/cpu.cpp',
  'widgets/cpu/cpu.c'
)

shared_module('cpu', cpu_sources,
        dependencies: deps,
        install: true
)

cputemp_sources = files(
  'widgets/cputemp.cpp',
  'widgets/cputemp/cputemp.c'
)

shared_module('cputemp', cputemp_sources,
        dependencies: deps,
        install: true
)

gpu_sources = files(
  'widgets/gpu.cpp',
  'widgets/gpu/gpu.c'
)

shared_module('gpu', gpu_sources,
        dependencies: deps,
        install: true
)

power_sources = files(
  'widgets/power.cpp',
  'widgets/power/power.c'
)

shared_module('power', power_sources,
        dependencies: deps,
        install: true
)

batt_sources = files(
  'widgets/batt.cpp',
  'widgets/batt/batt.c',
  'widgets/batt/batt_sys.c'
)

shared_module('batt', batt_sources,
        dependencies: deps,
        install: true
)

window_list_sources = files(
  'widgets/window-list/window-list.cpp',
  'widgets/window-list/toplevel.cpp'
)

shared_module('window-list', window_list_sources,
        dependencies: deps,
        install: true
)

tray_sources = files(
  'widgets/tray/watcher.cpp',
  'widgets/tray/tray.cpp',
  'widgets/tray/item.cpp',
  'widgets/tray/host.cpp'
)

shared_module('tray', tray_sources,
        dependencies: deps,
        install: true
)
install_data('wfpanelctl', install_dir : '/usr/bin')
