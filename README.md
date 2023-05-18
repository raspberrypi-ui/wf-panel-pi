# wf-panel-pi

A panel for the Raspberry Pi Wayfire desktop, based on wf-panel from the wf-shell project

# Build

Assuming a 64-bit system:

```
meson setup builddir --prefix=/usr --libdir=/usr/lib/aarch64-linux-gnu
cd builddir
meson compile
sudo meson install
```

# Configuration

wf-panel-pi uses a config file located (by default) in `~/.config/wf-panel-pi.ini`

