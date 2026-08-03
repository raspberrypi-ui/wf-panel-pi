name = 'launchers'

sources = files(
  'launchers.c',
  'launchers.cpp'
)

module_deps = [ ]

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : '-DPLUGIN_NAME="' + name + '"',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
