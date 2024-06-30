name = 'notify'

sources = files(
  'notify.cpp'
)

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        include_directories : incdir
)

install_data('notify.xml', install_dir: metadata_dir)
