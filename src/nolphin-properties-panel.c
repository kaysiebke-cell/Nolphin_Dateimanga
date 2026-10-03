/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-properties-panel.c: Eigenschaften als Seite der rechten
 * Arbeitsleiste.
 *
 * Gleiches Layout wie die Datei-Vorschau (nolphin-preview.c): großes
 * Symbol, Name, dann ein Raster aus grauer Beschriftung und Wert. Darunter
 * folgen die Bereiche "Zugriffsrechte" und "Öffnen mit", die der frühere
 * Eigenschaften-Dialog in eigenen Reitern hatte - hier untereinander statt
 * in einem Fenster im Fenster, ohne Reiter und ohne seitliches Scrollen.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <config.h>

#include "nolphin-properties-panel.h"

#include <glib/gi18n.h>
#include <sys/stat.h>
#include <libnolphin-private/nolphin-file-operations.h>
#include <libnolphin-private/nolphin-acl.h>

#define PANEL_ICON_SIZE 96

#define PERM_RW_BITS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

struct _NolphinPropertiesPanel
{
	GtkBox parent_instance;

	GtkWidget *content;
	GtkWidget *status_label;
	GtkWidget *name_entry;
	GtkWidget *perm_checks[9];

	/* ACL (weitere Benutzer/Gruppen) - ersetzt den früheren eigenen Dialog */
	GFile *acl_file;
	GtkWidget *acl_list;    /* Container für die geladenen Einträge */
	GtkWidget *acl_kind_combo;
	GtkWidget *acl_name_entry;
	GtkWidget *acl_checks[3];
	GtkWidget *acl_recursive_check;
	guint acl_generation;   /* erhöht bei jedem Neuaufbau: verwirft späte Antworten */

	GList *files;           /* NolphinFile*, jeweils referenziert */
	NolphinFile *watched;   /* bei genau einer Datei: für "changed" */
	gulong changed_handler;
	gulong deep_count_handler;
};

G_DEFINE_TYPE (NolphinPropertiesPanel, nolphin_properties_panel, GTK_TYPE_BOX)

static void rebuild (NolphinPropertiesPanel *panel);

/* ---------------------------------------------------------------------- */
/* kleine Helfer                                                          */
/* ---------------------------------------------------------------------- */

static void
panel_set_status (NolphinPropertiesPanel *panel, const gchar *text)
{
	gtk_label_set_text (GTK_LABEL (panel->status_label), text != NULL ? text : "");
}

static GtkWidget *
add_caption (GtkWidget *box, const gchar *text)
{
	GtkWidget *label = gtk_label_new (NULL);
	gchar *markup = g_markup_printf_escaped ("<small><b>%s</b></small>", text);

	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_top (label, 12);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	return label;
}

/* Graue Beschriftung links, markier- und kopierbarer Wert rechts - wie in
 * der Vorschau. Leere Werte werden ausgelassen. */
static void
add_info_row (GtkGrid *grid, gint *row, const gchar *label_text, const gchar *value_text)
{
	GtkWidget *label, *value;

	if (value_text == NULL || value_text[0] == '\0') {
		return;
	}

	label = gtk_label_new (label_text);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_START);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
	gtk_grid_attach (grid, label, 0, *row, 1, 1);

	value = gtk_label_new (value_text);
	gtk_widget_set_halign (value, GTK_ALIGN_START);
	gtk_label_set_xalign (GTK_LABEL (value), 0.0);
	gtk_label_set_line_wrap (GTK_LABEL (value), TRUE);
	gtk_label_set_line_wrap_mode (GTK_LABEL (value), PANGO_WRAP_WORD_CHAR);
	gtk_label_set_selectable (GTK_LABEL (value), TRUE);
	gtk_grid_attach (grid, value, 1, *row, 1, 1);

	(*row)++;
}

static GtkWidget *
new_info_grid (GtkWidget *box)
{
	GtkWidget *grid = gtk_grid_new ();

	gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);

	return grid;
}

/* Rückmeldung von Umbenennen/Rechte/Besitzer: Fehler im Statusfeld zeigen. */
static void
file_op_done (NolphinFile *file, GFile *result_location, GError *error, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;

	if (error != NULL) {
		panel_set_status (panel, error->message);
	} else {
		panel_set_status (panel, NULL);
	}
	g_object_unref (panel);
}

/* ---------------------------------------------------------------------- */
/* Umbenennen                                                             */
/* ---------------------------------------------------------------------- */

static void
on_name_activate (GtkEntry *entry, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinFile *file = panel->files != NULL ? panel->files->data : NULL;
	const gchar *original = g_object_get_data (G_OBJECT (entry), "original-name");
	const gchar *new_name = gtk_entry_get_text (entry);

	if (file == NULL || new_name == NULL || new_name[0] == '\0' || g_strcmp0 (new_name, original) == 0) {
		return;
	}

	panel_set_status (panel, _("Wird umbenannt …"));
	nolphin_file_rename (file, new_name, file_op_done, g_object_ref (panel));
}

