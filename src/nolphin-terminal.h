/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-terminal.h: integrated VTE terminal panel (F4)
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

#ifndef NOLPHIN_TERMINAL_H
#define NOLPHIN_TERMINAL_H

#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define NOLPHIN_TYPE_TERMINAL (nolphin_terminal_get_type ())
G_DECLARE_FINAL_TYPE (NolphinTerminal, nolphin_terminal, NOLPHIN, TERMINAL, GtkBox)

GtkWidget *nolphin_terminal_new (void);

/* Changes the shell's working directory to match the file manager's
 * current location. If the shell is currently running a foreground
 * command, the change is skipped rather than interrupting it - the
 * next navigation (or an explicit call once the shell is idle again)
 * will catch up. Locations without a local path (e.g. a remote URI
 * that isn't FUSE-mounted) are silently ignored, since a local shell
 * has no path to cd into. */
void nolphin_terminal_set_location (NolphinTerminal *terminal,
                                     GFile           *location);

gboolean nolphin_terminal_is_shell_busy (NolphinTerminal *terminal);

void nolphin_terminal_grab_focus (NolphinTerminal *terminal);

G_END_DECLS

#endif /* NOLPHIN_TERMINAL_H */
