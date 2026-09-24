/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-properties-window.h - interface for window that lets user modify 
                            icon properties

   Copyright (C) 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.

   Authors: Darin Adler <darin@bentspoon.com>
*/

#ifndef NOLPHIN_PROPERTIES_WINDOW_H
#define NOLPHIN_PROPERTIES_WINDOW_H

#include <gtk/gtk.h>
#include <libnolphin-private/nolphin-file.h>

typedef struct NolphinPropertiesWindow NolphinPropertiesWindow;

#define NOLPHIN_TYPE_PROPERTIES_WINDOW nolphin_properties_window_get_type()
#define NOLPHIN_PROPERTIES_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NOLPHIN_TYPE_PROPERTIES_WINDOW, NolphinPropertiesWindow))
#define NOLPHIN_PROPERTIES_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), NOLPHIN_TYPE_PROPERTIES_WINDOW, NolphinPropertiesWindowClass))
#define NOLPHIN_IS_PROPERTIES_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NOLPHIN_TYPE_PROPERTIES_WINDOW))
#define NOLPHIN_IS_PROPERTIES_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), NOLPHIN_TYPE_PROPERTIES_WINDOW))
#define NOLPHIN_PROPERTIES_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), NOLPHIN_TYPE_PROPERTIES_WINDOW, NolphinPropertiesWindowClass))

typedef struct NolphinPropertiesWindowDetails NolphinPropertiesWindowDetails;

struct NolphinPropertiesWindow {
	GtkDialog window;
	NolphinPropertiesWindowDetails *details;	
};

struct NolphinPropertiesWindowClass {
	GtkDialogClass parent_class;
	
	/* Keybinding signals */
	void (* close)    (NolphinPropertiesWindow *window);
};

typedef struct NolphinPropertiesWindowClass NolphinPropertiesWindowClass;

GType   nolphin_properties_window_get_type   (void);

void 	nolphin_properties_window_present    (GList       *files,
					       GtkWidget   *parent_widget,
					       const gchar *startup_id);

#endif /* NOLPHIN_PROPERTIES_WINDOW_H */
