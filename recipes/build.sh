#!/usr/bin/env bash
# build.sh -- GLib 2.56.4 + libffi + GIO for the x32 PC image: from the pins to the archives. Host side, Git Bash, Docker Desktop up.
#
#   REPO=<Symbian-ReverseEngineered checkout>  bash recipes/build.sh [steps]
# steps (default: all, in order): fetch tools native config objects gio cxxhdr archives images
# The sibling repositories (glib-symbian, gtk-symbian, dbus-symbian, hildon-symbian) are expected next to this one; they are mounted at /glib /gtk /dbus (read-only) in every container,
# this repository at /h. The docker volume (VOL) is the shared build tree of the whole stack; OUTD (the archives) is $REPO/.phorepoc-x86-build/hildon.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd -W); REPO=${REPO:-C:/Users/memph/Git/Symbian-ReverseEngineered}; VOL=${VOL:-hildon-wk}; PARENT=${HERE%/*}; SIB="-v $PARENT/glib-symbian:/glib:ro -v $PARENT/gtk-symbian:/gtk:ro -v $PARENT/dbus-symbian:/dbus:ro"
STEPS=${*:-fetch tools native config objects gio cxxhdr archives images}
OUTD=$REPO/.phorepoc-x86-build/hildon; mkdir -p "$OUTD"
IMG=phorepoc-x86
dr() { MSYS_NO_PATHCONV=1 docker.exe run --rm --name "hildon-$1" -e JOBS="${JOBS:-8}" -v "$HERE":/h $SIB -v "$REPO":/repo:ro -v "$OUTD":/o -v "$VOL":/wk $IMG "${@:2}"; }
has() { case " $STEPS " in *" $1 "*) return 0;; esac; return 1; }
[ "${CLEAN:-0}" = 1 ] && { docker.exe volume rm "$VOL" >/dev/null 2>&1 || true; }
docker.exe volume create "$VOL" >/dev/null
has fetch    && { echo "== fetch";    bash "$HERE/recipes/fetch.sh"; }
has tools    && { echo "== tools";    MSYS_NO_PATHCONV=1 docker.exe run --rm --name hildon-w1 -v "$HERE":/h $SIB -v "$REPO":/repo -v "$VOL":/wk $IMG bash -c 'W=/wk/lsc bash /repo/tools/phorepoc-x86/cxxrt/mk_pure_wrappers.sh >/dev/null; grep -q nostdinc /wk/lsc/bin/pp-gcc || { echo "pp-gcc wrapper not written"; exit 1; }; bash /h/recipes/libc_syms.sh'; }
has native   && { echo "== native";   dr n1 bash -c 'bash /h/recipes/native_config.sh && bash /h/recipes/native_build.sh'; }
has config   && { echo "== config";   dr c1 bash -c 'cd /wk/nat && cp -f config.h config.h.native; cp -f glib/glibconfig.h glibconfig.h.native; python3 /h/recipes/gen_config.py config.h.native glibconfig.h.native /wk/libc_syms.txt /repo/.epoc32/include/stdapis /wk/x32cfg | cut -c1-80'; }
has objects  && { echo "== objects";  dr o1 bash -c 'mkdir -p /wk/ffi-x32/include /wk/out; cp /wk/ffi-inst/include/*.h /wk/ffi-x32/include/; bash /h/recipes/build_x32.sh && bash /h/recipes/build_ffi_x32.sh'; }
has gio      && { echo "== gio";      dr g1 bash /h/recipes/build_gio_x32.sh | tail -6; }
has cxxhdr   && { echo "== cxxhdr";   MSYS_NO_PATHCONV=1 docker.exe run --rm --name hildon-x1 -v "$HERE":/h $SIB -v "$REPO":/repo -v "$VOL":/wk $IMG bash /h/recipes/build_cxxheaders_x32.sh | tail -3; }
has archives && { echo "== archives"; dr a1 bash /h/recipes/package_x32.sh; }
has images   && { echo "== images";   ( cd "$REPO" && ./ph build pheglib phegobject phegmodule phegthread phegio ) | tail -3; }
echo "build.sh: done ($STEPS)"