/* ---------------------------------------------------------------------- */
/* Zugriffsrechte                                                         */
/* ---------------------------------------------------------------------- */

static const struct {
	const gchar *label;
	guint bits[3];
} perm_rows[] = {
	{ N_("Eigentümer"), { S_IRUSR, S_IWUSR, S_IXUSR } },
	{ N_("Gruppe"),     { S_IRGRP, S_IWGRP, S_IXGRP } },
	{ N_("Andere"),     { S_IROTH, S_IWOTH, S_IXOTH } },
};

static void
on_perm_toggled (GtkToggleButton *button, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinFile *file = panel->files != NULL ? panel->files->data : NULL;
	guint perms;
	gint i;

	if (file == NULL) {
		return;
	}

	/* Sonderbits (setuid/setgid/sticky) bleiben unverändert. */
	perms = nolphin_file_get_permissions (file) & ~0777u;
	for (i = 0; i < 9; i++) {
		if (panel->perm_checks[i] != NULL &&
		    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->perm_checks[i]))) {
			perms |= GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (panel->perm_checks[i]), "bit"));
		}
	}

	nolphin_file_set_permissions (file, perms, file_op_done, g_object_ref (panel));
}

static void
on_owner_changed (GtkComboBox *combo, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinFile *file = panel->files != NULL ? panel->files->data : NULL;
	const gchar *id = gtk_combo_box_get_active_id (combo);

	if (file != NULL && id != NULL) {
		nolphin_file_set_owner (file, id, file_op_done, g_object_ref (panel));
	}
}

static void
on_group_changed (GtkComboBox *combo, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinFile *file = panel->files != NULL ? panel->files->data : NULL;
	const gchar *id = gtk_combo_box_get_active_id (combo);

	if (file != NULL && id != NULL) {
		nolphin_file_set_group (file, id, file_op_done, g_object_ref (panel));
	}
}

static void
recursive_done (gboolean success, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;

	panel_set_status (panel, success ? _("Rechte auf den Inhalt angewendet.")
					 : _("Rechte konnten nicht auf den gesamten Inhalt angewendet werden."));
	g_object_unref (panel);
}

/* Überträgt nur Lesen/Schreiben auf alle enthaltenen Dateien und Ordner;
 * Ausführen-Bits der einzelnen Objekte bleiben, wie sie sind. */
static void
on_apply_to_contents_clicked (GtkButton *button, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinFile *file = panel->files != NULL ? panel->files->data : NULL;
	gchar *uri;
	guint32 perms;

	if (file == NULL) {
		return;
	}

	uri = nolphin_file_get_uri (file);
	perms = nolphin_file_get_permissions (file) & PERM_RW_BITS;

	panel_set_status (panel, _("Wird angewendet …"));
	nolphin_file_set_permissions_recursive (uri, perms, PERM_RW_BITS, perms, PERM_RW_BITS,
						recursive_done, g_object_ref (panel));
	g_free (uri);
}

/* Fügt ein Auswahlfeld mit Namen hinzu; Einträge sind "Name" oder
 * "Name\nKlarname" (so liefert es nolphin_get_user_names()). */
static GtkWidget *
new_name_combo (GList *names, const gchar *current)
{
	GtkWidget *combo = gtk_combo_box_text_new ();
	gboolean found = FALSE;
	GList *l;

	for (l = names; l != NULL; l = l->next) {
		gchar **parts = g_strsplit ((const gchar *) l->data, "\n", 2);
		gchar *text = (parts[1] != NULL) ? g_strdup_printf ("%s - %s", parts[0], parts[1])
						 : g_strdup (parts[0]);

		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), parts[0], text);
		if (g_strcmp0 (parts[0], current) == 0) {
			found = TRUE;
		}
		g_free (text);
		g_strfreev (parts);
	}

	if (!found && current != NULL) {
		gtk_combo_box_text_prepend (GTK_COMBO_BOX_TEXT (combo), current, current);
	}
	if (current != NULL) {
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (combo), current);
	}
	gtk_widget_set_hexpand (combo, TRUE);

	return combo;
}

