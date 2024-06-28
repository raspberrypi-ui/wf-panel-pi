name = 'volumepulse'

sources = files(
  'volumepulse.cpp',
  'volumepulse.c',
  'micpulse.c',
  'commongui.c',
  'pulse.c',
  'bluetooth.c'
)

libpulse       = dependency('libpulse')
volumepulse_deps = [ libpulse ]

shared_module(name, sources,
        dependencies: deps + volumepulse_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)
