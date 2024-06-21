install_subdir('api/wf-panel-pi',
    install_dir: get_option('includedir'))

incdir = include_directories('api/wf-panel-pi')

subdir('panel')
subdir('widgets')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  version: meson.project_version(),
  name: meson.project_name(),
  description: 'Raspberry Pi GTK3 panel for Wayfire',
  variables: [
    'metadatadir=' + metadata_dir,
    'sysconfdir=' + sysconf_dir,
  ],
)
