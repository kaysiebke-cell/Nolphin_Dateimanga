/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-git.c: git integration via the system's git tool, §40
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

#include "nolphin-git.h"

#include <glib/gi18n.h>
#include <stdarg.h>
#include <string.h>

GQuark
nolphin_git_error_quark (void)
{
    return g_quark_from_static_string ("nolphin-git-error-quark");
}

const gchar *
nolphin_git_status_get_label (NolphinGitFileStatus status)
{
    switch (status) {
    case NOLPHIN_GIT_STATUS_UNMODIFIED:
        return _("Unverändert");
    case NOLPHIN_GIT_STATUS_MODIFIED:
        return _("Geändert");
    case NOLPHIN_GIT_STATUS_STAGED:
        return _("Bereitgestellt");
    case NOLPHIN_GIT_STATUS_ADDED:
        return _("Hinzugefügt");
    case NOLPHIN_GIT_STATUS_DELETED:
        return _("Gelöscht");
    case NOLPHIN_GIT_STATUS_RENAMED:
        return _("Umbenannt");
    case NOLPHIN_GIT_STATUS_UNTRACKED:
        return _("Unversioniert");
    case NOLPHIN_GIT_STATUS_IGNORED:
        return _("Ignoriert");
    case NOLPHIN_GIT_STATUS_CONFLICTED:
        return _("Konflikt");
    default:
        return "";
    }
}

gboolean
nolphin_git_is_available (void)
{
    gchar *path = g_find_program_in_path ("git");
    gboolean available = (path != NULL);
    g_free (path);
    return available;
}

gboolean
nolphin_git_directory_looks_like_repository (GFile *file)
{
    GFile *dir;
    gboolean owns_dir = FALSE;
    gboolean found = FALSE;
    guint depth;

    g_return_val_if_fail (G_IS_FILE (file), FALSE);

    if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY) {
        dir = file;
    } else {
        dir = g_file_get_parent (file);
        owns_dir = TRUE;
        if (dir == NULL) {
            return FALSE;
        }
    }

    /* A reasonable bound so a very deep, non-repository tree can't
     * make this scan indefinitely - real repositories are found well
     * before this in practice. */
    for (depth = 0; depth < 64 && dir != NULL; depth++) {
        GFile *dot_git = g_file_get_child (dir, ".git");
        gboolean has_dot_git = g_file_query_exists (dot_git, NULL);
        g_object_unref (dot_git);

        if (has_dot_git) {
            found = TRUE;
            break;
        }

        {
            GFile *parent = g_file_get_parent (dir);
            if (owns_dir) {
                g_object_unref (dir);
            }
            dir = parent;
            owns_dir = TRUE;
        }
    }

    if (dir != NULL && owns_dir) {
        g_object_unref (dir);
    }

    return found;
}

/* --- find repository root ---------------------------------------------- */

static void
find_root_communicate_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;
    GSubprocess *subprocess = G_SUBPROCESS (source);
    GError *error = NULL;
    gchar *stdout_buf = NULL;

    if (!g_subprocess_communicate_utf8_finish (subprocess, result, &stdout_buf, NULL, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        g_free (stdout_buf);
        return;
    }

    if (!g_subprocess_get_successful (subprocess)) {
        /* Not inside a repository - a normal, expected outcome for
         * most folders, not an error condition. */
        g_task_return_pointer (task, NULL, NULL);
        g_object_unref (task);
        g_free (stdout_buf);
        return;
    }

    {
        gchar *trimmed = g_strstrip (g_strdup (stdout_buf != NULL ? stdout_buf : ""));
        GFile *root = (trimmed[0] != '\0') ? g_file_new_for_path (trimmed) : NULL;
        g_free (trimmed);
        g_task_return_pointer (task, root, g_object_unref);
    }

    g_object_unref (task);
    g_free (stdout_buf);
}

