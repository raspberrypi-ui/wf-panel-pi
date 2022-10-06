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
                  'widgets/window-list/window-list.cpp',
                  'widgets/window-list/toplevel.cpp',
                  'widgets/notifications/daemon.cpp',
                  'widgets/notifications/single-notification.cpp',
                  'widgets/notifications/notification-info.cpp',
                  'widgets/notifications/notification-center.cpp']

deps = [gtkmm, wayland_client, libutil, wf_protos, wfconfig, gtklayershell, menu_cache, libfm]

if libpulse.found()
  widget_sources += 'widgets/volume.cpp'
  deps += [libpulse, libgvc]
endif

executable('wf-panel', ['panel.cpp'] + widget_sources,
        dependencies: deps,
        install: true)
