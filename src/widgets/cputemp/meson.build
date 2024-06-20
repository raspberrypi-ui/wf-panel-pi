cputemp_name = 'cputemp'

cputemp_sources = files(
  'cputemp.cpp',
  'cputemp.c'
)

shared_module('cputemp', cputemp_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + cputemp_name + '"',
        include_directories : incdir
)
