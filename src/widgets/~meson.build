incdir = include_directories('../panel/')

# clock widget

clock_name = 'clock'

clock_sources = files(
  'clock/clock.cpp',
  'clock/clock.c'
)

shared_module(clock_name, clock_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + clock_name + '"',
        include_directories : incdir
)

# launcher widget

launchers_name = 'launchers'

launchers_sources = files(
  'launchers/launchers.cpp'
)

shared_module(launchers_name, launchers_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + launchers_name + '"',
        include_directories : incdir
)

# volumepulse widget

volumepulse_name = 'volumepulse'

volumepulse_sources = files(
  'volumepulse/volumepulse.cpp',
  'volumepulse/volumepulse.c',
  'volumepulse/micpulse.c',
  'volumepulse/commongui.c',
  'volumepulse/pulse.c',
  'volumepulse/bluetooth.c'
)

shared_module(volumepulse_name, volumepulse_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + volumepulse_name + '"',
        include_directories : incdir
)

# smenu widget

smenu_name = 'smenu'

smenu_sources = files(
  'smenu/smenu.cpp',
  'smenu/menu.c',
  'smenu/gtk-run.c'
)

shared_module(smenu_name, smenu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + smenu_name + '"',
        include_directories : incdir
)

# netman widget

netman_name = 'netman'

netman_sources = files(
  'netman/netman.cpp',
  'netman/netman.c',
  'netman/ap-menu-item.c',
  'netman/applet.c',
  'netman/applet-device-bt.c',
  'netman/applet-device-wifi.c',
  'netman/applet-vpn-request.c',
  'netman/mb-menu-item.c',
  'netman/applet-agent.c',
  'netman/applet-device-ethernet.c',
  'netman/applet-dialogs.c',
  'netman/ethernet-dialog.c',
  'netman/mobile-helpers.c',
  'netman/utils.c',
  'netman/eap-method.c',
  'netman/nm-compat.c',
  'netman/nm-shared-utils.c'
)

resource_data = files(
  'netman/8021x.ui',
  'netman/connection-editor/ce-page-dsl.ui',
  'netman/gsm-unlock.ui',
  'netman/info.ui'
)

gnome = import('gnome')

netman_sources += gnome.compile_resources(
  'applet-resources',
  'netman/applet.gresource.xml',
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

# bluetooth widget

bluetooth_name = 'bluetooth'

bluetooth_sources = files(
  'bluetooth/bluetooth.cpp',
  'bluetooth/bluetooth.c'
)

shared_module(bluetooth_name, bluetooth_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + bluetooth_name + '"',
        include_directories : incdir
)

# ejecter widget

ejecter_name = 'ejecter'

ejecter_sources = files(
  'ejecter/ejecter.cpp',
  'ejecter/ejecter.c'
)

shared_module(ejecter_name, ejecter_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + ejecter_name + '"',
        include_directories : incdir
)

# updater widget

updater_name = 'updater'

updater_sources = files(
  'updater/updater.cpp',
  'updater/updater.c'
)

shared_module(updater_name, updater_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + updater_name + '"',
        include_directories : incdir
)

# cpu widget

cpu_name = 'cpu'

cpu_sources = files(
  'cpu/cpu.cpp',
  'cpu/cpu.c'
)

shared_module(cpu_name, cpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cpu_name + '"',
        include_directories : incdir
)

# cputemp widget

cputemp_name = 'cputemp'

cputemp_sources = files(
  'cputemp/cputemp.cpp',
  'cputemp/cputemp.c'
)

shared_module('cputemp', cputemp_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cputemp_name + '"',
        include_directories : incdir
)

# gpu widget

gpu_name = 'gpu'

gpu_sources = files(
  'gpu/gpu.cpp',
  'gpu/gpu.c'
)

shared_module(gpu_name, gpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + gpu_name + '"',
        include_directories : incdir
)

# power widget

power_name = 'power'

power_sources = files(
  'power/power.cpp',
  'power/power.c'
)

shared_module(power_name, power_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + power_name + '"',
        include_directories : incdir
)

# batt widget

batt_name = 'batt'

batt_sources = files(
  'batt/batt.cpp',
  'batt/batt.c',
  'batt/batt_sys.c'
)

shared_module(batt_name, batt_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + batt_name + '"',
        include_directories : incdir
)

# window-list widget

window_list_name = 'window-list'

window_list_sources = files(
  'window-list/window-list.cpp',
  'window-list/toplevel.cpp'
)

shared_module(window_list_name, window_list_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + window_list_name + '"',
        include_directories : incdir
)

# tray widget

tray_name = 'tray'

tray_sources = files(
  'tray/watcher.cpp',
  'tray/tray.cpp',
  'tray/item.cpp',
  'tray/host.cpp'
)

shared_module(tray_name, tray_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + tray_name + '"',
        include_directories : incdir
)

# notify widget - only for config dialog

notify_name = 'notify'

notify_sources = files(
  'notify/notify.cpp'
)

shared_module(notify_name, notify_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        include_directories : incdir
)

# spacing widget - only for config dialog

spacing_name = 'spacing'

spacing_sources = files(
  'spacing/spacing.cpp'
)

shared_module(spacing_name, spacing_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        include_directories : incdir
)
