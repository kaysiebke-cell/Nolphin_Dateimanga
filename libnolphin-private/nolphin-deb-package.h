/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-deb-package.h: build an installable .deb package from a file
 * selection, via the system's dpkg-deb (neuer Vertrag, "deb"-Funktion)
 *
 * Assembles a package root in a temporary directory (the selected
 * files/folders placed under the chosen install path, plus a
 * DEBIAN/control file written from the dialog's metadata) and hands it
 * to `dpkg-deb --build --root-owner-group`, which normalizes ownership
 * to root:root without needing fakeroot or actual root privileges.
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

#ifndef NOLPHIN_DEB_PACKAGE_H
#define NOLPHIN_DEB_PACKAGE_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define NOLPHIN_DEB_PACKAGE_ERROR (nolphin_deb_package_error_quark ())
GQuark nolphin_deb_package_error_quark (void);

typedef enum {
    NOLPHIN_DEB_PACKAGE_ERROR_TOOL_NOT_FOUND,
    NOLPHIN_DEB_PACKAGE_ERROR_INVALID_METADATA,
    NOLPHIN_DEB_PACKAGE_ERROR_REMOTE_FILE,
    NOLPHIN_DEB_PACKAGE_ERROR_IO,
    NOLPHIN_DEB_PACKAGE_ERROR_TOOL_FAILED
} NolphinDebPackageError;

/* All metadata the dialog collects. Everything except @package,
 * @version, @architecture, @maintainer and @install_path is optional
 * (NULL or "" leaves the corresponding control field out). */
typedef struct {
    gchar *package;        /* "Package:", must be a valid Debian package name */
    gchar *version;        /* "Version:" */
    gchar *architecture;   /* "Architecture:", e.g. "all", "amd64" */
    gchar *maintainer;     /* "Maintainer:", e.g. "Name <mail@example.com>" */
    gchar *description;    /* "Description:" - first line is the synopsis,
                             * further lines become the long description */
    gchar *section;        /* "Section:", optional */
    gchar *depends;        /* "Depends:", optional, comma-separated */
    gchar *install_path;   /* absolute path inside the package where the
                             * selected files/folders are placed, e.g.
                             * "/opt/mein-paket" */
} NolphinDebPackageInfo;

NolphinDebPackageInfo *nolphin_deb_package_info_new   (void);
void                   nolphin_deb_package_info_free  (NolphinDebPackageInfo *info);

gboolean     nolphin_deb_package_is_available     (void);

/* Debian policy package-name syntax: lowercase letters, digits, "+-.",
 * must start with an alphanumeric character, at least two characters. */
gboolean     nolphin_deb_package_name_is_valid    (const gchar *name);

/* @sources: GList of GFile* (files or directories, local only), copied
 * by basename under @info->install_path. @destination: the .deb file
 * to create. */
void         nolphin_deb_package_build_async  (GList                  *sources,
                                               const NolphinDebPackageInfo *info,
                                               GFile                  *destination,
                                               GCancellable           *cancellable,
                                               GAsyncReadyCallback     callback,
                                               gpointer                user_data);
gboolean     nolphin_deb_package_build_finish (GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* NOLPHIN_DEB_PACKAGE_H */
