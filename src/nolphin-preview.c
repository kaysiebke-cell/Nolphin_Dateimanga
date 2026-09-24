/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-preview.c: right-hand info/preview panel (F11)
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

#include "nolphin-preview.h"

#include <glib/gi18n.h>
#include <libnolphin-private/nolphin-global-preferences.h>
#include <libnolphin-private/nolphin-cad.h>

#define PREVIEW_IMAGE_SIZE 256

struct _NolphinPreview
{
    GtkBox parent_instance;

    GtkWidget *image;
    GtkWidget *name_label;
    GtkWidget *info_grid;
    GtkWidget *fallback_label;

    /* The single file this panel is currently tracking for async
     * refresh (late thumbnails, in-progress folder size, live
     * metadata changes). NULL in the empty/multi-selection cases,
     * where there's nothing sensible to keep watching. */
    NolphinFile *watched_file;
    gulong changed_handler_id;
    gulong deep_count_handler_id;

    /* §29: 3D/CAD metadata for watched_file, filled in asynchronously
     * (nolphin_cad_get_info_async runs a background thread - see
     * nolphin-cad.c). cad_info_file marks which file the two fields
     * below apply to (may be an attempt that failed, cad_info_error
     * set instead of cad_info) - NULL when nothing has been requested
     * yet for the current watched_file. */
    GCancellable *cad_cancellable;
    NolphinFile *cad_info_file;
    NolphinCadInfo *cad_info;
    gchar *cad_info_error;
};

G_DEFINE_TYPE (NolphinPreview, nolphin_preview, GTK_TYPE_BOX)

static void display_subject (NolphinPreview *preview, NolphinFile *file);

static void
clear_cad_state (NolphinPreview *preview)
{
    if (preview->cad_cancellable != NULL) {
        g_cancellable_cancel (preview->cad_cancellable);
        g_clear_object (&preview->cad_cancellable);
    }
    g_clear_pointer (&preview->cad_info, nolphin_cad_info_free);
    g_clear_pointer (&preview->cad_info_error, g_free);
    g_clear_pointer (&preview->cad_info_file, nolphin_file_unref);
}

static void
stop_watching_file (NolphinPreview *preview)
{
    if (preview->watched_file == NULL) {
        return;
    }

    if (preview->changed_handler_id != 0) {
        g_signal_handler_disconnect (preview->watched_file, preview->changed_handler_id);
        preview->changed_handler_id = 0;
    }
    if (preview->deep_count_handler_id != 0) {
        g_signal_handler_disconnect (preview->watched_file, preview->deep_count_handler_id);
        preview->deep_count_handler_id = 0;
    }

    clear_cad_state (preview);

    nolphin_file_unref (preview->watched_file);
    preview->watched_file = NULL;
}

static void
watched_file_changed_cb (NolphinFile *file,
                          gpointer     user_data)
{
    NolphinPreview *preview = NOLPHIN_PREVIEW (user_data);

    /* Re-render with the same subject; picks up whatever changed
     * (permissions, a newly-arrived thumbnail, deep count progress). */
    display_subject (preview, file);
}

static void
add_info_row (GtkGrid *grid, gint row, const gchar *label_text, const gchar *value_text)
{
    GtkWidget *label, *value;

    if (value_text == NULL || value_text[0] == '\0') {
        return;
    }

    label = gtk_label_new (label_text);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
    gtk_grid_attach (grid, label, 0, row, 1, 1);

    value = gtk_label_new (value_text);
    gtk_widget_set_halign (value, GTK_ALIGN_START);
    gtk_label_set_line_wrap (GTK_LABEL (value), TRUE);
    gtk_label_set_selectable (GTK_LABEL (value), TRUE);
    gtk_grid_attach (grid, value, 1, row, 1, 1);

    gtk_widget_show (label);
    gtk_widget_show (value);
}

static void
clear_grid (GtkGrid *grid)
{
    GList *children, *l;

    children = gtk_container_get_children (GTK_CONTAINER (grid));
    for (l = children; l != NULL; l = l->next) {
        gtk_widget_destroy (GTK_WIDGET (l->data));
    }
    g_list_free (children);
}

typedef struct {
    NolphinPreview *preview; /* reffed, so this stays valid even if the
                               * panel is torn down mid-request */
    NolphinFile *file;       /* reffed - the subject this analysis is for */
} CadRequest;

static void
cad_request_free (CadRequest *req)
{
    g_object_unref (req->preview);
    nolphin_file_unref (req->file);
    g_free (req);
}