static void
build_permissions (NolphinPropertiesPanel *panel, NolphinFile *file)
{
	GtkWidget *grid, *label, *combo, *box;
	gboolean can_set;
	guint perms;
	gint r, c, row = 0;
	static const gchar *col_titles[3] = { N_("Lesen"), N_("Schreiben"), N_("Ausführen") };

	if (!nolphin_file_can_get_permissions (file)) {
		return;
	}

	box = panel->content;
	add_caption (box, _("Zugriffsrechte"));

	perms = nolphin_file_get_permissions (file);
	can_set = nolphin_file_can_set_permissions (file);

	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);

	for (c = 0; c < 3; c++) {
		label = gtk_label_new (_(col_titles[c]));
		gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
		gtk_grid_attach (GTK_GRID (grid), label, c + 1, row, 1, 1);
	}
	row++;

	for (r = 0; r < 3; r++) {
		label = gtk_label_new (_(perm_rows[r].label));
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

		for (c = 0; c < 3; c++) {
			GtkWidget *check = gtk_check_button_new ();

			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), (perms & perm_rows[r].bits[c]) != 0);
			gtk_widget_set_sensitive (check, can_set);
			gtk_widget_set_halign (check, GTK_ALIGN_CENTER);
			g_object_set_data (G_OBJECT (check), "bit", GUINT_TO_POINTER (perm_rows[r].bits[c]));
			g_signal_connect (check, "toggled", G_CALLBACK (on_perm_toggled), panel);
			gtk_grid_attach (GTK_GRID (grid), check, c + 1, row, 1, 1);
			panel->perm_checks[r * 3 + c] = check;
		}
		row++;
	}

	/* Besitzer und Gruppe */
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_widget_set_margin_top (grid, 8);
	gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);

	if (nolphin_file_can_get_owner (file)) {
		gchar *current = nolphin_file_get_owner_name (file);
		GList *users = nolphin_get_user_names ();

		label = gtk_label_new (_("Besitzer"));
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
		gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

		combo = new_name_combo (users, current);
		gtk_widget_set_sensitive (combo, nolphin_file_can_set_owner (file));
		g_signal_connect (combo, "changed", G_CALLBACK (on_owner_changed), panel);
		gtk_grid_attach (GTK_GRID (grid), combo, 1, 0, 1, 1);

		g_list_free_full (users, g_free);
		g_free (current);
	}

	if (nolphin_file_can_get_group (file)) {
		gchar *current = nolphin_file_get_group_name (file);
		GList *groups = nolphin_file_get_settable_group_names (file);

		label = gtk_label_new (_("Gruppe"));
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
		gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

		combo = new_name_combo (groups, current);
		gtk_widget_set_sensitive (combo, nolphin_file_can_set_group (file));
		g_signal_connect (combo, "changed", G_CALLBACK (on_group_changed), panel);
		gtk_grid_attach (GTK_GRID (grid), combo, 1, 1, 1, 1);

		g_list_free_full (groups, g_free);
		g_free (current);
	}

	if (nolphin_file_is_directory (file) && can_set) {
		GtkWidget *button = gtk_button_new_with_label (_("Lesen/Schreiben auf Inhalt anwenden"));

		gtk_widget_set_halign (button, GTK_ALIGN_START);
		gtk_widget_set_margin_top (button, 8);
		gtk_widget_set_tooltip_text (button,
					     _("Überträgt die Lese- und Schreibrechte dieses Ordners auf alle enthaltenen Dateien und Ordner"));
		g_signal_connect (button, "clicked", G_CALLBACK (on_apply_to_contents_clicked), panel);
		gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	}
}

/* ---------------------------------------------------------------------- */
/* Weitere Benutzer und Gruppen (ACL)                                     */
/* ---------------------------------------------------------------------- */

typedef struct {
	NolphinPropertiesPanel *panel;  /* referenziert */
	guint generation;
} AclContext;

static AclContext *
acl_context_new (NolphinPropertiesPanel *panel)
{
	AclContext *ctx = g_new0 (AclContext, 1);

	ctx->panel = g_object_ref (panel);
	ctx->generation = panel->acl_generation;
	return ctx;
}

static void
acl_context_free (AclContext *ctx)
{
	g_object_unref (ctx->panel);
	g_free (ctx);
}

static void acl_load (NolphinPropertiesPanel *panel);

static gboolean
acl_recursive_wanted (NolphinPropertiesPanel *panel)
{
	return panel->acl_recursive_check != NULL &&
	       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->acl_recursive_check));
}

static void
acl_set_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
	AclContext *ctx = user_data;
	GError *error = NULL;
	gboolean ok = nolphin_acl_set_entry_finish (result, &error);

	if (ctx->generation == ctx->panel->acl_generation) {
		panel_set_status (ctx->panel, ok ? NULL : error->message);
		acl_load (ctx->panel);
	}
	g_clear_error (&error);
	acl_context_free (ctx);
}

static void
acl_remove_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
	AclContext *ctx = user_data;
	GError *error = NULL;
	gboolean ok = nolphin_acl_remove_entry_finish (result, &error);

	if (ctx->generation == ctx->panel->acl_generation) {
		panel_set_status (ctx->panel, ok ? NULL : error->message);
		acl_load (ctx->panel);
	}
	g_clear_error (&error);
	acl_context_free (ctx);
}

/* Häkchen einer bestehenden Zeile geändert: Eintrag mit allen drei
 * aktuellen Werten neu setzen. */