void
nolphin_git_find_repository_root_async (GFile                *file,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data)
{
    GTask *task;
    gchar *tool_path, *dir_path;
    GFile *dir;
    GSubprocessLauncher *launcher;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (file));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_find_repository_root_async);

    tool_path = g_find_program_in_path ("git");
    if (tool_path == NULL) {
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_NOT_FOUND,
                                 _("Das Werkzeug »git« ist nicht installiert."));
        g_object_unref (task);
        return;
    }

    /* git needs a directory to start looking from - if @file is
     * itself a plain file, its parent is used automatically by
     * passing it as -C's argument only when it's actually a
     * directory; otherwise walk up once. */
    if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY) {
        dir = g_object_ref (file);
    } else {
        dir = g_file_get_parent (file);
        if (dir == NULL) {
            g_free (tool_path);
            g_task_return_pointer (task, NULL, NULL);
            g_object_unref (task);
            return;
        }
    }

    dir_path = g_file_get_path (dir);
    g_object_unref (dir);
    if (dir_path == NULL) {
        g_free (tool_path);
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_REMOTE_FILE,
                                 _("Git wird für entfernte Orte noch nicht unterstützt."));
        g_object_unref (task);
        return;
    }

    launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                          G_SUBPROCESS_FLAGS_STDERR_SILENCE);
    subprocess = g_subprocess_launcher_spawn (launcher, &error, tool_path,
                                              "-C", dir_path, "rev-parse", "--show-toplevel", NULL);
    g_object_unref (launcher);
    g_free (tool_path);
    g_free (dir_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, find_root_communicate_cb, task);
    g_object_unref (subprocess);
}

GFile *
nolphin_git_find_repository_root_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}

/* --- status -------------------------------------------------------------- */

static NolphinGitFileStatus
classify_xy (gchar x, gchar y)
{
    if (x == 'U' || y == 'U') {
        return NOLPHIN_GIT_STATUS_CONFLICTED;
    }
    if (x == '?') {
        return NOLPHIN_GIT_STATUS_UNTRACKED;
    }
    if (x == '!') {
        return NOLPHIN_GIT_STATUS_IGNORED;
    }
    if (x == 'A') {
        return NOLPHIN_GIT_STATUS_ADDED;
    }
    if (x == 'D' || y == 'D') {
        return NOLPHIN_GIT_STATUS_DELETED;
    }
    if (x == 'R') {
        return NOLPHIN_GIT_STATUS_RENAMED;
    }
    if (x != ' ') {
        return NOLPHIN_GIT_STATUS_STAGED;
    }
    if (y != ' ') {
        return NOLPHIN_GIT_STATUS_MODIFIED;
    }
    return NOLPHIN_GIT_STATUS_UNMODIFIED;
}

static void
status_bytes_ready_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;
    GSubprocess *subprocess = G_SUBPROCESS (source);
    GError *error = NULL;
    GBytes *stdout_bytes = NULL;
    GBytes *stderr_bytes = NULL;
    GHashTable *table;
    const gchar *data;
    gsize size;
    gsize i;

    if (!g_subprocess_communicate_finish (subprocess, result, &stdout_bytes, &stderr_bytes, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!g_subprocess_get_successful (subprocess)) {
        const gchar *stderr_data = (stderr_bytes != NULL) ? g_bytes_get_data (stderr_bytes, NULL) : NULL;
        gchar *detail = (stderr_data != NULL) ? g_strstrip (g_strdup (stderr_data)) : NULL;
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_FAILED,
                                 "%s%s%s",
                                 _("git status wurde mit einem Fehler beendet."),
                                 (detail != NULL && detail[0] != '\0') ? "\n" : "",
                                 (detail != NULL) ? detail : "");
        g_free (detail);
        g_object_unref (task);
        g_clear_pointer (&stdout_bytes, g_bytes_unref);
        g_clear_pointer (&stderr_bytes, g_bytes_unref);
        return;
    }

    table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    /* Each record is "XY<space><path>\0", or for renames/copies
     * "XY<space><newpath>\0<oldpath>\0" - the old-path half (when
     * present) is skipped, only the current path is tracked. */
    data = stdout_bytes != NULL ? g_bytes_get_data (stdout_bytes, &size) : NULL;
    if (data != NULL) {
        i = 0;
        while (i < size) {
            gsize path_start;
            gchar rec_x, rec_y;

            if (i + 3 > size) {
                break;
            }
            rec_x = data[i];
            rec_y = data[i + 1];
            /* data[i+2] is the space separator */
            path_start = i + 3;

            /* find the NUL terminating this path field */
            gsize j = path_start;
            while (j < size && data[j] != '\0') {
                j++;
            }

            {
                gchar *path = g_strndup (data + path_start, j - path_start);
                g_hash_table_insert (table, path, GINT_TO_POINTER (classify_xy (rec_x, rec_y)));
            }

            i = (j < size) ? j + 1 : size;

            /* Renames/copies carry a second NUL-terminated field (the
             * old path) immediately after - skip it. */
            if (rec_x == 'R' || rec_x == 'C') {
                gsize k = i;
                while (k < size && data[k] != '\0') {
                    k++;
                }
                i = (k < size) ? k + 1 : size;
            }
        }
    }

    g_task_return_pointer (task, table, (GDestroyNotify) g_hash_table_unref);
    g_object_unref (task);
    g_clear_pointer (&stdout_bytes, g_bytes_unref);
    g_clear_pointer (&stderr_bytes, g_bytes_unref);
}

