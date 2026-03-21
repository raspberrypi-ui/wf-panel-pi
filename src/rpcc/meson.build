sources = files (
    'notify_rpcc.c'
)

plugin_name = 'rpcc_notify'

gtk = dependency ('gtk+-3.0')
deps = [ gtk ]

shared_module(plugin_name, sources, dependencies: deps, install: true,
install_dir: get_option('libdir') / 'rpcc',
c_args : [ '-DRPCC_DATA_DIR="' + presource_dir + '"', '-DPLUGIN_NAME="' + plugin_name + '"' ]
)
