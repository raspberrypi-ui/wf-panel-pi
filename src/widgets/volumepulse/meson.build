name = 'volumepulse'

sources = files(
  'volumepulse.cpp',
  'volumepulse.c',
  'micpulse.c',
  'commongui.c',
  'pulse.c',
  'bluetooth.c'
)

libpulse = dependency('libpulse')
module_deps = [ libpulse ]

metadata = files()

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(metadata, install_dir: metadata_dir)
