/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-deb-package.c: build an installable .deb package from a file
 * selection, via the system's dpkg-deb ("deb"-Funktion)
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

#include "nolphin-deb-package.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h>

GQuark
nolphin_deb_package_error_quark (void)
{
    return g_quark_from_static_string ("nolphin-deb-package-error-quark");
}

NolphinDebPackageInfo *
nolphin_deb_package_info_new (void)
{
    return g_new0 (NolphinDebPackageInfo, 1);
}

void
nolphin_deb_package_info_free (NolphinDebPackageInfo *info)
{
    if (info == NULL) {
        return;
    }

    g_free (info->package);
    g_free (info->version);
    g_free (info->architecture);
    g_free (info->maintainer);
    g_free (info->description);
    g_free (info->section);
    g_free (info->depends);
    g_free (info->install_path);
    g_free (info);
}

gboolean
nolphin_deb_package_is_available (void)
{
    gchar *path = g_find_program_in_path ("dpkg-deb");
    gboolean available = (path != NULL);
    g_free (path);
    return available;
}

/* Debian policy §5.6.7: package names consist only of lower case
 * letters, digits, "+", "-" and ".", and must start with an
 * alphanumeric character; at least two characters long. */
gboolean
nolphin_deb_package_name_is_valid (const gchar *name)
{
    gsize len, i;

    if (name == NULL) {
        return FALSE;
    }

    len = strlen (name);
    if (len < 2) {
        return FALSE;
    }

    if (!g_ascii_islower (name[0]) && !g_ascii_isdigit (name[0])) {
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        gchar c = name[i];
        if (!g_ascii_islower (c) && !g_ascii_isdigit (c) &&
            c != '+' && c != '-' && c != '.') {
            return FALSE;
        }
    }

    return TRUE;
}

/* --- assembling the package root -------------------------------------- */

/* Best-effort: a failure to preserve the mode bit (e.g. an
 * unsupported filesystem) must not abort the whole build, so errors
 * here are swallowed - the package still gets built, just possibly
 * with default permissions on that one entry. */
static void
copy_mode (GFile *source, GFile *dest)
{
    GFileInfo *info = g_file_query_info (source, G_FILE_ATTRIBUTE_UNIX_MODE,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
    if (info == NULL) {
        return;
    }

    if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_UNIX_MODE)) {
        guint32 mode = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_UNIX_MODE);
        g_file_set_attribute_uint32 (dest, G_FILE_ATTRIBUTE_UNIX_MODE, mode,
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
    }

    g_object_unref (info);
}

/* Recursively copies @source (a file, directory or symlink) as a
 * child of @dest_dir, keeping its basename, and adds every plain
 * file's size to *@total_bytes (used for the control file's
 * "Installed-Size"). */
