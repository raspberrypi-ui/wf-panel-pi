spacing_name = 'spacing'

spacing_sources = files(
  'spacing.cpp'
)

shared_module(spacing_name, spacing_sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wfpanel',
        include_directories : incdir
)
