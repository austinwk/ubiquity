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

## Complaints

Python isn't typed and the variables aren't always named in a way that makes the value's class clear.

Widgets from the glade xml are added as attributes/properties to the wizard via a loop using each widget's id. It would have been easier to read if they were added explicitly. This happens in ubiquity/frontend/gtk_ui.Wizard.__init__

```py
for builder in self.builders:
    for widget in builder.get_objects(): #A: 101 objects (buttons, etc)
        add_widget(self, widget) #A: Each object id is added as an attribute/property
        if isinstance(widget, Gtk.Window):
            self.toplevels.add(widget)
```

## Flow

```txt
bin/ubiquity.main()
|
|  install() -> bin/ubiquity.install()
|  |
|  |  wizard.run() -> ubiquity/frontend/gtk_ui.Wizard.run()
|  |  |
|  |  +  for each component (referred to as a page... really, each plugin)
|  |  |  page.filter_class(self, ui=ui) -> ubiquity/filteredcommand.FilteredCommand.__init__()
|  |  |
|  |  |  GLib.idle_add(lambda: self.dbfilter.start(auto_process=True)) -> ubiquity/filteredcommand.FilteredCommand.start()
|  |  |  |
|  |  |  |  self.prepare() -> ubiquity/plugins/<plugin>.Page.prepare()
|  |  |  |  |
|  |  |  |  |  TODO
|  |  |  |
|  |  |  |  self.dbfilter = DebconfFilter(self.db, widgets, self.is_automatic) -> ubiquity/debconffilter.DebconfFilter()
|  |  |  |  self.dbfilter.start(self.command, blocking=False, extra_env=env) -> ubiquity/debconffilter.DebconfFilter.start()
|  |  |  |  |
|  |  |  |  |  TODO
|  |  |  |
|  |  |  |  self.frontend.watch_debconf_fd(self.dbfilter.subout_fd ,self.process_input)
|  |  |  |  |
|  |  |  |  |TODO
|  |  |
|  |  |  Gtk.main()
|  |  |
-------- Wait for GUI event --------
|  |  |
|  |  E  [gui/gtk/ubiquity.ui] on_next_clicked -> ubiquity/frontend/gtk_ui.Wizard.on_next_clicked()
|  |  |  |
|  |  |  |


```

```txt
/usr/lib/ubiquity/bin/ubiquity<module>()
main(oem_config)

/usr/lib/ubiquity/bin/ubiquity main()
install(args[0], query=options.query)

/usr/lib/ubiquity/bin/ubiquity install()
ret = wizard.run()

/usr/lib/ubiquity/ubiquity/frontend/gtk_ui.py run()
Gtk.main()

/usr/lib/python3/dist-packages/gi/overrides/Gtk.py main()
return _Gtk_main(*args, **kwargs)

/usr/lib/python3/dist-packages/gi/overrides/GLib.py <lambda>()
func fdtransform = lambda, cond, *data: callback(channel, cond *data)

/usr/lib/ubiquity/ubiquity/frontend/gtk_ui.py watch_debconf_fd_helper()
return callback(source, debconf_condition)

/usr/lib/ubiquity/ubiquity/filteredcommand.py process_input()
if not self.process_line():

/usr/lib/ubiquity/ubiquity/filteredcommand.py process_line()
return self.dbfilter.process_line()

/usr/lib/ubiquity/ubiquity/debconffilter.py process_line()
if not input_widgets[0].run(priority, question):

/usr/lib/ubiquity/plugins/ubi-language.py run()
return plugin.Plugin.run(self, priority, question)

/usr/lib/ubiquity/ubiquity/filteredcommand.py run()
self.enter_ui_loop()

/usr/lib/ubiquity/ubiquity/filteredcommand.py enter_ui_loop()
self.frontend.run_main_loop()

/usr/lib/ubiquity/frontend/gtk_ui.py run_main_loop()
Gtk.main()

/usr/lib/python3/dist-packes/gi/overrides/Gtk.py main()
return _Gtk_main(*args, **kwargs)

/usr/lib/ubiquity/ubiquity/frontend/gtk_ui.py on_next_clicked()
breakpoint()
```

## Idle -> FilteredCommand.start

wizard.run: page = ubi-language

GUI displays, page is blank

wizard.run: Gtk.main()

IDLE -> FilteredCommand.start

ubi-language page displays

Click Continue button

wizard.run: page = ubi-console-setup

IDLE -> FilteredCommand.start

ubi-console-setup page displays

Click Continue button

wizard.run: page = ubi-prepare

IDLE -> FilteredCommand.start

ubi-prepare page displays

Click Continue button

wizard.run: page = ubi-partman

IDLE -> FilteredCommand.start

ubi-partman page displays

...repeat

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

Look into d-i partman, debian installer partman, partman auto, partman recipes. Info seems limited. [Here is a sample](https://salsa.debian.org/installer-team/debian-installer/-/blob/master/doc/devel/partman-auto-recipe.txt). This is used in ubi-partman, which seems to interact using strings like 'choose_partition' and 'active_partition'. It all seems esoteric.

## debconf

* capb
* command
* fget
* fset
* get
* go
* info
* input
* metaget
* progress
* register
* reset
* set
* settitle
* shutdown
* subst

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
