/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-archive.h: abstracted archive backend (create/extract/test), §30
 *
 * Drives the system's zip/tar/7z tools through GSubprocess - argv arrays
 * only, never a shell string - after checking each is actually installed.
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

#ifndef NOLPHIN_ARCHIVE_H
#define NOLPHIN_ARCHIVE_H

#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
    NOLPHIN_ARCHIVE_FORMAT_ZIP,
    NOLPHIN_ARCHIVE_FORMAT_TAR,
    NOLPHIN_ARCHIVE_FORMAT_TAR_GZ,
    NOLPHIN_ARCHIVE_FORMAT_TAR_BZ2,
    NOLPHIN_ARCHIVE_FORMAT_TAR_XZ,
    NOLPHIN_ARCHIVE_FORMAT_SEVEN_ZIP,
    NOLPHIN_ARCHIVE_FORMAT_UNKNOWN
} NolphinArchiveFormat;

#define NOLPHIN_ARCHIVE_ERROR (nolphin_archive_error_quark ())
GQuark nolphin_archive_error_quark (void);

typedef enum {
    NOLPHIN_ARCHIVE_ERROR_TOOL_NOT_FOUND,
    NOLPHIN_ARCHIVE_ERROR_UNKNOWN_FORMAT,
    NOLPHIN_ARCHIVE_ERROR_TOOL_FAILED
} NolphinArchiveError;

/* Human-readable label ("ZIP", "7-Zip", ...) and the tool this format
 * needs (e.g. "zip", "7z") - the latter is what
 * nolphin_archive_format_is_available() checks for with
 * g_find_program_in_path(). */
const gchar *nolphin_archive_format_get_label (NolphinArchiveFormat format);
const gchar *nolphin_archive_format_get_extension (NolphinArchiveFormat format);
gboolean     nolphin_archive_format_is_available (NolphinArchiveFormat format);

/* Guess the format from an archive's filename. FORMAT_UNKNOWN if no
 * known extension matches. */
NolphinArchiveFormat nolphin_archive_detect_format (GFile *archive_file);

/* Compress @sources (all siblings in the same directory) into a new
 * archive at @destination in @format. Fails with
 * NOLPHIN_ARCHIVE_ERROR_TOOL_NOT_FOUND if the required tool isn't
 * installed, checked up front before spawning anything. */
void     nolphin_archive_compress_async  (GList               *sources,
                                          GFile               *destination,
                                          NolphinArchiveFormat  format,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
gboolean nolphin_archive_compress_finish (GAsyncResult *result, GError **error);

/* Extract @archive_file into @destination_dir (must already exist). */
void     nolphin_archive_extract_async   (GFile               *archive_file,
                                          GFile               *destination_dir,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
gboolean nolphin_archive_extract_finish  (GAsyncResult *result, GError **error);

/* Verify @archive_file's integrity without extracting it anywhere. */
void     nolphin_archive_test_async      (GFile               *archive_file,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
gboolean nolphin_archive_test_finish     (GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* NOLPHIN_ARCHIVE_H */
