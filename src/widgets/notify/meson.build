name = 'notify'

sources = files(
  'notify.cpp'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        include_directories : incdir
)
