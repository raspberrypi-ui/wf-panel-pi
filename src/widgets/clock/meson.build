clock_name = 'clock'

clock_sources = files(
  'clock.cpp',
  'clock.c'
)

shared_module(clock_name, clock_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + clock_name + '"',
        include_directories : incdir
)