static gboolean
copy_recursive (GFile *source, GFile *dest_dir, guint64 *total_bytes, GError **error)
{
    gchar *basename;
    GFile *dest;
    GFileType type;
    gboolean ok = TRUE;

    basename = g_file_get_basename (source);
    dest = g_file_get_child (dest_dir, basename);
    g_free (basename);

    type = g_file_query_file_type (source, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);

    if (type == G_FILE_TYPE_SYMBOLIC_LINK) {
        GFileInfo *info = g_file_query_info (source, G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                             G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, error);
        if (info == NULL) {
            ok = FALSE;
        } else {
            const gchar *target = g_file_info_get_symlink_target (info);
            ok = g_file_make_symbolic_link (dest, target, NULL, error);
            g_object_unref (info);
        }
    } else if (type == G_FILE_TYPE_DIRECTORY) {
        ok = g_file_make_directory (dest, NULL, error);
        if (ok) {
            GFileEnumerator *enumerator;

            copy_mode (source, dest);

            enumerator = g_file_enumerate_children (source, G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, error);
            if (enumerator == NULL) {
                ok = FALSE;
            } else {
                GFileInfo *child_info;

                while (ok && (child_info = g_file_enumerator_next_file (enumerator, NULL, error)) != NULL) {
                    GFile *child = g_file_enumerator_get_child (enumerator, child_info);
                    ok = copy_recursive (child, dest, total_bytes, error);
                    g_object_unref (child);
                    g_object_unref (child_info);
                }
                if (ok && *error != NULL) {
                    /* g_file_enumerator_next_file() returned NULL
                     * because of a real error, not end-of-list. */
                    ok = FALSE;
                }
                g_object_unref (enumerator);
            }
        }
    } else {
        ok = g_file_copy (source, dest, G_FILE_COPY_NOFOLLOW_SYMLINKS, NULL, NULL, NULL, error);
        if (ok) {
            GFileInfo *size_info;

            copy_mode (source, dest);

            size_info = g_file_query_info (source, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
            if (size_info != NULL) {
                *total_bytes += g_file_info_get_size (size_info);
                g_object_unref (size_info);
            }
        }
    }

    g_object_unref (dest);
    return ok;
}

/* Best-effort recursive delete of the temporary package root - always
 * called at the end of a build, success or failure, to avoid leaking
 * temporary directories under /tmp. */
static void
remove_recursive (GFile *file)
{
    GFileType type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);

    if (type == G_FILE_TYPE_DIRECTORY) {
        GFileEnumerator *enumerator = g_file_enumerate_children (file, G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
        if (enumerator != NULL) {
            GFileInfo *info;
            while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
                GFile *child = g_file_enumerator_get_child (enumerator, info);
                remove_recursive (child);
                g_object_unref (child);
                g_object_unref (info);
            }
            g_object_unref (enumerator);
        }
    }

    g_file_delete (file, NULL, NULL);
}

/* Folds a possibly multi-line description into Debian control file
 * syntax: the first line is the synopsis on the "Description:" line
 * itself, every further line is indented with one leading space (an
 * otherwise-blank line becomes " ." per policy, since a truly empty
 * continuation line would end the field). */
static gchar *
build_control_content (const NolphinDebPackageInfo *info, guint64 total_bytes)
{
    GString *s = g_string_new (NULL);
    gchar **desc_lines;
    guint64 installed_size_kb = (total_bytes + 1023) / 1024;
    guint i;

    g_string_append_printf (s, "Package: %s\n", info->package);
    g_string_append_printf (s, "Version: %s\n", info->version);
    if (info->section != NULL && info->section[0] != '\0') {
        g_string_append_printf (s, "Section: %s\n", info->section);
    }
    g_string_append (s, "Priority: optional\n");
    g_string_append_printf (s, "Architecture: %s\n", info->architecture);
    if (info->depends != NULL && info->depends[0] != '\0') {
        g_string_append_printf (s, "Depends: %s\n", info->depends);
    }
    g_string_append_printf (s, "Installed-Size: %" G_GUINT64_FORMAT "\n", installed_size_kb);
    g_string_append_printf (s, "Maintainer: %s\n", info->maintainer);

    desc_lines = g_strsplit (info->description != NULL ? info->description : "", "\n", -1);
    g_string_append_printf (s, "Description: %s\n",
                            (desc_lines[0] != NULL && desc_lines[0][0] != '\0') ?
                            desc_lines[0] : _("Mit Nolphin erstelltes Paket"));
    for (i = 1; desc_lines[i] != NULL; i++) {
        if (desc_lines[i][0] == '\0') {
            g_string_append (s, " .\n");
        } else {
            g_string_append_printf (s, " %s\n", desc_lines[i]);
        }
    }
    g_strfreev (desc_lines);

    return g_string_free (s, FALSE);
}

/* --- async build ------------------------------------------------------- */

typedef struct {
    GList                  *sources;   /* GFile* */
    NolphinDebPackageInfo   *info;
    GFile                   *destination;
} BuildTaskData;

static void
build_task_data_free (BuildTaskData *data)
{
    g_list_free_full (data->sources, g_object_unref);
    nolphin_deb_package_info_free (data->info);
    g_clear_object (&data->destination);
    g_free (data);
}

