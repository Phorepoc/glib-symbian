#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
# package_x32.sh -- IN phorepoc-x86 (/wk, repo:/repo:ro, this repo:/h, glib-symbian:/glib, gtk-symbian:/gtk, dbus-symbian:/dbus, output dir :/o): archive the GLib objects + libffi + GIO into /o/{libpheglib,libphegobject,libphegmodule,libphegthread,libpheffi,libphegio}.a
set -u
CC=/wk/lsc/bin/pp-gcc; X=/wk/x32; S=/wk/glib-2.56.4; O=/o; mkdir -p $O /wk/out/t
INC="-I$X -I$X/glib -I$S -I$S/glib -I$S/gobject -I$S/gmodule -I/wk/ffi-x32 -I/h/overlay -include /h/overlay/phe_glib_compat.h"
CF="-O2 -g0 -w -fno-pic -DHAVE_CONFIG_H -DGLIB_STATIC_COMPILATION -DGOBJECT_STATIC_COMPILATION"
$CC $CF $INC -c /h/overlay/phe_posix_stubs.c -o /wk/out/phe_posix_stubs.o || exit 1
$CC $CF -c /h/overlay/phe_gio_stubs.c -o /wk/out/phe_gio_stubs.o || exit 1
mk() { local a=$1; shift; rm -f $O/$a; ar rcs $O/$a "$@"; echo "$a: $(ar t $O/$a | wc -l) members"; }
mk libpheglib.a /wk/out/phe_posix_stubs.o $X/glib/.libs/*.o $X/glib/deprecated/.libs/*.o $X/glib/libcharset/.libs/*.o $X/glib/gnulib/.libs/*.o $X/glib/pcre/.libs/*.o
mk libphegobject.a $X/gobject/.libs/*.o
mk libphegmodule.a $X/gmodule/.libs/*.o
mk libphegthread.a $X/gthread/.libs/*.o
mk libpheffi.a $(ls /wk/out/ffi/*.o | grep -v win64)
[ -d $X/gio/.libs ] && mk libphegio.a /wk/out/phe_gio_stubs.o /wk/out/phe_posix_stubs.o $X/gio/.libs/*.o $X/gio/xdgmime/.libs/*.o $X/gio/gvdb/.libs/*.o
true
# the GModule backend test and the symbol tables (gen_symtab.py: resource/gmodule/<dll>.sym of every DLL of this repository + the test plug-in)
mkdir -p /wk/out/gm $O/gmodule
$CC $CF $INC -c /h/os/src/phegmoduletest/phegmoduletest_main.c -o /wk/out/gm/phegmoduletest_main.o || echo "GMODULE TEST COMPILE FAIL"
rm -f $O/libphegmoduletest.a; ar rcs $O/libphegmoduletest.a /wk/out/gm/phegmoduletest_main.o; echo "libphegmoduletest.a built"
for d in /h/os/src/*/*.def; do n=$(basename $d .def); python3 /h/recipes/gen_symtab.py $d $O/gmodule/$n.sym > /dev/null; done; echo "symbol tables: $(ls $O/gmodule | wc -l)"
# the GIO file monitor test
$CC $CF $INC -I$S/gio -I$X/gio -c /h/os/src/phegiotest/phegiotest_main.c -o /wk/out/gm/phegiotest_main.o || echo "GIO TEST COMPILE FAIL"
rm -f $O/libphegiotest.a; ar rcs $O/libphegiotest.a /wk/out/gm/phegiotest_main.o; echo "libphegiotest.a built"
true
