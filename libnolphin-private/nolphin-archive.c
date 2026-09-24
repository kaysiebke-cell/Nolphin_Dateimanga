/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-archive.c: abstracted archive backend (create/extract/test), §30
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include <config.h>

#include "nolphin-archive.h"

#include <glib/gi18n.h>

typedef struct {
    const gchar *label;
    const gchar *extension;
    /* zip/unzip are separate binaries; tar and 7z use the same binary
     * for both directions, so create_tool == extract_tool for those. */
    const gchar *create_tool;
    const gchar *extract_tool;
} FormatInfo;

static const FormatInfo format_info[] = {
    [NOLPHIN_ARCHIVE_FORMAT_ZIP]       = { "ZIP",       ".zip",     "zip", "unzip" },
    [NOLPHIN_ARCHIVE_FORMAT_TAR]       = { "TAR",       ".tar",     "tar", "tar" },
    [NOLPHIN_ARCHIVE_FORMAT_TAR_GZ]    = { "TAR.GZ",    ".tar.gz",  "tar", "tar" },
    [NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2]   = { "TAR.BZ2",   ".tar.bz2", "tar", "tar" },
    [NOLPHIN_ARCHIVE_FORMAT_TAR_XZ]    = { "TAR.XZ",    ".tar.xz",  "tar", "tar" },
    [NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP] = { "7-Zip",     ".7z",      "7z",  "7z" },
};

GQuark
nolphin_archive_error_quark (void)
{
    return g_quark_from_static_string ("nolphin-archive-error-quark");
}

const gchar *
nolphin_archive_format_get_label (NolphinArchiveFormat format)
{
    if (format < 0 || format >= G_N_ELEMENTS (format_info)) {
        return _("Unbekannt");
    }
    return format_info[format].label;
}

const gchar *
nolphin_archive_format_get_extension (NolphinArchiveFormat format)
{
    if (format < 0 || format >= G_N_ELEMENTS (format_info)) {
        return "";
    }
    return format_info[format].extension;
}

static gboolean
tool_is_available (const gchar *tool)
{
    gchar *path = g_find_program_in_path (tool);
    gboolean available = (path != NULL);
    g_free (path);
    return available;
}

gboolean
nolphin_archive_format_is_available (NolphinArchiveFormat format)
{
    if (format < 0 || format >= G_N_ELEMENTS (format_info)) {
        return FALSE;
    }

    /* Both directions need to work for a format to count as usable;
     * for zip/unzip this is genuinely two different packages. */
    return tool_is_available (format_info[format].create_tool) &&
           tool_is_available (format_info[format].extract_tool);
}

NolphinArchiveFormat
nolphin_archive_detect_format (GFile *archive_file)
{
    gchar *name;
    gchar *lower;
    NolphinArchiveFormat result = NOLPHIN_ARCHIVE_FORMAT_UNKNOWN;

    name = g_file_get_basename (archive_file);
    if (name == NULL) {
        return NOLPHIN_ARCHIVE_FORMAT_UNKNOWN;
    }
    lower = g_ascii_strdown (name, -1);
    g_free (name);

    /* Longest/most specific suffixes first, so ".tar.gz" isn't
     * mis-detected as plain ".gz"/".tar". */
    if (g_str_has_suffix (lower, ".tar.gz") || g_str_has_suffix (lower, ".tgz")) {
        result = NOLPHIN_ARCHIVE_FORMAT_TAR_GZ;
    } else if (g_str_has_suffix (lower, ".tar.bz2") || g_str_has_suffix (lower, ".tbz2")) {
        result = NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2;
    } else if (g_str_has_suffix (lower, ".tar.xz") || g_str_has_suffix (lower, ".txz")) {
        result = NOLPHIN_ARCHIVE_FORMAT_TAR_XZ;
    } else if (g_str_has_suffix (lower, ".tar")) {
        result = NOLPHIN_ARCHIVE_FORMAT_TAR;
    } else if (g_str_has_suffix (lower, ".zip")) {
        result = NOLPHIN_ARCHIVE_FORMAT_ZIP;
    } else if (g_str_has_suffix (lower, ".7z")) {
        result = NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP;
    }

    g_free (lower);
    return result;
}

/* --- shared subprocess runner --------------------------------------- */

static void
subprocess_wait_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GSubprocess *subprocess = G_SUBPROCESS (source);
    GTask *task = G_TASK (user_data);
    GError *error = NULL;

    if (!g_subprocess_wait_finish (subprocess, result, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (g_subprocess_get_successful (subprocess)) {
        g_task_return_boolean (task, TRUE);
    } else {
        gchar *stderr_buf = NULL;
        GInputStream *err_stream = g_subprocess_get_stderr_pipe (subprocess);

        if (err_stream != NULL) {
            /* Best-effort: the pipe was fully drained by the time
             * wait_finish() returned (GSubprocess reads it internally
             * to avoid deadlocks), so this is just picking up what's
             * buffered - may be empty, that's fine. */
            GBytes *bytes = g_input_stream_read_bytes (err_stream, 4096, NULL, NULL);
            if (bytes != NULL) {
                gsize len;
                const gchar *data = g_bytes_get_data (bytes, &len);
                if (len > 0) {
                    stderr_buf = g_strndup (data, len);
                }
                g_bytes_unref (bytes);
            }
        }

        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_TOOL_FAILED,
                                 "%s",
                                 (stderr_buf != NULL && stderr_buf[0] != '\0') ?
                                 stderr_buf : _("Das Archivierungswerkzeug wurde mit einem Fehler beendet."));
        g_free (stderr_buf);
    }

    g_object_unref (task);
}

