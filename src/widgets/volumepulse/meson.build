volumepulse_name = 'volumepulse'

volumepulse_sources = files(
  'volumepulse.cpp',
  'volumepulse.c',
  'micpulse.c',
  'commongui.c',
  'pulse.c',
  'bluetooth.c'
)

shared_module(volumepulse_name, volumepulse_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + volumepulse_name + '"',
        include_directories : incdir
)
