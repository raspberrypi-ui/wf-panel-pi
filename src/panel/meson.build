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
        cpp_args : proj_args
)

# clock widget

clock_name = 'clock'

clock_sources = files(
  'widgets/clock/clock.cpp',
  'widgets/clock/clock.c'
)

shared_module(clock_name, clock_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + clock_name + '"'
)

# launcher widget

launchers_name = 'launchers'

launchers_sources = files(
  'widgets/launchers/launchers.cpp'
)

shared_module(launchers_name, launchers_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + launchers_name + '"'
)

# volumepulse widget

volumepulse_name = 'volumepulse'

volumepulse_sources = files(
  'widgets/volumepulse/volumepulse.cpp',
  'widgets/volumepulse/volumepulse.c',
  'widgets/volumepulse/micpulse.c',
  'widgets/volumepulse/commongui.c',
  'widgets/volumepulse/pulse.c',
  'widgets/volumepulse/bluetooth.c'
)

shared_module(volumepulse_name, volumepulse_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + volumepulse_name + '"'
)

# smenu widget

smenu_name = 'smenu'

smenu_sources = files(
  'widgets/smenu/smenu.cpp',
  'widgets/smenu/menu.c',
  'widgets/smenu/gtk-run.c'
)

shared_module(smenu_name, smenu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + smenu_name + '"'
)

# netman widget

netman_name = 'netman'

netman_sources = files(
  'widgets/netman/netman.cpp',
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

gnome = import('gnome')

netman_sources += gnome.compile_resources(
  'applet-resources',
  'widgets/netman/applet.gresource.xml',
  dependencies: resource_data,
  source_dir: 'widgets/netman'
)

shared_module(netman_name, netman_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + netman_name + '"'
)

# bluetooth widget

bluetooth_name = 'bluetooth'

bluetooth_sources = files(
  'widgets/bluetooth/bluetooth.cpp',
  'widgets/bluetooth/bluetooth.c'
)

shared_module(bluetooth_name, bluetooth_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + bluetooth_name + '"'
)

# ejecter widget

ejecter_name = 'ejecter'

ejecter_sources = files(
  'widgets/ejecter/ejecter.cpp',
  'widgets/ejecter/ejecter.c'
)

shared_module(ejecter_name, ejecter_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + ejecter_name + '"'
)

# updater widget

updater_name = 'updater'

updater_sources = files(
  'widgets/updater/updater.cpp',
  'widgets/updater/updater.c'
)

shared_module(updater_name, updater_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + updater_name + '"'
)

# cpu widget

cpu_name = 'cpu'

cpu_sources = files(
  'widgets/cpu/cpu.cpp',
  'widgets/cpu/cpu.c'
)

shared_module(cpu_name, cpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cpu_name + '"'
)

# cputemp widget

cputemp_name = 'cputemp'

cputemp_sources = files(
  'widgets/cputemp/cputemp.cpp',
  'widgets/cputemp/cputemp.c'
)

shared_module('cputemp', cputemp_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cputemp_name + '"'
)

# gpu widget

gpu_name = 'gpu'

gpu_sources = files(
  'widgets/gpu/gpu.cpp',
  'widgets/gpu/gpu.c'
)

shared_module(gpu_name, gpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + gpu_name + '"'
)

# power widget

power_name = 'power'

power_sources = files(
  'widgets/power/power.cpp',
  'widgets/power/power.c'
)

shared_module(power_name, power_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + power_name + '"'
)

# batt widget

batt_name = 'batt'

batt_sources = files(
  'widgets/batt/batt.cpp',
  'widgets/batt/batt.c',
  'widgets/batt/batt_sys.c'
)

shared_module(batt_name, batt_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + batt_name + '"'
)

# window-list widget

window_list_name = 'window-list'

window_list_sources = files(
  'widgets/window-list/window-list.cpp',
  'widgets/window-list/toplevel.cpp'
)

shared_module(window_list_name, window_list_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + window_list_name + '"'
)

# tray widget

tray_name = 'tray'

tray_sources = files(
  'widgets/tray/watcher.cpp',
  'widgets/tray/tray.cpp',
  'widgets/tray/item.cpp',
  'widgets/tray/host.cpp'
)

shared_module(tray_name, tray_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + tray_name + '"'
)

# notify widget - only for config dialog

notify_name = 'notify'

notify_sources = files(
  'widgets/notify/notify.cpp'
)

shared_module(notify_name, notify_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel'
)

# spacing widget - only for config dialog

spacing_name = 'spacing'

spacing_sources = files(
  'widgets/spacing/spacing.cpp'
)

shared_module(spacing_name, spacing_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel'
)


install_data('wfpanelctl', install_dir : '/usr/bin')