static void
on_acl_row_toggled (GtkToggleButton *button, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	GtkWidget **checks = g_object_get_data (G_OBJECT (button), "row-checks");
	NolphinAclEntryType type = (NolphinAclEntryType) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "entry-type"));
	const gchar *qualifier = g_object_get_data (G_OBJECT (button), "qualifier");

	if (panel->acl_file == NULL || checks == NULL) {
		return;
	}

	nolphin_acl_set_entry_async (panel->acl_file, type, qualifier,
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checks[0])),
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checks[1])),
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checks[2])),
				     acl_recursive_wanted (panel), NULL, acl_set_done, acl_context_new (panel));
}

static void
on_acl_remove_clicked (GtkButton *button, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	NolphinAclEntryType type = (NolphinAclEntryType) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "entry-type"));
	const gchar *qualifier = g_object_get_data (G_OBJECT (button), "qualifier");

	if (panel->acl_file == NULL) {
		return;
	}

	nolphin_acl_remove_entry_async (panel->acl_file, type, qualifier, acl_recursive_wanted (panel),
					NULL, acl_remove_done, acl_context_new (panel));
}

static void
acl_loaded (GObject *source, GAsyncResult *result, gpointer user_data)
{
	AclContext *ctx = user_data;
	NolphinPropertiesPanel *panel = ctx->panel;
	GError *error = NULL;
	GList *entries = nolphin_acl_get_entries_finish (result, &error);
	GList *children, *l;
	GtkWidget *grid = NULL;
	gint row = 1, c;

	if (ctx->generation != panel->acl_generation || panel->acl_list == NULL) {
		nolphin_acl_entry_list_free (entries);
		g_clear_error (&error);
		acl_context_free (ctx);
		return;
	}

	children = gtk_container_get_children (GTK_CONTAINER (panel->acl_list));
	for (l = children; l != NULL; l = l->next) {
		gtk_widget_destroy (GTK_WIDGET (l->data));
	}
	g_list_free (children);

	if (entries == NULL && error != NULL) {
		panel_set_status (panel, error->message);
		g_clear_error (&error);
		acl_context_free (ctx);
		return;
	}

	for (l = entries; l != NULL; l = l->next) {
		NolphinAclEntry *e = l->data;
		GtkWidget *label, *remove_button;
		GtkWidget **checks;
		gchar *text;
		const gboolean bits[3] = { e->can_read, e->can_write, e->can_execute };

		/* Eigentümer, Gruppe und Andere stehen schon oben im Raster. */
		if (e->type != NOLPHIN_ACL_ENTRY_USER && e->type != NOLPHIN_ACL_ENTRY_GROUP) {
			continue;
		}

		if (grid == NULL) {
			static const gchar *titles[3] = { N_("Lesen"), N_("Schreiben"), N_("Ausführen") };

			grid = gtk_grid_new ();
			gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
			gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
			for (c = 0; c < 3; c++) {
				GtkWidget *title = gtk_label_new (_(titles[c]));

				gtk_style_context_add_class (gtk_widget_get_style_context (title), "dim-label");
				gtk_grid_attach (GTK_GRID (grid), title, c + 1, 0, 1, 1);
			}
			gtk_box_pack_start (GTK_BOX (panel->acl_list), grid, FALSE, FALSE, 0);
		}

		text = g_strdup_printf ("%s %s",
					e->type == NOLPHIN_ACL_ENTRY_USER ? _("Benutzer") : _("Gruppe"),
					e->qualifier != NULL ? e->qualifier : "");
		label = gtk_label_new (text);
		g_free (text);
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
		gtk_widget_set_hexpand (label, TRUE);
		gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

		checks = g_new0 (GtkWidget *, 3);
		for (c = 0; c < 3; c++) {
			checks[c] = gtk_check_button_new ();
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checks[c]), bits[c]);
			gtk_widget_set_halign (checks[c], GTK_ALIGN_CENTER);
			gtk_grid_attach (GTK_GRID (grid), checks[c], c + 1, row, 1, 1);
		}
		for (c = 0; c < 3; c++) {
			g_object_set_data (G_OBJECT (checks[c]), "row-checks", checks);
			g_object_set_data (G_OBJECT (checks[c]), "entry-type", GINT_TO_POINTER (e->type));
			g_object_set_data_full (G_OBJECT (checks[c]), "qualifier", g_strdup (e->qualifier), g_free);
			g_signal_connect (checks[c], "toggled", G_CALLBACK (on_acl_row_toggled), panel);
		}
		/* checks[] gehört der Zeile: mit dem ersten Häkchen freigeben. */
		g_object_set_data_full (G_OBJECT (checks[0]), "row-checks-owner", checks, g_free);

		remove_button = gtk_button_new_from_icon_name ("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_relief (GTK_BUTTON (remove_button), GTK_RELIEF_NONE);
		gtk_widget_set_tooltip_text (remove_button, _("Diesen Eintrag entfernen"));
		g_object_set_data (G_OBJECT (remove_button), "entry-type", GINT_TO_POINTER (e->type));
		g_object_set_data_full (G_OBJECT (remove_button), "qualifier", g_strdup (e->qualifier), g_free);
		g_signal_connect (remove_button, "clicked", G_CALLBACK (on_acl_remove_clicked), panel);
		gtk_grid_attach (GTK_GRID (grid), remove_button, 4, row, 1, 1);

		row++;
	}

	if (grid == NULL) {
		GtkWidget *none = gtk_label_new (_("Keine weiteren Benutzer oder Gruppen eingetragen."));

		gtk_label_set_xalign (GTK_LABEL (none), 0.0);
		gtk_style_context_add_class (gtk_widget_get_style_context (none), "dim-label");
		gtk_box_pack_start (GTK_BOX (panel->acl_list), none, FALSE, FALSE, 0);
	}

	gtk_widget_show_all (panel->acl_list);
	nolphin_acl_entry_list_free (entries);
	acl_context_free (ctx);
}