static void
build_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    BuildTaskData *data = task_data;
    GError *error = NULL;
    gchar *tool_path = NULL;
    gchar *pkg_root = NULL;
    gchar *target_dir = NULL;
    GFile *target_dir_file = NULL;
    gchar *control_dir = NULL;
    gchar *control_path = NULL;
    gchar *control_content = NULL;
    gchar *dest_path = NULL;
    guint64 total_bytes = 0;
    GList *l;

    tool_path = g_find_program_in_path ("dpkg-deb");
    if (tool_path == NULL) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_TOOL_NOT_FOUND,
                                 _("Das Werkzeug »dpkg-deb« ist nicht installiert."));
        return;
    }

    if (!nolphin_deb_package_name_is_valid (data->info->package)) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
                                 _("»%s« ist kein gültiger Debian-Paketname (mindestens 2 Zeichen, "
                                   "nur Kleinbuchstaben, Ziffern, »+«, »-« und ».«, beginnend mit "
                                   "Buchstabe oder Ziffer)."),
                                 data->info->package != NULL ? data->info->package : "");
        g_free (tool_path);
        return;
    }

    if (data->info->version == NULL || data->info->version[0] == '\0' ||
        strpbrk (data->info->version, " \t\n") != NULL) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
                                 _("Die Versionsnummer darf nicht leer sein und keine Leerzeichen enthalten."));
        g_free (tool_path);
        return;
    }

    if (data->info->architecture == NULL || data->info->architecture[0] == '\0') {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
                                 _("Die Architektur darf nicht leer sein."));
        g_free (tool_path);
        return;
    }

    if (data->info->maintainer == NULL || data->info->maintainer[0] == '\0') {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
                                 _("Der Maintainer darf nicht leer sein."));
        g_free (tool_path);
        return;
    }

    if (data->info->install_path == NULL || data->info->install_path[0] != '/') {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
                                 _("Das Zielverzeichnis im Paket muss ein absoluter Pfad sein (z. B. »/opt/%s«)."),
                                 data->info->package);
        g_free (tool_path);
        return;
    }

    dest_path = g_file_get_path (data->destination);
    if (dest_path == NULL) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_REMOTE_FILE,
                                 _("Das .deb-Paket kann nur an einem lokalen Ort gespeichert werden."));
        g_free (tool_path);
        return;
    }

    for (l = data->sources; l != NULL; l = l->next) {
        gchar *p = g_file_get_path (G_FILE (l->data));
        gboolean local = (p != NULL);
        g_free (p);
        if (!local) {
            g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_REMOTE_FILE,
                                     _("Nur lokale Dateien und Ordner können in ein .deb-Paket aufgenommen werden."));
            g_free (tool_path);
            g_free (dest_path);
            return;
        }
    }

    pkg_root = g_dir_make_tmp ("nolphin-deb-XXXXXX", &error);
    if (pkg_root == NULL) {
        g_task_return_error (task, error);
        g_free (tool_path);
        g_free (dest_path);
        return;
    }

    target_dir = g_strconcat (pkg_root, data->info->install_path, NULL);
    if (g_mkdir_with_parents (target_dir, 0755) != 0) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_IO,
                                 _("Konnte das Zielverzeichnis »%s« im Paket nicht anlegen: %s"),
                                 data->info->install_path, g_strerror (errno));
        goto cleanup;
    }

    target_dir_file = g_file_new_for_path (target_dir);
    for (l = data->sources; l != NULL; l = l->next) {
        if (!copy_recursive (G_FILE (l->data), target_dir_file, &total_bytes, &error)) {
            g_task_return_error (task, error);
            goto cleanup;
        }
    }

    control_dir = g_build_filename (pkg_root, "DEBIAN", NULL);
    if (g_mkdir_with_parents (control_dir, 0755) != 0) {
        g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_IO,
                                 _("Konnte das Verzeichnis »DEBIAN« nicht anlegen: %s"), g_strerror (errno));
        goto cleanup;
    }

    control_content = build_control_content (data->info, total_bytes);
    control_path = g_build_filename (control_dir, "control", NULL);
    if (!g_file_set_contents (control_path, control_content, -1, &error)) {
        g_task_return_error (task, error);
        goto cleanup;
    }
    g_chmod (control_path, 0644);

    {
        GSubprocessLauncher *launcher;
        GSubprocess *subprocess;
        gchar *stderr_buf = NULL;

        launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDERR_PIPE |
                                              G_SUBPROCESS_FLAGS_STDOUT_SILENCE);
        /* --root-owner-group normalizes ownership in the .deb to
         * root:root without actually needing root privileges or
         * fakeroot (dpkg-deb >= 1.19.0.5, present on any current
         * Mint/Debian/Ubuntu release). */
        subprocess = g_subprocess_launcher_spawn (launcher, &error, tool_path,
                                                  "--build", "--root-owner-group",
                                                  pkg_root, dest_path, NULL);
        g_object_unref (launcher);

        if (subprocess == NULL) {
            g_task_return_error (task, error);
            goto cleanup;
        }

        if (!g_subprocess_communicate_utf8 (subprocess, NULL, cancellable, NULL, &stderr_buf, &error)) {
            g_object_unref (subprocess);
            g_task_return_error (task, error);
            g_free (stderr_buf);
            goto cleanup;
        }

        if (!g_subprocess_get_successful (subprocess)) {
            gchar *detail = (stderr_buf != NULL) ? g_strstrip (g_strdup (stderr_buf)) : NULL;
            g_task_return_new_error (task, NOLPHIN_DEB_PACKAGE_ERROR, NOLPHIN_DEB_PACKAGE_ERROR_TOOL_FAILED,
                                     "%s%s%s",
                                     _("dpkg-deb wurde mit einem Fehler beendet."),
                                     (detail != NULL && detail[0] != '\0') ? "\n" : "",
                                     (detail != NULL) ? detail : "");
            g_free (detail);
            g_object_unref (subprocess);
            g_free (stderr_buf);
            goto cleanup;
        }

        g_object_unref (subprocess);
        g_free (stderr_buf);
    }

    g_task_return_boolean (task, TRUE);

