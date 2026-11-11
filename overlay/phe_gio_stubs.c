/* Copyright The Phorepoc Authors */
/* SPDX-License-Identifier: BSD-2-Clause */
/* phe_gio_stubs.c -- names GIO calls that neither Open C nor pheglib exports to it (they were link stubs, which panic the caller):
 * ntohl, ngettext (a message lookup with no catalogs: the msgid, English plural rule), the resolver's SRV/MX record calls (no
 * res_query in Open C: they fail, GResolver reports the error) and get_mtab_monitor_file (no mount table: NULL, which GIO handles).
 * Archived into libphegio.a with phe_posix_stubs.o (fork: ENOSYS for GSubprocess). */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

uint32_t ntohl (uint32_t v) { return __builtin_bswap32 (v); }

char *ngettext (const char *msgid, const char *plural, unsigned long n) { return (char *) (n == 1 ? msgid : plural); }

int __res_query (const char *dname, int klass, int type, unsigned char *answer, int anslen)
{
  (void) dname; (void) klass; (void) type; (void) answer; (void) anslen;
  errno = ENOSYS;
  return -1;
}

int __dn_expand (const unsigned char *msg, const unsigned char *eom, const unsigned char *src, char *dst, int dstsiz)
{
  (void) msg; (void) eom; (void) src; (void) dst; (void) dstsiz;
  return -1;
}

const char *get_mtab_monitor_file (void) { return NULL; }
