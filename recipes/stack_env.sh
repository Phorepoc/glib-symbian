# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# stack_env.sh -- sourced by the prebuild recipes of the Hildon stack (glib-symbian, gtk-symbian, dbus-symbian, hildon-ui) in the
# toolchain container (the pdk's package.yml: PHOREPOC_BUILD / PHOREPOC_PDK / PHOREPOC_ADAPTATIONS set, the include tree exported).
#
# The stack's recipes use fixed paths: /h = the repository whose recipe runs, /glib /gtk /dbus = the sibling repositories, /wk = the
# shared work tree, /o = the archives. Here they are links: /wk -> $PHOREPOC_BUILD/hildon-stack/wk, /o -> $PHOREPOC_BUILD/hildon (what
# ".hildon/..." in the registries resolves to), /h -> a view of the repository with a writable dl/ (the repositories themselves are
# read-only mounts). Everything else comes from the build tree: Open C's headers ($PHOREPOC_BUILD/epoc32/include/stdapis), the x32
# libc/libm/libpthread/libdl images of the core layer ($PHOREPOC_BUILD/mmp-x32), cxxrt's libstdc++ tree, the pdk's pp-gcc wrappers.
: "${PHOREPOC_BUILD:?}" "${PHOREPOC_PDK:?}"
export ARCH=${PHOREPOC_ARCH:-x32} JOBS=${JOBS:-$(nproc)}
[ "$ARCH" = x32 ] || { echo "the Hildon stack is built for x32 only (ARCH=$ARCH)"; exit 1; }
STACK=$PHOREPOC_BUILD/hildon-stack
export STDAPIS=$PHOREPOC_BUILD/epoc32/include/stdapis MMP_X32=$PHOREPOC_BUILD/mmp-x32

# adaptation_dir NAME -- the checkout of a repository of the build (PHOREPOC_ADAPTATIONS; the names from PHOREPOC_ADAPTATION_NAMES
# when phe started the container, else the checkout's folder name)
adaptation_dir() {
  local repos names i n
  IFS=: read -r -a repos <<< "${PHOREPOC_ADAPTATIONS:-}"
  IFS=: read -r -a names <<< "${PHOREPOC_ADAPTATION_NAMES:-}"
  for i in "${!repos[@]}"; do
    n=${names[i]:-$(basename "${repos[i]}")}
    if [ "$n" = "$1" ]; then echo "${repos[i]}"; return 0; fi
  done
  echo "$1 is not in PHOREPOC_ADAPTATIONS" >&2
  return 1
}

stack_links() {
  mkdir -p "$STACK/wk/lsc" "$STACK/logs" "$STACK/stamps" "$PHOREPOC_BUILD/hildon"
  ln -sfn "$STACK/wk" /wk
  ln -sfn "$PHOREPOC_BUILD/hildon" /o
  local r d
  for r in glib gtk dbus; do
    if d=$(adaptation_dir "$r-symbian" 2>/dev/null); then ln -sfn "$d" "/$r"; fi
  done
}

# stack_use NAME -- point /h at repository NAME: a view of its top-level entries, with dl/ (the downloaded sources) in the build tree
stack_use() {
  local src v e
  src=$(adaptation_dir "$1"); v=$STACK/view/$1
  mkdir -p "$v" "$STACK/dl/$1"
  for e in "$src"/*; do [ "$(basename "$e")" = dl ] || ln -sfn "$e" "$v/$(basename "$e")"; done
  ln -sfn "$STACK/dl/$1" "$v/dl"
  ln -sfn "$v" /h
}

# stack_step REPO STEP COMMAND -- run COMMAND (a bash script line) once per build tree, its output in $STACK/logs/REPO-STEP.log
stack_step() {
  local stamp=$STACK/stamps/$1-$2 log=$STACK/logs/$1-$2.log
  if [ -f "$stamp" ]; then echo "== $1 $2 (done before)"; return 0; fi
  echo "== $1 $2"
  if bash -c "set -o pipefail; $3" > "$log" 2>&1; then
    tail -n 12 "$log"; touch "$stamp"
  else
    tail -n 80 "$log"; echo "$1 $2 FAILED (log: $log)"; return 1
  fi
}

# stack_need FILE... -- fail unless every archive was made
stack_need() {
  local f rc=0
  for f; do [ -s "/o/$f" ] || { echo "missing: \$PHOREPOC_BUILD/hildon/$f"; rc=1; }; done
  return $rc
}

# the GLib layer: cxxrt (its archives and libstdc++ tree), the pp-gcc wrappers, GLib + libffi + GIO -> /o/libphe{glib,gobject,...}.a
stack_glib() {
  stack_links
  local cx; cx=$(adaptation_dir cxxrt)
  stack_step cxxrt build "bash '$cx/build.sh'"
  # the libstdc++ header tree (HarfBuzz) and source (libsupc++) of cxxrt's x32 build, where the recipes look for them
  . "$cx/gcc.lock"
  ln -sfn "$PHOREPOC_BUILD/cxxrt-work/lsc-x32/build" /wk/lsc/build
  ln -sfn "$PHOREPOC_BUILD/cxxrt-work/lsc-x32/gcc-$GCC_VERSION" "/wk/lsc/gcc-$GCC_VERSION"
  stack_use glib-symbian
  stack_step glib fetch "bash /h/recipes/fetch.sh"
  stack_step glib tools "W=/wk/lsc bash '$PHOREPOC_PDK/tools/toolchain/mk_pure_wrappers.sh' && bash /h/recipes/libc_syms.sh"
  stack_step glib native "bash /h/recipes/native_config.sh && bash /h/recipes/native_build.sh"
  stack_step glib config "cd /wk/nat && cp -f config.h config.h.native && cp -f glib/glibconfig.h glibconfig.h.native && python3 /h/recipes/gen_config.py config.h.native glibconfig.h.native /wk/libc_syms.txt '$STDAPIS' /wk/x32cfg"
  stack_step glib objects "mkdir -p /wk/ffi-x32/include /wk/out && cp /wk/ffi-inst/include/*.h /wk/ffi-x32/include/ && bash /h/recipes/build_x32.sh && bash /h/recipes/build_ffi_x32.sh"
  stack_step glib gio "bash /h/recipes/build_gio_x32.sh"
  stack_step glib archives "bash /h/recipes/package_x32.sh"
  stack_need libpheglib.a libphegobject.a libphegmodule.a libphegthread.a libpheffi.a libphegio.a
}
