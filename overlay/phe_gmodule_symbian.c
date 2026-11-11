/* Copyright The Phorepoc Authors */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* phe_gmodule_symbian.c -- the GModule backend for this system, replacing gmodule-dl.c (patch_glib.sh copies it over gmodule/gmodule-dl.c). LGPL-2.1+ with GLib. OURS (Phorepoc).
 *
 * A module is a DLL loaded with RLibrary::Load (phegmodule_rlib.cpp: Open C has no dlopen for E32 DLLs). The DLLs of this system export by ORDINAL only (frozen .def files,
 * "name @ N NONAME"), so a symbol NAME is resolved through a table the DLL ships beside it: the symbol table file  <drive>:/resource/gmodule/<dll name>.sym , one "name ordinal" per line,
 * generated from the DLL's own .def by recipes/gen_symtab.py (the packaging step of every repository that has DLLs). g_module_symbol (module, name) works on any DLL that has one.
 * File names: the module "path/to/foo.so", "foo.dll" or "foo" is the DLL foo.dll (directory ignored: the loader searches \sys\bin on every drive). g_module_open (NULL) (the main
 * program) gives a handle on which every lookup answers "undefined symbol": a program's symbols cannot be enumerated here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* phegmodule_rlib.cpp */
extern int phe_lib_load (const char *dll_name, void **handle);
extern void *phe_lib_ordinal (void *handle, int ordinal);
extern void phe_lib_close (void *handle);

typedef struct
{
  void *lib;
  GHashTable *symbols;          /* name -> ordinal (GINT_TO_POINTER), NULL when the DLL has no table */
  gchar *dll;
} PheModule;

static gchar *
dll_name_of (const gchar *file_name)
{
  gchar *base = g_path_get_basename (file_name), *dot, *name;
  for (dot = base + strlen (base); dot > base && *dot != '.' && *dot != '/' && *dot != '\\'; dot--) ;
  if (dot > base && *dot == '.') *dot = 0;
  name = g_strconcat (base, ".dll", NULL);
  g_free (base);
  return name;
}

static GHashTable *
load_symbol_table (const gchar *dll)
{
  static const char *drives[] = { "z", "c", "e", "d", "f", NULL };
  gchar *base = g_strndup (dll, strlen (dll) - 4);   /* without .dll */
  GHashTable *t = NULL; int i;
  for (i = 0; drives[i] && !t; i++) {
    gchar *path = g_strdup_printf ("%s:/resource/gmodule/%s.sym", drives[i], base);
    FILE *f = fopen (path, "r");
    if (f) {
      char line[300];
      t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      while (fgets (line, sizeof line, f)) {
        char *sp = strrchr (line, ' '); int ord;
        if (!sp || line[0] == '#') continue;
        ord = atoi (sp + 1);
        if (ord <= 0) continue;
        *sp = 0;
        g_hash_table_insert (t, g_strdup (line), GINT_TO_POINTER (ord));
      }
      fclose (f);
    }
    g_free (path);
  }
  g_free (base);
  return t;
}

static gpointer
_g_module_open (const gchar *file_name, gboolean bind_lazy, gboolean bind_local)
{
  PheModule *m; gchar *dll; void *lib = NULL; int r;
  (void) bind_lazy; (void) bind_local;
  dll = dll_name_of (file_name);
  r = phe_lib_load (dll, &lib);
  if (r != 0) {
    gchar *msg = g_strdup_printf ("cannot load %s (error %d)", dll, r);
    g_module_set_error (msg);
    g_free (msg); g_free (dll);
    return NULL;
  }
  m = g_new0 (PheModule, 1);
  m->lib = lib; m->dll = dll;
  m->symbols = load_symbol_table (dll);
  return m;
}

static gpointer
_g_module_self (void)
{
  /* the main program: a module handle with no library; a program's symbols cannot be enumerated here, so every lookup answers "undefined symbol" (as for a program that exports nothing) */
  return g_new0 (PheModule, 1);
}

static void
_g_module_close (gpointer handle, gboolean is_unref)
{
  PheModule *m = handle;
  (void) is_unref;
  if (m->lib) phe_lib_close (m->lib);
  if (m->symbols) g_hash_table_destroy (m->symbols);
  g_free (m->dll); g_free (m);
}

static gpointer
_g_module_symbol (gpointer handle, const gchar *symbol_name)
{
  PheModule *m = handle; gpointer ord, p;
  if (!m->lib) {
    gchar *msg = g_strdup_printf ("undefined symbol: %s (the main program exports none on this system)", symbol_name);
    g_module_set_error (msg); g_free (msg);
    return NULL;
  }
  if (!m->symbols) {
    gchar *msg = g_strdup_printf ("%s has no symbol table (resource/gmodule/%s.sym)", m->dll, m->dll);
    g_module_set_error (msg); g_free (msg);
    return NULL;
  }
  ord = g_hash_table_lookup (m->symbols, symbol_name);
  if (!ord) {
    gchar *msg = g_strdup_printf ("undefined symbol: %s", symbol_name);
    g_module_set_error (msg); g_free (msg);
    return NULL;
  }
  p = phe_lib_ordinal (m->lib, GPOINTER_TO_INT (ord));
  if (!p) g_module_set_error ("the ordinal of the symbol is not exported by the DLL (stale symbol table)");
  return p;
}

static gchar *
_g_module_build_path (const gchar *directory, const gchar *module_name)
{
  if (directory && *directory)
    return g_strconcat (directory, "/", module_name, "." G_MODULE_SUFFIX, NULL);
  return g_strconcat (module_name, "." G_MODULE_SUFFIX, NULL);
}
