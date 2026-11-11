#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# prebuild.sh -- the pdk's package.yml runs it in the toolchain container before the components: cxxrt's archives (the DLLs link its
# libphcompat + libgcc), then GLib 2.56.4 + libffi + GIO as the archives under $PHOREPOC_BUILD/hildon/ that registry/ links
# (recipes/stack_env.sh). The prebuilds of gtk-symbian, dbus-symbian and hildon-ui run this part first.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/recipes/stack_env.sh"
stack_glib