static void
acl_load (NolphinPropertiesPanel *panel)
{
	if (panel->acl_file == NULL) {
		return;
	}
	nolphin_acl_get_entries_async (panel->acl_file, NULL, acl_loaded, acl_context_new (panel));
}

static void
on_acl_add_clicked (GtkButton *button, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;
	const gchar *name = gtk_entry_get_text (GTK_ENTRY (panel->acl_name_entry));
	NolphinAclEntryType type;

	if (panel->acl_file == NULL) {
		return;
	}
	if (name == NULL || name[0] == '\0') {
		panel_set_status (panel, _("Bitte einen Benutzer- oder Gruppennamen eingeben."));
		return;
	}

	type = (gtk_combo_box_get_active (GTK_COMBO_BOX (panel->acl_kind_combo)) == 0)
		? NOLPHIN_ACL_ENTRY_USER : NOLPHIN_ACL_ENTRY_GROUP;

	nolphin_acl_set_entry_async (panel->acl_file, type, name,
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->acl_checks[0])),
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->acl_checks[1])),
				     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->acl_checks[2])),
				     acl_recursive_wanted (panel), NULL, acl_set_done, acl_context_new (panel));
	gtk_entry_set_text (GTK_ENTRY (panel->acl_name_entry), "");
}

/* Erweiterte Rechte für einzelne weitere Benutzer/Gruppen. Erscheint nur,
 * wenn setfacl/getfacl vorhanden sind und die Datei lokal liegt. */
static void
build_acl (NolphinPropertiesPanel *panel, NolphinFile *file)
{
	GtkWidget *box = panel->content;
	GtkWidget *hint, *form, *row, *button;
	gint c;
	static const gchar *check_labels[3] = { N_("Lesen"), N_("Schreiben"), N_("Ausführen") };

	if (!nolphin_acl_is_available ()) {
		return;
	}

	panel->acl_file = nolphin_file_get_location (file);
	{
		gchar *path = (panel->acl_file != NULL) ? g_file_get_path (panel->acl_file) : NULL;

		if (path == NULL) {
			g_clear_object (&panel->acl_file);
			return;
		}
		g_free (path);
	}

	add_caption (box, _("Weitere Benutzer und Gruppen"));

	hint = gtk_label_new (_("Zusätzliche Rechte für einzelne Personen oder Gruppen, über die oben genannten hinaus (ACL)."));
	gtk_label_set_line_wrap (GTK_LABEL (hint), TRUE);
	gtk_label_set_xalign (GTK_LABEL (hint), 0.0);
	gtk_style_context_add_class (gtk_widget_get_style_context (hint), "dim-label");
	gtk_box_pack_start (GTK_BOX (box), hint, FALSE, FALSE, 0);

	panel->acl_list = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_set_margin_top (panel->acl_list, 4);
	gtk_box_pack_start (GTK_BOX (box), panel->acl_list, FALSE, FALSE, 0);

	/* Formular zum Hinzufügen */
	form = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin_top (form, 6);
	gtk_box_pack_start (GTK_BOX (box), form, FALSE, FALSE, 0);

	row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (form), row, FALSE, FALSE, 0);

	panel->acl_kind_combo = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (panel->acl_kind_combo), _("Benutzer"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (panel->acl_kind_combo), _("Gruppe"));
	gtk_combo_box_set_active (GTK_COMBO_BOX (panel->acl_kind_combo), 0);
	gtk_box_pack_start (GTK_BOX (row), panel->acl_kind_combo, FALSE, FALSE, 0);

	panel->acl_name_entry = gtk_entry_new ();
	gtk_entry_set_placeholder_text (GTK_ENTRY (panel->acl_name_entry), _("Name"));
	gtk_box_pack_start (GTK_BOX (row), panel->acl_name_entry, TRUE, TRUE, 0);

	row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start (GTK_BOX (form), row, FALSE, FALSE, 0);
	for (c = 0; c < 3; c++) {
		panel->acl_checks[c] = gtk_check_button_new_with_label (_(check_labels[c]));
		gtk_box_pack_start (GTK_BOX (row), panel->acl_checks[c], FALSE, FALSE, 0);
	}

	button = gtk_button_new_with_label (_("Hinzufügen"));
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name ("list-add-symbolic", GTK_ICON_SIZE_BUTTON));
	gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
	g_signal_connect (button, "clicked", G_CALLBACK (on_acl_add_clicked), panel);
	gtk_box_pack_end (GTK_BOX (row), button, FALSE, FALSE, 0);

	if (nolphin_file_is_directory (file)) {
		panel->acl_recursive_check = gtk_check_button_new_with_label (_("Auch auf Inhalt anwenden"));
		gtk_box_pack_start (GTK_BOX (form), panel->acl_recursive_check, FALSE, FALSE, 0);
	}

	acl_load (panel);
}

