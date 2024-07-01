name = 'updater'

sources = files(
  'updater.cpp',
  'updater.c'
)

packagekit = dependency('packagekit-glib2')
module_deps = [ packagekit ]

metadata = files(
  name + '.xml'
)

shared_module(name, sources,
        dependencies: deps + module_deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(metadata, install_dir: metadata_dir)