static void
cad_info_ready_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
    CadRequest *req = user_data;
    GError *error = NULL;
    NolphinCadInfo *info = nolphin_cad_get_info_finish (result, &error);

    /* Only apply the result if this is still the file the panel is
     * showing - otherwise the user has already moved on and this is a
     * stale, possibly-cancelled request that arrived late. */
    if (req->file == req->preview->watched_file) {
        g_clear_pointer (&req->preview->cad_info, nolphin_cad_info_free);
        g_clear_pointer (&req->preview->cad_info_error, g_free);
        g_clear_pointer (&req->preview->cad_info_file, nolphin_file_unref);

        req->preview->cad_info_file = nolphin_file_ref (req->file);
        if (info != NULL) {
            req->preview->cad_info = info;
        } else {
            req->preview->cad_info_error = g_strdup (error != NULL ? error->message : _("Unknown error"));
        }

        /* Re-render: display_single_file() will now find the cache
         * populated and show the real rows instead of re-requesting. */
        display_subject (req->preview, req->file);
    } else {
        nolphin_cad_info_free (info);
    }

    g_clear_error (&error);
    cad_request_free (req);
}

static void
add_cad_info_rows (GtkGrid *grid, gint *row, NolphinCadInfo *info)
{
    gchar *text;

    switch (info->format) {
        case NOLPHIN_CAD_FORMAT_STL:
            add_info_row (grid, (*row)++, _("STL type:"),
                          info->stl_is_binary ? _("Binary") : _("ASCII"));
            if (info->stl_triangle_count_known) {
                text = g_strdup_printf ("%" G_GUINT64_FORMAT, info->stl_triangle_count);
                add_info_row (grid, (*row)++, _("Triangles:"), text);
                g_free (text);
            } else {
                add_info_row (grid, (*row)++, _("Triangles:"), _("not counted (file too large)"));
            }
            break;
        case NOLPHIN_CAD_FORMAT_STEP:
            add_info_row (grid, (*row)++, _("STEP description:"), info->step_description);
            add_info_row (grid, (*row)++, _("STEP file name:"), info->step_file_name);
            add_info_row (grid, (*row)++, _("STEP timestamp:"), info->step_timestamp);
            add_info_row (grid, (*row)++, _("STEP author:"), info->step_author);
            add_info_row (grid, (*row)++, _("STEP schema:"), info->step_schema);
            break;
        case NOLPHIN_CAD_FORMAT_FCSTD:
            add_info_row (grid, (*row)++, _("FreeCAD comment:"), info->fcstd_comment);
            add_info_row (grid, (*row)++, _("FreeCAD author:"), info->fcstd_author);
            add_info_row (grid, (*row)++, _("FreeCAD company:"), info->fcstd_company);
            add_info_row (grid, (*row)++, _("FreeCAD created:"), info->fcstd_created_date);
            add_info_row (grid, (*row)++, _("FreeCAD modified:"), info->fcstd_last_modified_date);
            break;
        default:
            break;
    }
}

