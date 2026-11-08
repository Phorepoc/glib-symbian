> GLib 2.56, GObject, GModule, GThread and GIO: the C library that GTK, D-Bus and Hildon programs build on.

GLib 2.56.4 for Phorepoc OS (libglib, libgobject with libffi, libgmodule, libgthread and libgio), the base library of GTK, D-Bus and Hildon programs. GModule loads plug-in DLLs by name; GIO has a file monitor on the file server's change notification. It installs in `\sys\bin\` and `\resource\gmodule\` of the install drive. Nothing in the system needs it: install it before GTK, D-Bus or Hildon.

The libraries are LGPL and the like, kept in DLLs of their own that you can replace. The package installs every licence text and the source offer in `\resource\glib-symbian\licences\`. Needs: none beyond the system.