/* ---------------------------------------------------------------------- */
/* Öffnen mit                                                             */
/* ---------------------------------------------------------------------- */

static void
on_set_default_app_clicked (GtkButton *button, gpointer user_data)
{
	GtkComboBox *combo = GTK_COMBO_BOX (g_object_get_data (G_OBJECT (button), "combo"));
	NolphinPropertiesPanel *panel = user_data;
	const gchar *mime = g_object_get_data (G_OBJECT (button), "mime");
	const gchar *id = gtk_combo_box_get_active_id (combo);
	GList *apps = g_object_get_data (G_OBJECT (combo), "apps");
	GList *l;

	for (l = apps; l != NULL && id != NULL; l = l->next) {
		GAppInfo *app = l->data;

		if (g_strcmp0 (g_app_info_get_id (app), id) == 0) {
			GError *error = NULL;

			if (g_app_info_set_as_default_for_type (app, mime, &error)) {
				gchar *msg = g_strdup_printf (_("%s ist jetzt Standard für diesen Dateityp."),
							      g_app_info_get_display_name (app));
				panel_set_status (panel, msg);
				g_free (msg);
			} else {
				panel_set_status (panel, error->message);
				g_clear_error (&error);
			}
			return;
		}
	}
}

static void
free_app_list (GList *apps)
{
	g_list_free_full (apps, g_object_unref);
}

static void
build_open_with (NolphinPropertiesPanel *panel, NolphinFile *file)
{
	GtkWidget *box = panel->content;
	GtkWidget *row, *combo, *button;
	gchar *mime;
	GList *apps, *l;
	GAppInfo *current;

	if (nolphin_file_is_directory (file)) {
		return;
	}

	mime = nolphin_file_get_mime_type (file);
	if (mime == NULL) {
		return;
	}

	add_caption (box, _("Öffnen mit"));

	apps = g_app_info_get_all_for_type (mime);
	if (apps == NULL) {
		GtkWidget *none = gtk_label_new (_("Keine passende Anwendung gefunden."));

		gtk_label_set_xalign (GTK_LABEL (none), 0.0);
		gtk_style_context_add_class (gtk_widget_get_style_context (none), "dim-label");
		gtk_box_pack_start (GTK_BOX (box), none, FALSE, FALSE, 0);
		g_free (mime);
		return;
	}

	row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (box), row, FALSE, FALSE, 0);

	combo = gtk_combo_box_text_new ();
	gtk_widget_set_hexpand (combo, TRUE);
	for (l = apps; l != NULL; l = l->next) {
		GAppInfo *app = l->data;

		if (g_app_info_get_id (app) != NULL) {
			gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), g_app_info_get_id (app),
						   g_app_info_get_display_name (app));
		}
	}
	current = g_app_info_get_default_for_type (mime, FALSE);
	if (current != NULL && g_app_info_get_id (current) != NULL) {
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (combo), g_app_info_get_id (current));
	}
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) < 0) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	}
	g_clear_object (&current);
	g_object_set_data_full (G_OBJECT (combo), "apps", apps, (GDestroyNotify) free_app_list);
	gtk_box_pack_start (GTK_BOX (row), combo, TRUE, TRUE, 0);

	button = gtk_button_new_with_label (_("Als Standard"));
	gtk_widget_set_tooltip_text (button, _("Macht die gewählte Anwendung zum Standard für diesen Dateityp"));
	g_object_set_data (G_OBJECT (button), "combo", combo);
	g_object_set_data_full (G_OBJECT (button), "mime", mime, g_free);
	g_signal_connect (button, "clicked", G_CALLBACK (on_set_default_app_clicked), panel);
	gtk_box_pack_start (GTK_BOX (row), button, FALSE, FALSE, 0);
}

/* ---------------------------------------------------------------------- */
/* Aufbau                                                                 */
/* ---------------------------------------------------------------------- */