void
nolphin_git_get_status_async (GFile                *repo_root,
                              GCancellable         *cancellable,
                              GAsyncReadyCallback   callback,
                              gpointer              user_data)
{
    GTask *task;
    gchar *tool_path, *path;
    GSubprocessLauncher *launcher;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_get_status_async);

    tool_path = g_find_program_in_path ("git");
    if (tool_path == NULL) {
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_NOT_FOUND,
                                 _("Das Werkzeug »git« ist nicht installiert."));
        g_object_unref (task);
        return;
    }

    path = g_file_get_path (repo_root);
    if (path == NULL) {
        g_free (tool_path);
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_REMOTE_FILE,
                                 _("Git wird für entfernte Orte noch nicht unterstützt."));
        g_object_unref (task);
        return;
    }

    launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                          G_SUBPROCESS_FLAGS_STDERR_PIPE);
    subprocess = g_subprocess_launcher_spawn (launcher, &error, tool_path,
                                              "-C", path, "status", "--porcelain=v1", "-z", "--ignored", NULL);
    g_object_unref (launcher);
    g_free (tool_path);
    g_free (path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_async (subprocess, NULL, cancellable, status_bytes_ready_cb, task);
    g_object_unref (subprocess);
}

GHashTable *
nolphin_git_get_status_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}

/* --- add / commit / pull / push / log / diff ---------------------------- */

static void
simple_bool_communicate_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;
    GSubprocess *subprocess = G_SUBPROCESS (source);
    GError *error = NULL;
    gchar *stderr_buf = NULL;

    if (!g_subprocess_communicate_utf8_finish (subprocess, result, NULL, &stderr_buf, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        g_free (stderr_buf);
        return;
    }

    if (!g_subprocess_get_successful (subprocess)) {
        gchar *detail = (stderr_buf != NULL) ? g_strstrip (g_strdup (stderr_buf)) : NULL;
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_FAILED,
                                 "%s%s%s",
                                 _("git wurde mit einem Fehler beendet."),
                                 (detail != NULL && detail[0] != '\0') ? "\n" : "",
                                 (detail != NULL) ? detail : "");
        g_free (detail);
        g_object_unref (task);
        g_free (stderr_buf);
        return;
    }

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
    g_free (stderr_buf);
}