static void
display_single_file (NolphinPreview *preview, NolphinFile *file)
{
    GtkGrid *grid = GTK_GRID (preview->info_grid);
    gint row = 0;
    gchar *display_name;
    gchar *text;
    goffset size;
    gboolean is_dir;

    display_name = nolphin_file_get_display_name (file);
    gtk_label_set_text (GTK_LABEL (preview->name_label), display_name);
    gtk_label_set_line_wrap (GTK_LABEL (preview->name_label), TRUE);
    g_free (display_name);

    clear_grid (grid);

    text = nolphin_file_get_string_attribute (file, "type");
    add_info_row (grid, row++, _("Type:"), text);
    g_free (text);

    is_dir = nolphin_file_is_directory (file);
    size = nolphin_file_get_size (file);
    if (is_dir) {
        text = nolphin_file_get_string_attribute (file, "deep_size");
    } else if (size >= 0) {
        text = nolphin_file_get_string_attribute (file, "size");
    } else {
        text = NULL;
    }
    add_info_row (grid, row++, _("Size:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "date_modified_full");
    add_info_row (grid, row++, _("Modified:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "date_accessed_full");
    add_info_row (grid, row++, _("Accessed:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "permissions");
    add_info_row (grid, row++, _("Permissions:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "owner");
    add_info_row (grid, row++, _("Owner:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "group");
    add_info_row (grid, row++, _("Group:"), text);
    g_free (text);

    text = nolphin_file_get_string_attribute (file, "where");
    add_info_row (grid, row++, _("Location:"), text);
    g_free (text);

    text = nolphin_file_get_symbolic_link_target_path (file);
    if (text != NULL) {
        add_info_row (grid, row++, _("Link target:"), text);
        g_free (text);
    }

    /* §29: 3D/CAD metadata. Detection itself is free (just an
     * extension check); the actual read happens in a background
     * thread and is cached per-file so re-rendering (e.g. once the
     * result arrives) doesn't re-request it. */
    {
        GFile *location = nolphin_file_get_location (file);
        NolphinCadFormat cad_format = nolphin_cad_detect_format (location);

        if (cad_format == NOLPHIN_CAD_FORMAT_UNKNOWN) {
            g_object_unref (location);
        } else if (!nolphin_cad_format_has_backend (cad_format)) {
            add_info_row (grid, row++, _("3D/CAD format:"), nolphin_cad_format_get_label (cad_format));
            add_info_row (grid, row++, _("3D/CAD preview:"),
                          _("No backend available for this format on this system"));
            g_object_unref (location);
        } else if (preview->cad_info_file == file) {
            /* Already attempted for this exact file - show the cached
             * outcome instead of asking again. */
            add_info_row (grid, row++, _("3D/CAD format:"), nolphin_cad_format_get_label (cad_format));
            if (preview->cad_info != NULL) {
                add_cad_info_rows (grid, &row, preview->cad_info);
            } else {
                add_info_row (grid, row++, _("3D/CAD preview:"), preview->cad_info_error);
            }
            g_object_unref (location);
        } else {
            CadRequest *req;

            add_info_row (grid, row++, _("3D/CAD format:"), nolphin_cad_format_get_label (cad_format));
            add_info_row (grid, row++, _("3D/CAD preview:"), _("Analyzing…"));

            if (preview->cad_cancellable != NULL) {
                g_cancellable_cancel (preview->cad_cancellable);
                g_object_unref (preview->cad_cancellable);
            }
            preview->cad_cancellable = g_cancellable_new ();

            req = g_new0 (CadRequest, 1);
            req->preview = g_object_ref (preview);
            req->file = nolphin_file_ref (file);

            nolphin_cad_get_info_async (location, preview->cad_cancellable, cad_info_ready_cb, req);
            g_object_unref (location);
        }
    }

    /* Image preview. Skip large files rather than decode them
     * synchronously - nolphin_file_get_icon_pixbuf() returns whatever
     * is already cached/generated (falling back to a generic mime
     * icon) without blocking; a real thumbnail that's still being
     * generated in the background arrives later via "changed". */
    if (g_settings_get_boolean (nolphin_preferences, NOLPHIN_PREFERENCES_PREVIEW_ENABLED) &&
        (size < 0 || size <= g_settings_get_int64 (nolphin_preferences, NOLPHIN_PREFERENCES_PREVIEW_MAX_FILE_SIZE))) {
        GdkPixbuf *pixbuf;

        pixbuf = nolphin_file_get_icon_pixbuf (file, PREVIEW_IMAGE_SIZE, FALSE,
                                               gtk_widget_get_scale_factor (GTK_WIDGET (preview)),
                                               NOLPHIN_FILE_ICON_FLAGS_USE_THUMBNAILS);
        if (pixbuf != NULL) {
            gtk_image_set_from_pixbuf (GTK_IMAGE (preview->image), pixbuf);
            g_object_unref (pixbuf);
        } else {
            gtk_image_clear (GTK_IMAGE (preview->image));
        }
    } else {
        gtk_image_clear (GTK_IMAGE (preview->image));
    }

    /* If FreeCAD embedded a thumbnail in this .FCStd, prefer showing
     * that actual preview over the generic file-type icon above. */
    if (preview->cad_info_file == file && preview->cad_info != NULL &&
        preview->cad_info->fcstd_thumbnail_png != NULL) {
        GInputStream *stream = g_memory_input_stream_new_from_bytes (preview->cad_info->fcstd_thumbnail_png);
        GdkPixbuf *thumb = gdk_pixbuf_new_from_stream (stream, NULL, NULL);

        g_object_unref (stream);
        if (thumb != NULL) {
            gtk_image_set_from_pixbuf (GTK_IMAGE (preview->image), thumb);
            g_object_unref (thumb);
        }
    }

    if (is_dir) {
        nolphin_file_recompute_deep_counts (file);
    }
}

static void
display_multi_selection (NolphinPreview *preview, GList *selection)
{
    GtkGrid *grid = GTK_GRID (preview->info_grid);
    GList *l;
    guint count;
    goffset total_size;
    gchar *text;

    count = g_list_length (selection);
    total_size = 0;
    for (l = selection; l != NULL; l = l->next) {
        NolphinFile *file = NOLPHIN_FILE (l->data);
        if (!nolphin_file_is_directory (file)) {
            total_size += nolphin_file_get_size (file);
        }
    }

    text = g_strdup_printf (ngettext ("%u item selected", "%u items selected", count), count);
    gtk_label_set_text (GTK_LABEL (preview->name_label), text);
    g_free (text);

    clear_grid (grid);

    text = g_format_size (total_size);
    add_info_row (grid, 0, _("Combined size:"), text);
    g_free (text);
    add_info_row (grid, 1, _("Note:"), _("folder sizes not included"));

    gtk_image_clear (GTK_IMAGE (preview->image));
}

static void
display_subject (NolphinPreview *preview, NolphinFile *file)
{
    if (file != preview->watched_file) {
        stop_watching_file (preview);

        preview->watched_file = nolphin_file_ref (file);
        preview->changed_handler_id =
            g_signal_connect (file, "changed", G_CALLBACK (watched_file_changed_cb), preview);
        preview->deep_count_handler_id =
            g_signal_connect (file, "updated_deep_count_in_progress",
                              G_CALLBACK (watched_file_changed_cb), preview);
    }

    display_single_file (preview, file);

    gtk_stack_set_visible_child_name (GTK_STACK (gtk_widget_get_parent (preview->fallback_label)),
                                      "content");
}

void
nolphin_preview_set_selection (NolphinPreview *preview,
                                GList          *selection,
                                NolphinFile    *directory_as_file)
{
    g_return_if_fail (NOLPHIN_IS_PREVIEW (preview));

    if (g_list_length (selection) == 1) {
        display_subject (preview, NOLPHIN_FILE (selection->data));
    } else if (selection != NULL) {
        stop_watching_file (preview);
        display_multi_selection (preview, selection);
    } else if (directory_as_file != NULL) {
        display_subject (preview, directory_as_file);
    } else {
        nolphin_preview_clear (preview);
    }
}

void
nolphin_preview_clear (NolphinPreview *preview)
{
    g_return_if_fail (NOLPHIN_IS_PREVIEW (preview));

    stop_watching_file (preview);
    gtk_label_set_text (GTK_LABEL (preview->name_label), "");
    clear_grid (GTK_GRID (preview->info_grid));
    gtk_image_clear (GTK_IMAGE (preview->image));
}

static void
nolphin_preview_dispose (GObject *object)
{
    NolphinPreview *preview = NOLPHIN_PREVIEW (object);

    stop_watching_file (preview);

    G_OBJECT_CLASS (nolphin_preview_parent_class)->dispose (object);
}

static void
nolphin_preview_class_init (NolphinPreviewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = nolphin_preview_dispose;
}

static void
nolphin_preview_init (NolphinPreview *preview)
{
    GtkWidget *scrolled, *content_box, *stack;

    gtk_orientable_set_orientation (GTK_ORIENTABLE (preview), GTK_ORIENTATION_VERTICAL);

    stack = gtk_stack_new ();

    content_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start (content_box, 8);
    gtk_widget_set_margin_end (content_box, 8);
    gtk_widget_set_margin_top (content_box, 8);
    gtk_widget_set_margin_bottom (content_box, 8);

    preview->image = gtk_image_new ();
    gtk_widget_set_halign (preview->image, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (content_box), preview->image, FALSE, FALSE, 0);

    preview->name_label = gtk_label_new ("");
    gtk_label_set_line_wrap (GTK_LABEL (preview->name_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (preview->name_label), GTK_JUSTIFY_CENTER);
    gtk_style_context_add_class (gtk_widget_get_style_context (preview->name_label), "heading");
    gtk_box_pack_start (GTK_BOX (content_box), preview->name_label, FALSE, FALSE, 0);

    preview->info_grid = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (preview->info_grid), 4);
    gtk_grid_set_column_spacing (GTK_GRID (preview->info_grid), 8);
    gtk_box_pack_start (GTK_BOX (content_box), preview->info_grid, FALSE, FALSE, 0);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled), content_box);
    gtk_stack_add_named (GTK_STACK (stack), scrolled, "content");

    preview->fallback_label = gtk_label_new (_("No file selected"));
    gtk_style_context_add_class (gtk_widget_get_style_context (preview->fallback_label), "dim-label");
    gtk_stack_add_named (GTK_STACK (stack), preview->fallback_label, "empty");

    gtk_stack_set_visible_child_name (GTK_STACK (stack), "empty");

    gtk_box_pack_start (GTK_BOX (preview), stack, TRUE, TRUE, 0);

    gtk_widget_show_all (stack);
}

GtkWidget *
nolphin_preview_new (void)
{
    return GTK_WIDGET (g_object_new (NOLPHIN_TYPE_PREVIEW, NULL));
}
