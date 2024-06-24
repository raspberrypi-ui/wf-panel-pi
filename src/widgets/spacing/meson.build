name = 'spacing'

sources = files(
  'spacing.cpp'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        include_directories : incdir
)
