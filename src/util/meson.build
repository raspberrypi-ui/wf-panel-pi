util_includes = ['.','../panel']

util = static_library('util', ['gtk-utils.cpp', 'wf-shell-app.cpp', 'wf-autohide-window.cpp', 'wf-popover.cpp', 'conf-utils.cpp', 'configure.c', 'notification.c'],
    dependencies: [wf_protos, wayland_client, gtkmm, wfconfig, libinotify, gtklayershell],
    include_directories: util_includes)

libutil = declare_dependency(
        link_with: util)