cleanup:
    if (pkg_root != NULL) {
        GFile *root_file = g_file_new_for_path (pkg_root);
        remove_recursive (root_file);
        g_object_unref (root_file);
    }
    g_clear_object (&target_dir_file);
    g_free (tool_path);
    g_free (dest_path);
    g_free (pkg_root);
    g_free (target_dir);
    g_free (control_dir);
    g_free (control_path);
    g_free (control_content);
}

void
nolphin_deb_package_build_async (GList                        *sources,
                                 const NolphinDebPackageInfo   *info,
                                 GFile                         *destination,
                                 GCancellable                  *cancellable,
                                 GAsyncReadyCallback            callback,
                                 gpointer                       user_data)
{
    GTask *task;
    BuildTaskData *data;
    GList *l;

    g_return_if_fail (sources != NULL);
    g_return_if_fail (info != NULL);
    g_return_if_fail (G_IS_FILE (destination));

    data = g_new0 (BuildTaskData, 1);
    for (l = sources; l != NULL; l = l->next) {
        data->sources = g_list_prepend (data->sources, g_object_ref (G_FILE (l->data)));
    }
    data->sources = g_list_reverse (data->sources);

    data->info = nolphin_deb_package_info_new ();
    data->info->package = g_strdup (info->package);
    data->info->version = g_strdup (info->version);
    data->info->architecture = g_strdup (info->architecture);
    data->info->maintainer = g_strdup (info->maintainer);
    data->info->description = g_strdup (info->description);
    data->info->section = g_strdup (info->section);
    data->info->depends = g_strdup (info->depends);
    data->info->install_path = g_strdup (info->install_path);

    data->destination = g_object_ref (destination);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, nolphin_deb_package_build_async);
    g_task_set_task_data (task, data, (GDestroyNotify) build_task_data_free);

    g_task_run_in_thread (task, build_thread);
    g_object_unref (task);
}

gboolean
nolphin_deb_package_build_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}