static void
build_single (NolphinPropertiesPanel *panel, NolphinFile *file)
{
	GtkWidget *box = panel->content;
	GtkWidget *image, *grid;
	GdkPixbuf *pixbuf;
	gchar *text;
	gint row = 0;
	gboolean is_dir = nolphin_file_is_directory (file);

	pixbuf = nolphin_file_get_icon_pixbuf (file, PANEL_ICON_SIZE, FALSE,
					       gtk_widget_get_scale_factor (GTK_WIDGET (panel)),
					       NOLPHIN_FILE_ICON_FLAGS_USE_THUMBNAILS);
	image = gtk_image_new ();
	if (pixbuf != NULL) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
		g_object_unref (pixbuf);
	}
	gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);

	text = nolphin_file_get_display_name (file);
	panel->name_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (panel->name_entry), text);
	gtk_entry_set_alignment (GTK_ENTRY (panel->name_entry), 0.5);
	g_object_set_data_full (G_OBJECT (panel->name_entry), "original-name", text, g_free);
	if (nolphin_file_can_rename (file)) {
		gtk_widget_set_tooltip_text (panel->name_entry, _("Name ändern und Eingabetaste drücken"));
		g_signal_connect (panel->name_entry, "activate", G_CALLBACK (on_name_activate), panel);
	} else {
		gtk_editable_set_editable (GTK_EDITABLE (panel->name_entry), FALSE);
	}
	gtk_box_pack_start (GTK_BOX (box), panel->name_entry, FALSE, FALSE, 0);

	add_caption (box, _("Allgemein"));
	grid = new_info_grid (box);

	text = nolphin_file_get_string_attribute (file, "type");
	add_info_row (GTK_GRID (grid), &row, _("Typ"), text);
	g_free (text);

	text = is_dir ? nolphin_file_get_string_attribute (file, "deep_size")
		      : (nolphin_file_get_size (file) >= 0 ? nolphin_file_get_string_attribute (file, "size") : NULL);
	add_info_row (GTK_GRID (grid), &row, _("Größe"), text);
	g_free (text);

	text = nolphin_file_get_string_attribute (file, "where");
	add_info_row (GTK_GRID (grid), &row, _("Ort"), text);
	g_free (text);

	text = nolphin_file_get_string_attribute (file, "date_modified_full");
	add_info_row (GTK_GRID (grid), &row, _("Geändert"), text);
	g_free (text);

	text = nolphin_file_get_string_attribute (file, "date_accessed_full");
	add_info_row (GTK_GRID (grid), &row, _("Zugegriffen"), text);
	g_free (text);

	text = nolphin_file_get_symbolic_link_target_path (file);
	add_info_row (GTK_GRID (grid), &row, _("Verknüpfungsziel"), text);
	g_free (text);

	build_permissions (panel, file);
	build_acl (panel, file);
	build_open_with (panel, file);

	if (is_dir) {
		nolphin_file_recompute_deep_counts (file);
	}
}

