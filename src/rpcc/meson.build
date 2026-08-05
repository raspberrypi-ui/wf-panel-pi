plugin_name = 'rpcc_wf-panel-pi'

sources = files (
    'notify_rpcc.c',
    'config_rpcc.c',
    'plugin.c',
   '../panel/plug_conf.c',
   '../panel/lxutils.c'
)

gtk = dependency ('gtk+-3.0')
gtkmm = dependency('gtkmm-3.0', version: '>=3.24')
evdev = dependency('libevdev')
libxml2 = dependency('libxml-2.0')
gtklayershell = dependency('gtk-layer-shell-0', version: '>= 0.1', fallback: ['gtk-layer-shell', 'gtk_layer_shell_dep'])
deps = [ gtk, gtkmm, evdev, libxml2, gtklayershell ]

proj_args = '-DPLUGIN_PATH="' + get_option ('prefix') + '/' + get_option('libdir') + '/wf-panel-pi/"'

shared_module(plugin_name, sources, dependencies: deps, install: true,
    install_dir: get_option('libdir') / 'rpcc',
    c_args : [ proj_args, '-DRPCC_DATA_DIR="' + presource_dir + '"', '-DPLUGIN_NAME="' + plugin_name + '"' ],
    cpp_args : proj_args,
    include_directories : incdir
)
