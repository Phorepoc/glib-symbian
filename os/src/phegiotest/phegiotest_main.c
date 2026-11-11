/* Copyright The Phorepoc Authors */
/* SPDX-License-Identifier: BSD-2-Clause */
/* phegiotest_main.c -- GIO file monitors on this system (overlay/phe_filemonitor.c: RFs::NotifyChange): a watched file that is rewritten, a watched directory in which a file is created and
 * deleted; every change must arrive as a signal in the main loop. BSD-2-Clause (Phorepoc). */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

extern void phe_log (const char *msg);
#define LOG(...) do { char m_[300]; g_snprintf (m_, sizeof m_, __VA_ARGS__); phe_log (m_); } while (0)
#define DIR "c:/data/phemon"

static GMainLoop *loop;
static int file_changed, dir_created, dir_deleted, step;
static gchar *file_path, *new_path;

static const char *
ev_name (GFileMonitorEvent e)
{
  switch (e) { case G_FILE_MONITOR_EVENT_CHANGED: return "changed"; case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT: return "changes-done"; case G_FILE_MONITOR_EVENT_DELETED: return "deleted";
               case G_FILE_MONITOR_EVENT_CREATED: return "created"; default: return "other"; }
}

static void
on_file (GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, gpointer d)
{
  gchar *b = g_file_get_basename (f); (void) m; (void) o; (void) d;
  LOG ("[phegiotest] file monitor: %s %s", b, ev_name (e));
  if (e == G_FILE_MONITOR_EVENT_CHANGED || e == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) file_changed++;
  g_free (b);
}

static void
on_dir (GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, gpointer d)
{
  gchar *b = g_file_get_basename (f); (void) m; (void) o; (void) d;
  LOG ("[phegiotest] directory monitor: %s %s", b, ev_name (e));
  if (!strcmp (b, "new.desktop")) { if (e == G_FILE_MONITOR_EVENT_CREATED) dir_created++; if (e == G_FILE_MONITOR_EVENT_DELETED) dir_deleted++; }
  g_free (b);
}

static gboolean
drive (gpointer d)
{
  (void) d;
  switch (step++) {
    case 0: g_file_set_contents (file_path, "two\n", -1, NULL); LOG ("[phegiotest] rewrote the watched file"); break;
    case 1: g_file_set_contents (new_path, "[Desktop Entry]\n", -1, NULL); LOG ("[phegiotest] created new.desktop in the watched directory"); break;
    case 2: g_unlink (new_path); LOG ("[phegiotest] deleted new.desktop"); break;
    default: {
      int ok = file_changed > 0 && dir_created > 0 && dir_deleted > 0;
      LOG ("[phegiotest] file changes seen %d, created %d, deleted %d", file_changed, dir_created, dir_deleted);
      LOG ("[phegiotest] done: %s", ok ? "PASS" : "FAIL");
      g_main_loop_quit (loop); return FALSE; }
  }
  return TRUE;
}

/* GIO's zlib converters (libz.dll): gzip a text through a GConverterOutputStream, gunzip it back, compare */
static int
zlib_roundtrip (void)
{
  GString *text = g_string_new (NULL); GError *err = NULL; int i, ok;
  for (i = 0; i < 200; i++) g_string_append_printf (text, "line %d of the GIO zlib round trip\n", i);
  GOutputStream *mem = g_memory_output_stream_new_resizable ();
  GConverter *c = G_CONVERTER (g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, -1));
  GOutputStream *zs = g_converter_output_stream_new (mem, c);
  ok = g_output_stream_write_all (zs, text->str, text->len, NULL, NULL, &err) && g_output_stream_close (zs, NULL, &err);
  gsize zlen = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem));
  GInputStream *in = g_memory_input_stream_new_from_data (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem)), zlen, NULL);
  GConverter *d = G_CONVERTER (g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP));
  GInputStream *us = g_converter_input_stream_new (in, d);
  char *back = g_malloc0 (text->len + 64); gsize got = 0;
  ok = ok && g_input_stream_read_all (us, back, text->len + 63, &got, NULL, &err);
  ok = ok && got == text->len && memcmp (back, text->str, got) == 0 && zlen < text->len;
  LOG ("[phegiotest] zlib round trip %s: %u bytes -> %u gzip -> %u%s%s", ok ? "OK" : "FAILED", (unsigned) text->len, (unsigned) zlen,
       (unsigned) got, err ? " error: " : "", err ? err->message : "");
  g_free (back); g_object_unref (us); g_object_unref (in); g_object_unref (zs); g_object_unref (mem);
  g_object_unref (c); g_object_unref (d); g_string_free (text, TRUE); g_clear_error (&err);
  return ok;
}

int
phe_giotest_main (void)
{
  GFile *f, *dir; GFileMonitor *fm, *dm; GError *err = NULL;
  zlib_roundtrip ();
  g_mkdir_with_parents (DIR, 0755);
  file_path = g_strdup (DIR "/watched.conf"); new_path = g_strdup (DIR "/new.desktop");
  g_unlink (new_path); g_file_set_contents (file_path, "one\n", -1, NULL);
  f = g_file_new_for_path (file_path); dir = g_file_new_for_path (DIR);
  fm = g_file_monitor_file (f, G_FILE_MONITOR_NONE, NULL, &err);
  LOG ("[phegiotest] g_file_monitor_file: %s%s%s", fm ? "ok" : "NULL", err ? ": " : "", err ? err->message : ""); g_clear_error (&err);
  dm = g_file_monitor_directory (dir, G_FILE_MONITOR_NONE, NULL, &err);
  LOG ("[phegiotest] g_file_monitor_directory: %s%s%s", dm ? "ok" : "NULL", err ? ": " : "", err ? err->message : ""); g_clear_error (&err);
  if (!fm || !dm) { LOG ("[phegiotest] done: FAIL"); return 1; }
  g_signal_connect (fm, "changed", G_CALLBACK (on_file), NULL); g_signal_connect (dm, "changed", G_CALLBACK (on_dir), NULL);
  loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add_seconds (3, drive, NULL);
  g_main_loop_run (loop);
  g_file_monitor_cancel (fm); g_file_monitor_cancel (dm);
  g_object_unref (fm); g_object_unref (dm);
  LOG ("[phegiotest] monitors cancelled");
  return file_changed > 0 && dir_created > 0 && dir_deleted > 0 ? 0 : 1;
}
