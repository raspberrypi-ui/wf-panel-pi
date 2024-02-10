# wf-panel-pi (kbdlayout branch)

A panel for the Raspberry Pi Wayfire desktop, based on wf-panel from the wf-shell project

# Build

```
First, make sure that you have all required tools and dependencies installed:
```
sudo apt install \
  meson ninja-build wayfire-dev libwayland-dev wayland-protocols \
  libgtkmm-3.0-dev libwf-config-dev libgtk-layer-shell-dev \
  libpulse-dev libmenu-cache-dev libfm-gtk-dev libnm-dev libnma-dev \
  libsecret-1-dev libnotify-dev libpackagekit-glib2-dev libudev-dev \
  libdbusmenu-gtk3-dev libxkbcommon-dev libwlroots-dev

(that should do it for a base desktop version of pi bookworm, if you
encounter missing tools in the build, you should add the required
packages accordingly)


```
Then, do the actual build:

```
meson setup builddir --prefix=/usr --libdir=/usr/lib/<library-location>
cd builddir
meson compile
sudo meson install
```
On a 32-bit system, `<library-location>` should be `arm-linux-gnueabihf`.
On a 64-bit system, `<library-location>` should be `aarch64-linux-gnu`.

```
Note that this overwrites files from the installed package wf-panel-pi. If
a newer version of wf-panel-pi comes up, your changes will be lost. You
might prefer to uninstall the distributed wf-panel-pi first to avoid this.


# Configuration

wf-panel-pi uses a config file located (by default) in `~/.config/wf-panel-pi.ini`
The default configuration may not include KbdLayout, but you can add it manuall
by right-clicking on the taskbar and selecting "Preferences". Note that for
KbdLayout to work, the kbdd plugin (installed with this build) must be enabled
in your $HOME/.config/wayfire.ini. A nice idea is to use the `wcm` tool (which,
AFAIK, is not in the standard repo, so you have to build it from source) to get
a decent wayfire.ini and then add the kbdd plugin to the already-enabled ones.
