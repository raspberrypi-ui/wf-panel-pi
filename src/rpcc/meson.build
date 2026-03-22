plugin_name = 'rpcc_notify'

sources = files (
    'notify_rpcc.c'
)

gtk = dependency ('gtk+-3.0')
deps = [ gtk ]

presource_dir = join_paths(share_dir, 'rpcc')
pui_dir = join_paths(presource_dir, 'ui')

shared_module(plugin_name, sources, dependencies: deps, install: true,
install_dir: get_option('libdir') / 'rpcc',
c_args : [ '-DRPCC_DATA_DIR="' + presource_dir + '"', '-DPLUGIN_NAME="' + plugin_name + '"' ]
)

install_data('notify.ui', install_dir: pui_dir)
