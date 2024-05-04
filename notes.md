# Notes

[PyGObject API](https://lazka.github.io/pgi-docs/)

Execution

1. Double clicking the desktop icon runs: `sudo --preserve-env=DBUS_SESSION_BUS_ADDRESS,XDG_DATA_DIRS,XDG_RUNTIME_DIR sh -c 'WEBKIT_DISABLE_COMPOSITING_MODE=1 ubiquity gtk_ui'` (-c tells `sh` to read commands from a string)
2. Which executes: `/usr/bin/ubiquity` (src: [bin/ubiquity-wrapper](bin/ubiquity-wrapper))
3. Which calls `/usr/lib/ubiquity/bin/ubiquity` (src: [bin/ubiquity](bin/ubiquity))

`sudo --preserve-env=list` Sudo excludes many environment variables. This keeps the listed ones.

* `DBUS_SESSION_BUS_ADDRESS`: unix:path=/run/user/999/bus
* `XDG_DATA_DIRS`: /usr/share/cinnamon:/usr/share/gnome:/home/mint/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share
* `XDG_RUNTIME_DIR`: /run/user/999

`debconf` package is `/lib/python3/dist-packages/debconf.py`

`Debconf.get(...)` and `Debconf.set(...)` (and others) are added by `Debconf.setCommand(...)`

## Debugging

Any `breakpoint()` after the following will not be accessible in the terminal.

/usr/lib/python3/dist-packages/debconf.py

```python
class Debconf:
    # ...
    def __init__(self, title=None, read=None, write=None, run_frontend=False):
        for command in ('capb set reset title input beginblock endblock go get'
                        ' register unregister subst fset fget previous_module'
                        ' visible purge metaget exist version settitle'
                        ' info progress data').split():
            self.setCommand(command)
        self.read = read or sys.stdin
        self.write = write or sys.stdout
        sys.stdout = sys.stderr                      # <---
        if run_frontend:
            runFrontEnd()
        self.setUp(title)
```

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

    // ubiquity/frontend/gtk_ui.py (on import)
    "UBIQUITY_GLADE": "/usr/share/ubiquity/gtk"
}
```

## Questions

When is [debian/ubiquity.templates](debian/ubiquity.templates) applied? During the build process?

## GUI

started by calling ubiquity/frontend/gtk_ui.Wizard('linuxmint')

gtk_ui.Wizard -> base.BaseFrontend

plugin.PageGtk -> PageBase

Wizard (class)

* Inherits from BaseFrontend
* Manages Gtk.Builder
  * Manages the GtkWindow (root ui), which is in gui/gtk/ubiquity.ui

BaseFrontend (class)

* Manages debconf-communicate via DebconfCommunicator. DebconfCommunicator inherits from debconf.Debconf, which is an external library.
* Loads and orders ubiquity/plugins via plugin_manager. Plugins are modules. Each module represents a step in the installation: language, partitioning, user, etc

Plugins (modules in ubiquity/plugins)

* All plugins (except ubi_wireless) have a Page and a PageGtk class.
* All plugins (except ubi_wireless) are wrapped in a Component that exposes their Page, PageGtk, classes.
* All plugins have a `plugin_widgets` property
  * At least has one reference to `self.page`

PageGtk (class in all but one plugin)

* Inherits from PageBase (which is defined in each plugin)

PageBase (class in all but one plugin)

* Inherits from plugin.PluginUI

PluginUI (class)

* Inherits from UntrustedBase

## Plugins (sorted as in BaseFrontend)

ubi-language

* `page`: Gtk.Builder of `stepLanguage.ui`
* `plugin_widgets`: `page`
* `plugin_optional_widgets`: -

ubi-console-setup

* `page`: Gtk.Builder of `stepKeyboardConf.ui`
* `plugin_widgets`: `page`
* `plugin_optional_widgets`: -

ubi-wireless

* `page`: Gtk.Builder of `stepWireless.ui`
* `plugin_widgets`: `page`
* `plugin_optional_widgets`: -

ubi-prepare

* `page`: Uses something different
* `plugin_widgets`: stepPrepare
* `plugin_optional_widgets`: stepNoSpace, stepRST

ubi-partman

* `page`: Uses something different
* `plugin_widgets`: stepPartAsk
* `plugin_optional_widgets`: stepPartAuto, stepPartAdvanced, stepPartCrypto, stepBitlocker
* `plugin_is_install`: True

ubi-timezone

* `page`: Gtk.Builder of `stepLocation.ui`
* `plugin_widgets`: `page`
* `plugin_optional_widgets`: -

ubi-usersetup

* `page`: Gtk.Builder of `stepUserInfo.ui`
* `plugin_widgets`: `page`
* `optional_widgets`: -

ubi-network

Note: "network component only usable with debconf frontend"
