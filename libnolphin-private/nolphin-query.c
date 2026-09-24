/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Novell, Inc.
 *
 * Nolphin is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nolphin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 * Boston, MA 02110-1335, USA.
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 */

#include <config.h>
#include <string.h>

#include "nolphin-query.h"
#include <eel/eel-glib-extensions.h>
#include <glib/gi18n.h>
#include <libnolphin-private/nolphin-file-utilities.h>

struct NolphinQueryDetails {
    gchar *file_pattern;
    gchar *content_pattern;
    char *location_uri;
    GList *mime_types;
    gboolean show_hidden;
    gboolean file_case_sensitive;
    gboolean file_use_regex;
    gboolean content_case_sensitive;
    gboolean content_use_regex;
    gboolean count_hits;
    gboolean recurse;
};

G_DEFINE_TYPE (NolphinQuery, nolphin_query, G_TYPE_OBJECT);

static void
finalize (GObject *object)
{
	NolphinQuery *query;

	query = NOLPHIN_QUERY (object);
    g_free (query->details->file_pattern);
	g_free (query->details->content_pattern);
	g_free (query->details->location_uri);

	G_OBJECT_CLASS (nolphin_query_parent_class)->finalize (object);
}

static void
nolphin_query_class_init (NolphinQueryClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->finalize = finalize;

	g_type_class_add_private (class, sizeof (NolphinQueryDetails));
}

static void
nolphin_query_init (NolphinQuery *query)
{
	query->details = G_TYPE_INSTANCE_GET_PRIVATE (query, NOLPHIN_TYPE_QUERY,
						      NolphinQueryDetails);
}

NolphinQuery *
nolphin_query_new (void)
{
	return g_object_new (NOLPHIN_TYPE_QUERY,  NULL);
}

char *
nolphin_query_get_file_pattern (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), NULL);

	return g_strdup (query->details->file_pattern);
}

void
nolphin_query_set_file_pattern (NolphinQuery *query, const char *text)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

	g_free (query->details->file_pattern);
	query->details->file_pattern = g_strstrip (g_strdup (text));
}

char *
nolphin_query_get_content_pattern (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), NULL);

    return g_strdup (query->details->content_pattern);
}

void
nolphin_query_set_content_pattern (NolphinQuery *query, const char *text)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    g_clear_pointer (&query->details->content_pattern, g_free);

    if (text && text[0] != '\0') {
        query->details->content_pattern = g_strstrip (g_strdup (text));
    }
}

gboolean
nolphin_query_has_content_pattern (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);

    return query->details->content_pattern != NULL;
}

char *
nolphin_query_get_location (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), NULL);

	return g_strdup (query->details->location_uri);
}

void
nolphin_query_set_location (NolphinQuery *query, const char *uri)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

	g_free (query->details->location_uri);
	query->details->location_uri = g_strdup (uri);
}

GList *
nolphin_query_get_mime_types (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), NULL);

	return eel_g_str_list_copy (query->details->mime_types);
}

void
nolphin_query_set_mime_types (NolphinQuery *query, GList *mime_types)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

	g_list_free_full (query->details->mime_types, g_free);
	query->details->mime_types = eel_g_str_list_copy (mime_types);
}

void
nolphin_query_add_mime_type (NolphinQuery *query, const char *mime_type)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

	query->details->mime_types = g_list_append (query->details->mime_types,
						    g_strdup (mime_type));
}

void
nolphin_query_set_show_hidden (NolphinQuery *query, gboolean hidden)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->show_hidden = hidden;
}

gboolean
nolphin_query_get_show_hidden (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);

    return query->details->show_hidden;
}

char *
nolphin_query_to_readable_string (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), NULL);

    GFile *file;
    gchar *location_title, *readable;

	if (!query || !query->details->file_pattern || query->details->file_pattern[0] == '\0') {
		return g_strdup (_("Suche"));
	}

    file = g_file_new_for_uri (query->details->location_uri);
    location_title = nolphin_compute_search_title_for_location (file);

    g_object_unref (file);

    readable = g_strdup_printf (_("in »%s« suchen"), location_title);

    g_free (location_title);

    return readable;
}

gboolean
nolphin_query_get_file_case_sensitive (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->file_case_sensitive;
}

void
nolphin_query_set_file_case_sensitive (NolphinQuery *query, gboolean case_sensitive)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->file_case_sensitive = case_sensitive;
}

gboolean
nolphin_query_get_content_case_sensitive (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->content_case_sensitive;
}

void
nolphin_query_set_content_case_sensitive (NolphinQuery *query, gboolean case_sensitive)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->content_case_sensitive = case_sensitive;
}

gboolean
nolphin_query_get_use_file_regex (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->file_use_regex;
}

void
nolphin_query_set_use_file_regex (NolphinQuery *query, gboolean file_use_regex)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->file_use_regex = file_use_regex;
}

gboolean
nolphin_query_get_use_content_regex (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->content_use_regex;
}

void
nolphin_query_set_use_content_regex (NolphinQuery *query, gboolean content_use_regex)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->content_use_regex = content_use_regex;
}

gboolean
nolphin_query_get_count_hits (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->count_hits;
}

void
nolphin_query_set_count_hits (NolphinQuery *query, gboolean count_hits)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->count_hits = count_hits;
}

gboolean
nolphin_query_get_recurse (NolphinQuery *query)
{
    g_return_val_if_fail (NOLPHIN_IS_QUERY (query), FALSE);
    return query->details->recurse;
}

void
nolphin_query_set_recurse (NolphinQuery *query, gboolean recurse)
{
    g_return_if_fail (NOLPHIN_IS_QUERY (query));

    query->details->recurse = recurse;
}

