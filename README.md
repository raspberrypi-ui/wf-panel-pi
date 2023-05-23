# wf-panel-pi

A panel for the Raspberry Pi Wayfire desktop, based on wf-panel from the wf-shell project

# Build

```
meson setup builddir --prefix=/usr --libdir=/usr/lib/<library-location>
cd builddir
meson compile
sudo meson install
```
On a 32-bit system, <library-location> should be "arm-linux-gnueabihf".
On a 64-bit system, <library-location> should be "aarch64-linux-gnu".


# Configuration

wf-panel-pi uses a config file located (by default) in `~/.config/wf-panel-pi.ini`

