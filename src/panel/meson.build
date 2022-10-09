widget_sources = ['widgets/battery.cpp',
                  'widgets/menu.cpp',
                  'widgets/clock.cpp',
                  'widgets/launchers.cpp',
                  'widgets/network.cpp',
                  'widgets/spacing.cpp',
                  'widgets/lxutils.c',
                  'widgets/volumepulse.cpp',
                  'widgets/volumepulse/volumepulse.c',
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
	            'widgets/netman/netman-resources.c',
                  'widgets/bluetooth.cpp',
                  'widgets/bluetooth/bluetooth.c',
                  'widgets/window-list/window-list.cpp',
                  'widgets/window-list/toplevel.cpp',
                  'widgets/notifications/daemon.cpp',
                  'widgets/notifications/single-notification.cpp',
                  'widgets/notifications/notification-info.cpp',
                  'widgets/notifications/notification-center.cpp']

deps = [gtkmm, wayland_client, libutil, wf_protos, wfconfig, gtklayershell, menu_cache, libfm, netman, nma, secret, notify]

if libpulse.found()
  widget_sources += 'widgets/volume.cpp'
  deps += [libpulse, libgvc]
endif

executable('wf-panel', ['panel.cpp'] + widget_sources,
        dependencies: deps,
        install: true)
