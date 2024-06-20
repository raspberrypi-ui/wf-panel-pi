name = 'cpu'

sources = files(
  'cpu.cpp',
  'cpu.c'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