static void
build_multi (NolphinPropertiesPanel *panel, GList *files)
{
	GtkWidget *box = panel->content;
	GtkWidget *label, *grid;
	goffset total = 0;
	guint count = g_list_length (files), dirs = 0;
	gint row = 0;
	gchar *text;
	GList *l;

	for (l = files; l != NULL; l = l->next) {
		NolphinFile *f = l->data;

		if (nolphin_file_is_directory (f)) {
			dirs++;
		} else if (nolphin_file_get_size (f) >= 0) {
			total += nolphin_file_get_size (f);
		}
	}

	text = g_strdup_printf (_("%u Objekte ausgewählt"), count);
	label = gtk_label_new (text);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "heading");
	gtk_widget_set_margin_top (label, 24);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	g_free (text);

	add_caption (box, _("Allgemein"));
	grid = new_info_grid (box);

	text = g_strdup_printf ("%u", count - dirs);
	add_info_row (GTK_GRID (grid), &row, _("Dateien"), text);
	g_free (text);

	if (dirs > 0) {
		text = g_strdup_printf ("%u", dirs);
		add_info_row (GTK_GRID (grid), &row, _("Ordner"), text);
		g_free (text);
	}

	text = g_format_size (total);
	add_info_row (GTK_GRID (grid), &row, _("Größe der Dateien"), text);
	g_free (text);

	text = nolphin_file_get_string_attribute (NOLPHIN_FILE (files->data), "where");
	add_info_row (GTK_GRID (grid), &row, _("Ort"), text);
	g_free (text);

	label = gtk_label_new (_("Wähle ein einzelnes Objekt, um Name, Zugriffsrechte und „Öffnen mit“ zu bearbeiten."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
	gtk_widget_set_margin_top (label, 12);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
}

static void
rebuild (NolphinPropertiesPanel *panel)
{
	GList *children, *l;

	children = gtk_container_get_children (GTK_CONTAINER (panel->content));
	for (l = children; l != NULL; l = l->next) {
		gtk_widget_destroy (GTK_WIDGET (l->data));
	}
	g_list_free (children);

	panel->acl_generation++;
	panel->acl_list = NULL;
	panel->acl_kind_combo = NULL;
	panel->acl_name_entry = NULL;
	panel->acl_recursive_check = NULL;
	memset (panel->acl_checks, 0, sizeof (panel->acl_checks));
	g_clear_object (&panel->acl_file);
	panel->name_entry = NULL;
	memset (panel->perm_checks, 0, sizeof (panel->perm_checks));

	if (panel->files == NULL) {
		GtkWidget *label = gtk_label_new (_("Nichts ausgewählt."));

		gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
		gtk_widget_set_margin_top (label, 24);
		gtk_box_pack_start (GTK_BOX (panel->content), label, FALSE, FALSE, 0);
	} else if (panel->files->next == NULL) {
		build_single (panel, NOLPHIN_FILE (panel->files->data));
	} else {
		build_multi (panel, panel->files);
	}

	gtk_widget_show_all (panel->content);
}

static void
on_file_changed (NolphinFile *file, gpointer user_data)
{
	NolphinPropertiesPanel *panel = user_data;

	/* Nicht mitten in der Namenseingabe neu aufbauen. */
	if (panel->name_entry != NULL && gtk_widget_has_focus (panel->name_entry)) {
		return;
	}
	rebuild (panel);
}

static void
stop_watching (NolphinPropertiesPanel *panel)
{
	if (panel->watched != NULL) {
		if (panel->changed_handler != 0) {
			g_signal_handler_disconnect (panel->watched, panel->changed_handler);
			panel->changed_handler = 0;
		}
		if (panel->deep_count_handler != 0) {
			g_signal_handler_disconnect (panel->watched, panel->deep_count_handler);
			panel->deep_count_handler = 0;
		}
		nolphin_file_unref (panel->watched);
		panel->watched = NULL;
	}
}

void
nolphin_properties_panel_set_files (NolphinPropertiesPanel *panel, GList *files)
{
	GList *l;

	g_return_if_fail (NOLPHIN_IS_PROPERTIES_PANEL (panel));

	stop_watching (panel);
	g_list_free_full (panel->files, (GDestroyNotify) nolphin_file_unref);
	panel->files = NULL;

	for (l = files; l != NULL; l = l->next) {
		panel->files = g_list_prepend (panel->files, nolphin_file_ref (NOLPHIN_FILE (l->data)));
	}
	panel->files = g_list_reverse (panel->files);

	if (panel->files != NULL && panel->files->next == NULL) {
		panel->watched = nolphin_file_ref (NOLPHIN_FILE (panel->files->data));
		panel->changed_handler = g_signal_connect (panel->watched, "changed",
							   G_CALLBACK (on_file_changed), panel);
		/* Ordnergröße wird im Hintergrund berechnet und trifft später ein. */
		panel->deep_count_handler = g_signal_connect (panel->watched, "updated_deep_count_in_progress",
							      G_CALLBACK (on_file_changed), panel);
	}

	panel_set_status (panel, NULL);
	rebuild (panel);
}

/* ---------------------------------------------------------------------- */

static void
nolphin_properties_panel_dispose (GObject *object)
{
	NolphinPropertiesPanel *panel = NOLPHIN_PROPERTIES_PANEL (object);

	stop_watching (panel);
	panel->acl_generation++;
	g_clear_object (&panel->acl_file);
	g_list_free_full (panel->files, (GDestroyNotify) nolphin_file_unref);
	panel->files = NULL;

	G_OBJECT_CLASS (nolphin_properties_panel_parent_class)->dispose (object);
}

static void
nolphin_properties_panel_class_init (NolphinPropertiesPanelClass *klass)
{
	G_OBJECT_CLASS (klass)->dispose = nolphin_properties_panel_dispose;
}

static void
nolphin_properties_panel_init (NolphinPropertiesPanel *panel)
{
	GtkWidget *scroller;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (panel), GTK_ORIENTATION_VERTICAL);

	scroller = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (panel), scroller, TRUE, TRUE, 0);

	panel->content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (panel->content), 12);
	gtk_container_add (GTK_CONTAINER (scroller), panel->content);

	panel->status_label = gtk_label_new ("");
	gtk_label_set_xalign (GTK_LABEL (panel->status_label), 0.0);
	gtk_label_set_line_wrap (GTK_LABEL (panel->status_label), TRUE);
	gtk_widget_set_margin_start (panel->status_label, 12);
	gtk_widget_set_margin_end (panel->status_label, 12);
	gtk_widget_set_margin_bottom (panel->status_label, 8);
	gtk_box_pack_start (GTK_BOX (panel), panel->status_label, FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET (panel));
}

GtkWidget *
nolphin_properties_panel_new (void)
{
	return g_object_new (NOLPHIN_TYPE_PROPERTIES_PANEL, NULL);
}
