gpu_name = 'gpu'

gpu_sources = files(
  'gpu.cpp',
  'gpu.c'
)

shared_module(gpu_name, gpu_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + gpu_name + '"',
        include_directories : incdir
)
