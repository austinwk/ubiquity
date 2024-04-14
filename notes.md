# Notes

Execution

1. Double clicking the desktop icon runs: `sudo --preserve-env=DBUS_SESSION_BUS_ADDRESS,XDG_DATA_DIRS,XDG_RUNTIME_DIR sh -c 'WEBKIT_DISABLE_COMPOSITING_MODE=1 ubiquity gtk_ui'`
2. Which executes: `/usr/bin/ubiquity` (src: [bin/ubiquity-wrapper](bin/ubiquity-wrapper))
3. Which calls `/usr/lib/ubiquity/bin/ubiquity` (src: [bin/ubiquity](bin/ubiquity))

`sudo --preserve-env=list` Sudo excludes many environment variables. This keeps the listed ones.

* `DBUS_SESSION_BUS_ADDRESS`: unix:path=/run/user/999/bus
* `XDG_DATA_DIRS`: /usr/share/cinnamon:/usr/share/gnome:/home/mint/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share
* `XDG_RUNTIME_DIR`: /run/user/999

`debconf` package is `/lib/python3/dist-packages/debconf.py`

`Debconf.get(...)` and `Debconf.set(...)` (and others) are added by `Debconf.setCommand(...)`

## Environment

```json
{
    // Initial
    "SUDO_GID": "999",
    "MAIL": "/var/mail/root",
    "USER": "root",
    "HOME": "/root",
    "DBUS_SESSION_BUS_ADDRESS": "unix:path=/run/user/999/bus",
    "SUDO_UID": "999",
    "LOGNAME": "root",
    "TERM": "unknown",
    "PATH": "/usr/local/sbin:/user/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin",
    "XDG_RUNTIME_DIR": "/run/user/999",
    "DISPLAY": ":0",
    "LANG": "C.UTF-8",
    "XDG_CURRENT_DESKTOP": "X-Cinnamon",
    "WEBKIT_DISABLE_COMPOSITING_MODE": "1",
    "XAUTHORITY": "/home/mint/.Xauthority",
    "SUDO_COMMAND": "/usr/bin/sh -c WEBKIT_DISABLE_COMPOSITING_MODE=1 ubiquity gtk_ui",
    "SHELL": "/bin/bash",
    "SUDO_USER": "mint",
    "PWD": "/home/mint",
    "XDG_DATA_DIRS": "/usr/share/cinnamon:/usr/share/gnome:/home/mint/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share",

    // bin/ubiquity.force_utf8_locale()
    "LC_NUMERIC": "C.UTF-8",

    // bin/ubiquity.install()
    "UBIQUITY_FRONTEND": "gtk_ui",

    // ubiquity/frontend/base.BaseFrontend.__init__()
    "SCIM_USER": "SUDO_USER",
    "SCIM_HOME": "/home/mint",

    // ubiquity/i18n.reset_locale()
    "LANG": "en_US.UTF-8", // Changed from above
    "LANGUAGE": "en_US.UTF-8",
}
```

## Questions

When is [debian/ubiquity.templates](debian/ubiquity.templates) applied? During the build process?