/* Runs argv[0] (an ABSOLUTE path, already resolved by the caller via
 * g_find_program_in_path so this never silently falls through to a
 * shell PATH lookup) with the given working directory, and reports
 * completion through @task. Takes ownership of @task's one reference. */
static void
run_tool_async (gchar        **argv,
                const gchar   *working_directory,
                GCancellable  *cancellable,
                GTask         *task)
{
    GSubprocessLauncher *launcher;
    GSubprocess *subprocess;
    GError *error = NULL;

    launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDERR_PIPE |
                                          G_SUBPROCESS_FLAGS_STDOUT_SILENCE);
    if (working_directory != NULL) {
        g_subprocess_launcher_set_cwd (launcher, working_directory);
    }

    subprocess = g_subprocess_launcher_spawnv (launcher, (const gchar * const *) argv, &error);
    g_object_unref (launcher);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_wait_async (subprocess, cancellable, subprocess_wait_cb, task);
    g_object_unref (subprocess);
}

static gboolean
check_tool_available (NolphinArchiveFormat format, const gchar *tool, GTask *task)
{
    if (tool_is_available (tool)) {
        return TRUE;
    }

    g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_TOOL_NOT_FOUND,
                             _("Das für %s-Archive benötigte Werkzeug »%s« ist nicht installiert."),
                             tool, format_info[format].label);
    g_object_unref (task);
    return FALSE;
}

/* --- compress --------------------------------------------------------- */

