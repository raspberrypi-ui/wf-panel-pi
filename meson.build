project(
    'wf-panel-pi',
    'c',
    'cpp',
    version: '0.8.0',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: [
        'cpp_std=c++17',
        'c_std=c11',
        'warning_level=2',
        'werror=false',
    ],
)

add_project_arguments(['-Wno-pedantic', '-Wno-unused-parameter', '-Wno-parentheses'], language: 'cpp')

share_dir = join_paths(get_option('prefix'), 'share')
resource_dir = join_paths(share_dir, 'wf-panel-pi')
applications_dir = join_paths(share_dir, 'applications')
metadata_dir = join_paths(resource_dir, 'metadata')
ui_dir = join_paths(resource_dir, 'ui')
icon_dir = join_paths(resource_dir, 'icons')
sysconf_dir = join_paths(get_option('prefix'), get_option('sysconfdir'))

gettext_package = meson.project_name()

add_project_arguments('-DICONDIR="' + icon_dir + '"', language : 'cpp')
add_project_arguments('-DMETADATA_DIR="' + metadata_dir + '"', language : 'cpp')
add_project_arguments('-DSYSCONF_DIR="' + sysconf_dir + '"', language : 'cpp')
add_project_arguments('-DRESOURCE_DIR="' + resource_dir + '"', language : 'cpp')
add_project_arguments('-DPACKAGE_DATA_DIR="' + resource_dir + '"', language : 'c')
add_project_arguments('-DGETTEXT_PACKAGE="' + gettext_package + '"', language : 'c')
add_project_arguments('-DGETTEXT_PACKAGE="' + gettext_package + '"', language : 'cpp')
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : 'c')
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : 'cpp')

wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')

subdir('proto')

gtkmm          = dependency('gtkmm-3.0', version: '>=3.24')
deps = [ wf_protos, gtkmm ]

subdir('metadata')
subdir('data')
subdir('src')
subdir('po')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    version:      meson.project_version(),
    filebase:     meson.project_name(),
    name:         meson.project_name(),
    description: 'Panel for Raspberry Pi Wayland desktop',
    requires:     gtkmm,
    subdirs:     'wf-panel-pi',
    variables:    ['metadatadir=' + metadata_dir,
                   'uidir=' + ui_dir,
                   'sysconfdir=' + sysconf_dir,
                   'plugindir=${libdir}/wf-panel-pi',
                   'icondir=' + icon_dir]
    )
