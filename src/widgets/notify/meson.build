notify_name = 'notify'

notify_sources = files(
  'notify.cpp'
)

shared_module(notify_name, notify_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        include_directories : incdir
)
