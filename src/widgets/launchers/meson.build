name = 'launchers'

sources = files(
  'launchers.c',
  'launchers.cpp'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : '-DPLUGIN_NAME="' + name + '"',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
