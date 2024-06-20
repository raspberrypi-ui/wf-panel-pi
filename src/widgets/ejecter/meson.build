ejecter_name = 'ejecter'

ejecter_sources = files(
  'ejecter.cpp',
  'ejecter.c'
)

shared_module(ejecter_name, ejecter_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        cpp_args : '-DPLUGIN_NAME="' + ejecter_name + '"',
        include_directories : incdir
)
