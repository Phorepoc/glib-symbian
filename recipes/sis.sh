#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# sis.sh -- the pdk's package.yml runs it after the components: GLib as $PHOREPOC_BUILD/sis/glib-symbian-<arch>.sis (the DLLs, their resources, the licence texts and the
# source offer in \resource\glib-symbian\licences). The package needs: nothing of ours (installed first, by UID and version).
# Installs through the Catalogue; no image carries it. Resources are found by the programs on whatever drive it was installed to (glib-symbian overlay/phe_respath.h).
set -eu
ARCH=${PHOREPOC_ARCH:-x32}
REPO=$(cd "$(dirname "$0")/.." && pwd)
LIC=
for f in "$REPO"/LICENSES/* "$REPO/LICENSE" "$REPO/NOTICE" "$REPO/SOURCE-OFFER.txt"; do
  LIC="$LIC --file $f=!:/resource/glib-symbian/licences/$(basename "$f")"
done
python3 "$PHOREPOC_PDK/tools/make_sis.py" pheglib phegobject phegmodule phegthread phegio --unsigned --version "${PHOREPOC_PACKAGE_VERSION:-1.0.0}" \
  --name "GLib" --vendor Phorepoc $LIC \
  --out "$PHOREPOC_BUILD/sis/glib-symbian-$ARCH.sis"