void
nolphin_archive_compress_async (GList                *sources,
                                GFile                *destination,
                                NolphinArchiveFormat   format,
                                GCancellable          *cancellable,
                                GAsyncReadyCallback    callback,
                                gpointer               user_data)
{
    GTask *task;
    GPtrArray *argv;
    GFile *parent;
    gchar *parent_path, *dest_path, *tool_path;
    GList *l;

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_archive_compress_async);

    if (sources == NULL) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Nichts zu komprimieren."));
        g_object_unref (task);
        return;
    }

    if (format < 0 || format >= G_N_ELEMENTS (format_info)) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Unbekanntes Archivformat."));
        g_object_unref (task);
        return;
    }

    if (!check_tool_available (format, format_info[format].create_tool, task)) {
        return;
    }

    /* All sources must be siblings: they're passed to the archiver as
     * plain basenames with the shared parent as the working
     * directory, so the archive stores relative paths instead of this
     * machine's absolute filesystem layout. */
    parent = g_file_get_parent (G_FILE (sources->data));
    if (parent == NULL) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Ein Objekt ohne übergeordneten Ordner kann nicht komprimiert werden."));
        g_object_unref (task);
        return;
    }
    for (l = sources->next; l != NULL; l = l->next) {
        GFile *this_parent = g_file_get_parent (G_FILE (l->data));
        gboolean same = (this_parent != NULL && g_file_equal (parent, this_parent));
        g_clear_object (&this_parent);
        if (!same) {
            g_clear_object (&parent);
            g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                     _("Alle ausgewählten Objekte müssen sich im selben Ordner befinden, um gemeinsam komprimiert zu werden."));
            g_object_unref (task);
            return;
        }
    }

    parent_path = g_file_get_path (parent);
    dest_path = g_file_get_path (destination);
    g_clear_object (&parent);

    if (parent_path == NULL || dest_path == NULL) {
        g_free (parent_path);
        g_free (dest_path);
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Entfernte Orte werden für das Archivieren noch nicht unterstützt."));
        g_object_unref (task);
        return;
    }

    tool_path = g_find_program_in_path (format_info[format].create_tool);
    argv = g_ptr_array_new_with_free_func (g_free);
    g_ptr_array_add (argv, tool_path); /* ownership moves to argv */

    switch (format) {
        case NOLPHIN_ARCHIVE_FORMAT_ZIP:
            g_ptr_array_add (argv, g_strdup ("-r"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR:
            g_ptr_array_add (argv, g_strdup ("-cf"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR_GZ:
            g_ptr_array_add (argv, g_strdup ("-czf"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2:
            g_ptr_array_add (argv, g_strdup ("-cjf"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR_XZ:
            g_ptr_array_add (argv, g_strdup ("-cJf"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP:
            g_ptr_array_add (argv, g_strdup ("a"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        default:
            g_assert_not_reached ();
    }

    for (l = sources; l != NULL; l = l->next) {
        gchar *basename = g_file_get_basename (G_FILE (l->data));
        g_ptr_array_add (argv, basename); /* ownership moves to argv */
    }
    g_ptr_array_add (argv, NULL);

    run_tool_async ((gchar **) argv->pdata, parent_path, cancellable, task);

    g_ptr_array_free (argv, TRUE);
    g_free (parent_path);
    g_free (dest_path);
}

gboolean
nolphin_archive_compress_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

/* --- extract ------------------------------------------------------------ */

void
nolphin_archive_extract_async (GFile               *archive_file,
                               GFile               *destination_dir,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback   callback,
                               gpointer              user_data)
{
    GTask *task;
    NolphinArchiveFormat format;
    GPtrArray *argv;
    gchar *archive_path, *dest_path, *tool_path, *o_arg;

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_archive_extract_async);

    format = nolphin_archive_detect_format (archive_file);
    if (format == NOLPHIN_ARCHIVE_FORMAT_UNKNOWN) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Nicht erkannter Archivtyp."));
        g_object_unref (task);
        return;
    }

    if (!check_tool_available (format, format_info[format].extract_tool, task)) {
        return;
    }

    archive_path = g_file_get_path (archive_file);
    dest_path = g_file_get_path (destination_dir);
    if (archive_path == NULL || dest_path == NULL) {
        g_free (archive_path);
        g_free (dest_path);
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Entfernte Orte werden für das Archivieren noch nicht unterstützt."));
        g_object_unref (task);
        return;
    }

    tool_path = g_find_program_in_path (format_info[format].extract_tool);
    argv = g_ptr_array_new_with_free_func (g_free);
    g_ptr_array_add (argv, tool_path);

    switch (format) {
        case NOLPHIN_ARCHIVE_FORMAT_ZIP:
            g_ptr_array_add (argv, g_strdup ("-o")); /* overwrite without prompting */
            g_ptr_array_add (argv, g_strdup (archive_path));
            g_ptr_array_add (argv, g_strdup ("-d"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_GZ:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_XZ:
            /* Plain -xf: GNU tar auto-detects the compression, no
             * need to pick z/j/J again on the way out. */
            g_ptr_array_add (argv, g_strdup ("-xf"));
            g_ptr_array_add (argv, g_strdup (archive_path));
            g_ptr_array_add (argv, g_strdup ("-C"));
            g_ptr_array_add (argv, g_strdup (dest_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP:
            g_ptr_array_add (argv, g_strdup ("x"));
            g_ptr_array_add (argv, g_strdup (archive_path));
            o_arg = g_strconcat ("-o", dest_path, NULL); /* 7z requires no space after -o */
            g_ptr_array_add (argv, o_arg);
            g_ptr_array_add (argv, g_strdup ("-y"));
            break;
        default:
            g_assert_not_reached ();
    }
    g_ptr_array_add (argv, NULL);

    run_tool_async ((gchar **) argv->pdata, NULL, cancellable, task);

    g_ptr_array_free (argv, TRUE);
    g_free (archive_path);
    g_free (dest_path);
}

gboolean
nolphin_archive_extract_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

/* --- test ---------------------------------------------------------------- */

void
nolphin_archive_test_async (GFile               *archive_file,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback   callback,
                            gpointer              user_data)
{
    GTask *task;
    NolphinArchiveFormat format;
    GPtrArray *argv;
    gchar *archive_path, *tool_path;

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_archive_test_async);

    format = nolphin_archive_detect_format (archive_file);
    if (format == NOLPHIN_ARCHIVE_FORMAT_UNKNOWN) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Nicht erkannter Archivtyp."));
        g_object_unref (task);
        return;
    }

    if (!check_tool_available (format, format_info[format].extract_tool, task)) {
        return;
    }

    archive_path = g_file_get_path (archive_file);
    if (archive_path == NULL) {
        g_task_return_new_error (task, NOLPHIN_ARCHIVE_ERROR, NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
                                 _("Entfernte Orte werden für das Archivieren noch nicht unterstützt."));
        g_object_unref (task);
        return;
    }

    tool_path = g_find_program_in_path (format_info[format].extract_tool);
    argv = g_ptr_array_new_with_free_func (g_free);
    g_ptr_array_add (argv, tool_path);

    switch (format) {
        case NOLPHIN_ARCHIVE_FORMAT_ZIP:
            g_ptr_array_add (argv, g_strdup ("-t"));
            g_ptr_array_add (argv, g_strdup (archive_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_TAR:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_GZ:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2:
        case NOLPHIN_ARCHIVE_FORMAT_TAR_XZ:
            /* tar has no dedicated integrity-check mode; listing the
             * contents at least fails loudly if the archive is
             * truncated or not actually a tar stream. */
            g_ptr_array_add (argv, g_strdup ("-tf"));
            g_ptr_array_add (argv, g_strdup (archive_path));
            break;
        case NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP:
            g_ptr_array_add (argv, g_strdup ("t"));
            g_ptr_array_add (argv, g_strdup (archive_path));
            break;
        default:
            g_assert_not_reached ();
    }
    g_ptr_array_add (argv, NULL);

    run_tool_async ((gchar **) argv->pdata, NULL, cancellable, task);

    g_ptr_array_free (argv, TRUE);
    g_free (archive_path);
}

gboolean
nolphin_archive_test_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}
