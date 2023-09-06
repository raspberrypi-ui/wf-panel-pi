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

wayfire        = dependency('wayfire')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')
gtkmm          = dependency('gtkmm-3.0', version: '>=3.24')
wfconfig       = dependency('wf-config', version: '>=0.7.0') #TODO fallback submodule
gtklayershell  = dependency('gtk-layer-shell-0', version: '>= 0.1', fallback: ['gtk-layer-shell', 'gtk_layer_shell_dep'])
libpulse       = dependency('libpulse')
menu_cache     = dependency('libmenu-cache')
libfm          = dependency('libfm-gtk3')
netman	       = dependency('libnm')
nma	       	   = dependency('libnma')
secret	       = dependency('libsecret-1')
notify	       = dependency('libnotify')
packagekit     = dependency('packagekit-glib2')
udev           = dependency('libudev')

needs_libinotify = ['freebsd', 'dragonfly'].contains(host_machine.system())
libinotify       = dependency('libinotify', required: needs_libinotify)

add_project_arguments(['-Wno-pedantic', '-Wno-unused-parameter', '-Wno-parentheses'], language: 'cpp')

share_dir = join_paths(get_option('prefix'), 'share')
resource_dir = join_paths(share_dir, 'wf-panel-pi')
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
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : 'c')

subdir('metadata')
subdir('proto')
subdir('data')
subdir('src')
subdir('po')