static void
simple_text_communicate_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;
    GSubprocess *subprocess = G_SUBPROCESS (source);
    GError *error = NULL;
    gchar *stdout_buf = NULL;
    gchar *stderr_buf = NULL;

    if (!g_subprocess_communicate_utf8_finish (subprocess, result, &stdout_buf, &stderr_buf, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        g_free (stdout_buf);
        g_free (stderr_buf);
        return;
    }

    if (!g_subprocess_get_successful (subprocess)) {
        gchar *detail = (stderr_buf != NULL) ? g_strstrip (g_strdup (stderr_buf)) : NULL;
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_FAILED,
                                 "%s%s%s",
                                 _("git wurde mit einem Fehler beendet."),
                                 (detail != NULL && detail[0] != '\0') ? "\n" : "",
                                 (detail != NULL) ? detail : "");
        g_free (detail);
        g_object_unref (task);
        g_free (stdout_buf);
        g_free (stderr_buf);
        return;
    }

    /* Combine stdout with stderr's informational text (pull/push
     * commonly report their summary on stderr even on success). */
    {
        gchar *combined;
        if (stderr_buf != NULL && stderr_buf[0] != '\0') {
            combined = g_strconcat (stdout_buf != NULL ? stdout_buf : "", stderr_buf, NULL);
        } else {
            combined = g_strdup (stdout_buf != NULL ? stdout_buf : "");
        }
        g_task_return_pointer (task, combined, g_free);
    }

    g_object_unref (task);
    g_free (stdout_buf);
    g_free (stderr_buf);
}

static gchar *
require_repo_path (GFile *repo_root, GTask *task)
{
    gchar *path;

    if (!nolphin_git_is_available ()) {
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_NOT_FOUND,
                                 _("Das Werkzeug »git« ist nicht installiert."));
        g_object_unref (task);
        return NULL;
    }

    path = g_file_get_path (repo_root);
    if (path == NULL) {
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_REMOTE_FILE,
                                 _("Git wird für entfernte Orte noch nicht unterstützt."));
        g_object_unref (task);
        return NULL;
    }

    return path;
}

static GSubprocess *
spawn_git (const gchar *repo_path, GSubprocessFlags flags, GError **error, ...)
{
    GPtrArray *argv;
    GSubprocessLauncher *launcher;
    GSubprocess *subprocess;
    gchar *tool_path;
    va_list ap;
    const gchar *arg;

    tool_path = g_find_program_in_path ("git");
    if (tool_path == NULL) {
        g_set_error (error, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_NOT_FOUND,
                    "%s", _("Das Werkzeug »git« ist nicht installiert."));
        return NULL;
    }

    argv = g_ptr_array_new_with_free_func (g_free);
    g_ptr_array_add (argv, tool_path);
    g_ptr_array_add (argv, g_strdup ("-C"));
    g_ptr_array_add (argv, g_strdup (repo_path));

    va_start (ap, error);
    while ((arg = va_arg (ap, const gchar *)) != NULL) {
        g_ptr_array_add (argv, g_strdup (arg));
    }
    va_end (ap);
    g_ptr_array_add (argv, NULL);

    launcher = g_subprocess_launcher_new (flags);
    g_subprocess_launcher_setenv (launcher, "GIT_TERMINAL_PROMPT", "0", TRUE);
    g_subprocess_launcher_setenv (launcher, "GIT_ASKPASS", "", TRUE);
    subprocess = g_subprocess_launcher_spawnv (launcher, (const gchar * const *) argv->pdata, error);
    g_object_unref (launcher);
    g_ptr_array_free (argv, TRUE);

    return subprocess;
}

void
nolphin_git_add_async (GFile *repo_root, GList *files,
                       GCancellable *cancellable,
                       GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path;
    GPtrArray *argv;
    GSubprocessLauncher *launcher;
    GSubprocess *subprocess;
    GError *error = NULL;
    gchar *tool_path;
    GList *l;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_add_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    if (files == NULL) {
        g_free (repo_path);
        g_task_return_new_error (task, NOLPHIN_GIT_ERROR, NOLPHIN_GIT_ERROR_TOOL_FAILED,
                                 _("Nichts zum Hinzufügen ausgewählt."));
        g_object_unref (task);
        return;
    }

    tool_path = g_find_program_in_path ("git");
    argv = g_ptr_array_new_with_free_func (g_free);
    g_ptr_array_add (argv, tool_path);
    g_ptr_array_add (argv, g_strdup ("-C"));
    g_ptr_array_add (argv, repo_path); /* ownership moves to argv */
    g_ptr_array_add (argv, g_strdup ("add"));
    g_ptr_array_add (argv, g_strdup ("--"));
    for (l = files; l != NULL; l = l->next) {
        gchar *p = g_file_get_path (G_FILE (l->data));
        if (p != NULL) {
            g_ptr_array_add (argv, p); /* ownership moves to argv */
        }
    }
    g_ptr_array_add (argv, NULL);

    launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDERR_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE);
    subprocess = g_subprocess_launcher_spawnv (launcher, (const gchar * const *) argv->pdata, &error);
    g_object_unref (launcher);
    g_ptr_array_free (argv, TRUE);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_bool_communicate_cb, task);
    g_object_unref (subprocess);
}

