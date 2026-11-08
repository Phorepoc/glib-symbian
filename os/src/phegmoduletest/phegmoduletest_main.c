/* phegmoduletest_main.c -- GModule on this system: open a plug-in DLL, look its exports up by NAME, call them, fail cleanly for a missing symbol / DLL, close. BSD-2-Clause (Phorepoc). */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gmodule.h>

extern void phe_log (const char *msg);
#define LOG(...) do { char m_[300]; g_snprintf (m_, sizeof m_, __VA_ARGS__); phe_log (m_); } while (0)
static int ok_n, fail_n;
#define CHECK(c, what) do { if (c) { ok_n++; LOG ("[phegmoduletest] ok: %s", what); } else { fail_n++; LOG ("[phegmoduletest] FAIL: %s", what); } } while (0)

int
phe_gmodule_test_main (void)
{
  GModule *m; gpointer p = NULL;
  CHECK (g_module_supported (), "g_module_supported");
  m = g_module_open ("z:/sys/bin/phegmoduleplugin.so", G_MODULE_BIND_LAZY);
  LOG ("[phegmoduletest] g_module_open(phegmoduleplugin.so): %s (%s)", m ? "ok" : "failed", m ? g_module_name (m) : g_module_error ());
  CHECK (m != NULL, "open a plug-in DLL");
  if (m) {
    int (*add) (int, int) = NULL; const char *(*name) (void) = NULL;
    CHECK (g_module_symbol (m, "plugin_add", &p), "symbol plugin_add by name"); add = (int (*) (int, int)) p;
    CHECK (add && add (40, 2) == 42, "plugin_add (40, 2) == 42");
    p = NULL;
    CHECK (g_module_symbol (m, "plugin_name", &p), "symbol plugin_name by name"); name = (const char *(*) (void)) p;
    CHECK (name && !strcmp (name (), "phegmoduleplugin"), "plugin_name () == phegmoduleplugin");
    p = NULL;
    CHECK (!g_module_symbol (m, "no_such_symbol", &p), "an unknown symbol is not found");
    LOG ("[phegmoduletest] error text: %s", g_module_error ());
    CHECK (g_module_close (m), "close");
  }
  m = g_module_open ("z:/sys/bin/phe_no_such_plugin.so", G_MODULE_BIND_LAZY);
  CHECK (m == NULL, "opening a missing DLL fails");
  LOG ("[phegmoduletest] error text: %s", g_module_error ());
  m = g_module_open (NULL, G_MODULE_BIND_LAZY);
  CHECK (m == NULL, "the main program has no symbol lookup (documented limit)");
  LOG ("[phegmoduletest] done: %d passed, %d failed", ok_n, fail_n);
  return fail_n ? 1 : 0;
}
