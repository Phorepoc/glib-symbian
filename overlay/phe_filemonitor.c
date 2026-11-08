/* phe_filemonitor.c -- the GIO local file monitor of this system (appended to gio/glocalfilemonitor.c by patch_gio.sh). LGPL-2.1+ with GLib. OURS (Phorepoc).
 *
 * GIO has no monitor implementation for Open C (inotify, kqueue, FEN, Win32 are all elsewhere), so g_file_monitor_file () / g_file_monitor_directory () returned NULL and nothing that
 * watches a configuration file or a directory of plug-in entries (libhildondesktop) saw a change. This one uses what the platform has: RFs::NotifyChange, in a thread of its own
 * (phegio_notify.cpp: the thread waits for the file server to report a change below the directory and calls back). The callback only schedules a check in the main context the monitor was
 * made in; the check compares what is there (names, modification times, sizes) with the last snapshot and reports created / changed / deleted through the same GFileMonitorSource
 * every other implementation uses, so the signals come in the main loop, rate-limited and with CHANGES_DONE_HINT like the inotify ones. */
#include "giomodule.h"
#include <sys/stat.h>
#include <dirent.h>

extern void *phe_notify_start (const char *dir_path, void (*cb) (void *), void *user);
extern void phe_notify_stop (void *handle);

typedef struct
{
  gint refs;
  GFileMonitorSource *source;
  gchar *dirname, *basename, *filename;
  GHashTable *snapshot;                  /* name -> "mtime:size" */
  void *notify;
  GMainContext *context;
  gboolean dead;
} PheWatch;

typedef struct { GLocalFileMonitor parent_instance; PheWatch *watch; } GPhefileMonitor;
typedef struct { GLocalFileMonitorClass parent_class; } GPhefileMonitorClass;
GType g_phe_file_monitor_get_type (void);
#define G_TYPE_PHE_FILE_MONITOR (g_phe_file_monitor_get_type ())
#define G_PHE_FILE_MONITOR(o) ((GPhefileMonitor *) (o))
G_DEFINE_TYPE_WITH_CODE (GPhefileMonitor, g_phe_file_monitor, G_TYPE_LOCAL_FILE_MONITOR,
                         g_io_extension_point_implement (G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME, g_define_type_id, "phe", 20))

static void
phe_watch_unref (PheWatch *w)
{
  if (!g_atomic_int_dec_and_test (&w->refs)) return;
  if (w->snapshot) g_hash_table_destroy (w->snapshot);
  if (w->context) g_main_context_unref (w->context);
  g_free (w->dirname); g_free (w->basename); g_free (w->filename);
  g_free (w);
}

static void
snapshot_add (GHashTable *t, const gchar *name, const gchar *path, gboolean with_content)
{
  struct stat st;
  if (stat (path, &st) == 0) {
    gchar *sum = NULL;
    if (with_content && st.st_size <= 65536) {                /* a watched file: its content too (the file system's time stamps are two seconds coarse, a rewrite of the same size can look the same) */
      gchar *data = NULL; gsize len = 0;
      if (g_file_get_contents (path, &data, &len, NULL)) { sum = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *) data, len); g_free (data); }
    }
    g_hash_table_replace (t, g_strdup (name), g_strdup_printf ("%" G_GINT64_FORMAT ":%" G_GINT64_FORMAT ":%s", (gint64) st.st_mtime, (gint64) st.st_size, sum ? sum : ""));
    g_free (sum);
  }
}

static GHashTable *
take_snapshot (PheWatch *w)
{
  GHashTable *t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (w->basename) snapshot_add (t, w->basename, w->filename, TRUE);
  else {
    DIR *d = opendir (w->dirname);
    if (d) {
      struct dirent *e;
      while ((e = readdir (d)) != NULL) {
        gchar *path;
        if (!strcmp (e->d_name, ".") || !strcmp (e->d_name, "..")) continue;
        path = g_build_filename (w->dirname, e->d_name, NULL); snapshot_add (t, e->d_name, path, FALSE); g_free (path);
      }
      closedir (d);
    }
  }
  return t;
}

static void
report (PheWatch *w, GFileMonitorEvent ev, const gchar *name)
{
  g_file_monitor_source_handle_event (w->source, ev, name, NULL, NULL, g_get_real_time ());
}

/* in the main context of the monitor: what changed since the last look */
static gboolean
phe_check (gpointer data)
{
  PheWatch *w = data;
  if (!w->dead) {
    GHashTable *now = take_snapshot (w); GHashTableIter it; gpointer k, v;
    g_hash_table_iter_init (&it, now);
    while (g_hash_table_iter_next (&it, &k, &v)) {
      const gchar *old = g_hash_table_lookup (w->snapshot, k);
      if (!old) { report (w, G_FILE_MONITOR_EVENT_CREATED, k); report (w, G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT, k); }
      else if (strcmp (old, v)) { report (w, G_FILE_MONITOR_EVENT_CHANGED, k); report (w, G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT, k); }
    }
    g_hash_table_iter_init (&it, w->snapshot);
    while (g_hash_table_iter_next (&it, &k, &v)) if (!g_hash_table_lookup (now, k)) report (w, G_FILE_MONITOR_EVENT_DELETED, k);
    g_hash_table_destroy (w->snapshot); w->snapshot = now;
  }
  phe_watch_unref (w);
  return G_SOURCE_REMOVE;
}

/* in the notification thread: schedule the check */
static void
phe_notified (void *user)
{
  PheWatch *w = user;
  g_atomic_int_inc (&w->refs);
  g_main_context_invoke (w->context, phe_check, w);
}

static gboolean g_phe_file_monitor_is_supported (void) { return TRUE; }

static void
g_phe_file_monitor_start (GLocalFileMonitor *local_monitor, const gchar *dirname, const gchar *basename, const gchar *filename, GFileMonitorSource *source)
{
  GPhefileMonitor *m = G_PHE_FILE_MONITOR (local_monitor);
  PheWatch *w = g_new0 (PheWatch, 1);
  w->refs = 1; w->source = source; w->dirname = g_strdup (dirname); w->basename = g_strdup (basename); w->filename = basename ? g_build_filename (dirname, basename, NULL) : g_strdup (filename);
  w->context = g_main_context_ref (g_main_context_get_thread_default () ? g_main_context_get_thread_default () : g_main_context_default ());
  w->snapshot = take_snapshot (w);
  m->watch = w;
  w->notify = phe_notify_start (dirname, phe_notified, w);
}

static gboolean
g_phe_file_monitor_cancel (GFileMonitor *monitor)
{
  GPhefileMonitor *m = G_PHE_FILE_MONITOR (monitor);
  if (m->watch) {
    PheWatch *w = m->watch; m->watch = NULL;
    w->dead = TRUE;
    if (w->notify) phe_notify_stop (w->notify);       /* waits for the notification thread to end: it makes no more callbacks afterwards */
    w->notify = NULL;
    phe_watch_unref (w);
  }
  return TRUE;
}

static void g_phe_file_monitor_init (GPhefileMonitor *m) { (void) m; }

static void
g_phe_file_monitor_class_init (GPhefileMonitorClass *klass)
{
  GFileMonitorClass *file_monitor_class = G_FILE_MONITOR_CLASS (klass);
  GLocalFileMonitorClass *local_file_monitor_class = G_LOCAL_FILE_MONITOR_CLASS (klass);
  local_file_monitor_class->is_supported = g_phe_file_monitor_is_supported;
  local_file_monitor_class->start = g_phe_file_monitor_start;
  local_file_monitor_class->mount_notify = FALSE;
  file_monitor_class->cancel = g_phe_file_monitor_cancel;
}
