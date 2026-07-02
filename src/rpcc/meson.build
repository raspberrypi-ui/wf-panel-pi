plugin_name = 'rpcc_wf-panel-pi'

sources = files (
    'notify_rpcc.c',
    'configure.c',
    'plugin.c'
)

config_sources = files(
   '../panel/config/types.cpp',
   '../panel/config/option.cpp',
   '../panel/config/section.cpp',
   '../panel/config/log.cpp',
   '../panel/config/xml.cpp',
   '../panel/config/config-manager.cpp',
   '../panel/config/file.cpp',
   '../panel/config/duration.cpp',
   '../panel/config/compound-option.cpp',
   '../panel/lxutils.c'
)

gtk = dependency ('gtk+-3.0')
gtkmm = dependency('gtkmm-3.0', version: '>=3.24')
deps = [ gtk ]
evdev = dependency('libevdev')
libxml2 = dependency('libxml-2.0')
gtklayershell = dependency('gtk-layer-shell-0', version: '>= 0.1', fallback: ['gtk-layer-shell', 'gtk_layer_shell_dep'])

presource_dir = join_paths(share_dir, 'rpcc')
pui_dir = join_paths(presource_dir, 'ui')

incdir = include_directories('../api/wf-panel-pi')

proj_args = '-DPLUGIN_PATH="' + get_option ('prefix') + '/' + get_option('libdir') + '/wf-panel-pi/"'

shared_module(plugin_name, sources + config_sources, dependencies: [ gtk, gtkmm, evdev, libxml2, gtklayershell ], install: true,
    install_dir: get_option('libdir') / 'rpcc',
    c_args : [ proj_args, '-DRPCC_DATA_DIR="' + presource_dir + '"', '-DPLUGIN_NAME="' + plugin_name + '"' ],
    cpp_args : proj_args,
    include_directories : incdir
)

install_data('wf-panel-pi.ui', install_dir: pui_dir)
