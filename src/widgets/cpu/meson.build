cpu_name = 'cpu'

cpu_sources = files(
  'cpu.cpp',
  'cpu.c'
)

shared_module(cpu_name, cpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cpu_name + '"',
        include_directories : incdir
)