gboolean
nolphin_git_add_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

void
nolphin_git_commit_async (GFile *repo_root, const gchar *message,
                          GCancellable *cancellable,
                          GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));
    g_return_if_fail (message != NULL && message[0] != '\0');

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_commit_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDERR_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE,
                            &error, "commit", "-m", message, NULL);
    g_free (repo_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_bool_communicate_cb, task);
    g_object_unref (subprocess);
}

gboolean
nolphin_git_commit_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

void
nolphin_git_pull_async (GFile *repo_root,
                        GCancellable *cancellable,
                        GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_pull_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                            &error, "pull", NULL);
    g_free (repo_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_text_communicate_cb, task);
    g_object_unref (subprocess);
}

gchar *
nolphin_git_pull_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}

void
nolphin_git_push_async (GFile *repo_root,
                        GCancellable *cancellable,
                        GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_push_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                            &error, "push", NULL);
    g_free (repo_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_text_communicate_cb, task);
    g_object_unref (subprocess);
}

gchar *
nolphin_git_push_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}

void
nolphin_git_remote_add_async (GFile *repo_root, const gchar *name, const gchar *url,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));
    g_return_if_fail (name != NULL && name[0] != '\0');
    g_return_if_fail (url != NULL && url[0] != '\0');

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_remote_add_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDERR_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE,
                            &error, "remote", "add", name, url, NULL);
    g_free (repo_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_bool_communicate_cb, task);
    g_object_unref (subprocess);
}

gboolean
nolphin_git_remote_add_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

void
nolphin_git_log_async (GFile *repo_root, GFile *path, guint max_count,
                       GCancellable *cancellable,
                       GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path, *rel_path = NULL;
    gchar *count_arg;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_log_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    count_arg = g_strdup_printf ("-%u", max_count > 0 ? max_count : 50);

    if (path != NULL) {
        rel_path = g_file_get_path (path);
    }

    if (rel_path != NULL) {
        subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                &error, "log", count_arg,
                                "--pretty=format:%h %ad %an: %s", "--date=short",
                                "--", rel_path, NULL);
    } else {
        subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                &error, "log", count_arg,
                                "--pretty=format:%h %ad %an: %s", "--date=short", NULL);
    }

    g_free (repo_path);
    g_free (rel_path);
    g_free (count_arg);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_text_communicate_cb, task);
    g_object_unref (subprocess);
}

gchar *
nolphin_git_log_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}

void
nolphin_git_diff_async (GFile *repo_root, GFile *path,
                        GCancellable *cancellable,
                        GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    gchar *repo_path, *rel_path = NULL;
    GSubprocess *subprocess;
    GError *error = NULL;

    g_return_if_fail (G_IS_FILE (repo_root));

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_git_diff_async);

    repo_path = require_repo_path (repo_root, task);
    if (repo_path == NULL) {
        return;
    }

    if (path != NULL) {
        rel_path = g_file_get_path (path);
    }

    if (rel_path != NULL) {
        subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                &error, "diff", "--", rel_path, NULL);
    } else {
        subprocess = spawn_git (repo_path, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                &error, "diff", NULL);
    }

    g_free (repo_path);
    g_free (rel_path);

    if (subprocess == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_subprocess_communicate_utf8_async (subprocess, NULL, cancellable, simple_text_communicate_cb, task);
    g_object_unref (subprocess);
}

gchar *
nolphin_git_diff_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}
