/* Copyright The Phorepoc Authors */
/* SPDX-License-Identifier: BSD-2-Clause */
/* phe_respath.h -- where an installed package's resources are. BSD-2-Clause (Phorepoc).
 * A package installed through the Catalogue puts its resources (\resource\...) on its install drive (C:, E:, ...), while the same files of a ROM build are on Z:. A program asks
 * phe_res ("resource/dbus/session.conf") and gets "<drive>:/resource/dbus/session.conf" of the first drive that has the file (installed copies first, the ROM last).
 * If no drive has it, the Z: path is returned (so the error message names it). The answer lives in a small ring of static buffers. */
#ifndef PHE_RESPATH_H
#define PHE_RESPATH_H
#include <stdio.h>
#include <unistd.h>

static inline const char *
phe_res (const char *rel)
{
  static char ring[8][160]; static int next;
  static const char drives[] = "cedfz";
  char *buf = ring[next++ & 7]; int i;
  for (i = 0; drives[i]; i++) {
    snprintf (buf, 160, "%c:/%s", drives[i], rel);
    if (access (buf, F_OK) == 0) return buf;
  }
  snprintf (buf, 160, "z:/%s", rel);
  return buf;
}
#endif
