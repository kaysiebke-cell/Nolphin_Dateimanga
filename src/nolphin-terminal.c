/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * nolphin-terminal.c: integrated VTE terminal panel (F4)
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

#include "nolphin-terminal.h"

#include <vte/vte.h>
#include <glib/gi18n.h>
#include <unistd.h>

struct _NolphinTerminal
{
    GtkBox parent_instance;

    GtkWidget *vte;
    gboolean spawned;
    gboolean spawn_pending;
    GPid shell_pid;
};

G_DEFINE_TYPE (NolphinTerminal, nolphin_terminal, GTK_TYPE_BOX)

static void
nolphin_terminal_class_init (NolphinTerminalClass *klass)
{
    /* Nothing to override yet; the class exists so future signals/
     * properties (e.g. "shell-exited") have somewhere to live. */
}

static void
child_spawned_cb (VteTerminal *vte,
                   GPid         pid,
                   GError      *error,
                   gpointer     user_data)
{
    NolphinTerminal *terminal = NOLPHIN_TERMINAL (user_data);

    terminal->spawn_pending = FALSE;

    if (error != NULL) {
        gchar *message = g_strdup_printf (_("Failed to start terminal: %s\r\n"), error->message);
        vte_terminal_feed (vte, message, -1);
        g_free (message);
        return;
    }

    terminal->spawned = TRUE;
    terminal->shell_pid = pid;
}

static void
spawn_shell (NolphinTerminal *terminal,
             const gchar     *directory)
{
    gchar *shell;
    gchar *argv[2];

    shell = vte_get_user_shell ();
    if (shell == NULL || shell[0] == '\0') {
        g_free (shell);
        shell = g_strdup (g_getenv ("SHELL"));
    }
    if (shell == NULL || shell[0] == '\0') {
        g_free (shell);
        shell = g_strdup ("/bin/sh");
    }

    argv[0] = shell;
    argv[1] = NULL;

    terminal->spawn_pending = TRUE;

    vte_terminal_spawn_async (VTE_TERMINAL (terminal->vte),
                               VTE_PTY_DEFAULT,
                               directory,
                               argv,
                               NULL, /* envv: inherit ours */
                               G_SPAWN_SEARCH_PATH,
                               NULL, NULL, NULL, /* child_setup */
                               -1,   /* default timeout */
                               NULL, /* cancellable */
                               child_spawned_cb,
                               terminal);

    g_free (shell);
}

gboolean
nolphin_terminal_is_shell_busy (NolphinTerminal *terminal)
{
    VtePty *pty;
    int fd;
    pid_t fg_pgid;

    g_return_val_if_fail (NOLPHIN_IS_TERMINAL (terminal), FALSE);

    if (!terminal->spawned) {
        return FALSE;
    }

    pty = vte_terminal_get_pty (VTE_TERMINAL (terminal->vte));
    if (pty == NULL) {
        return FALSE;
    }

    fd = vte_pty_get_fd (pty);
    if (fd < 0) {
        return FALSE;
    }

    fg_pgid = tcgetpgrp (fd);
    if (fg_pgid == (pid_t) -1) {
        /* Can't tell - better to sync than to get permanently stuck
         * never syncing again. */
        return FALSE;
    }

    return fg_pgid != (pid_t) terminal->shell_pid;
}

void
nolphin_terminal_set_location (NolphinTerminal *terminal,
                                GFile           *location)
{
    gchar *path;
    gchar *quoted;
    gchar *command;

    g_return_if_fail (NOLPHIN_IS_TERMINAL (terminal));

    if (location == NULL) {
        return;
    }

    path = g_file_get_path (location);
    if (path == NULL) {
        /* Remote/virtual location with no local path - a local shell
         * has nowhere to cd into, so leave the terminal as it is. */
        return;
    }

    if (!terminal->spawned) {
        if (!terminal->spawn_pending) {
            spawn_shell (terminal, path);
        }
        g_free (path);
        return;
    }

    if (nolphin_terminal_is_shell_busy (terminal)) {
        g_free (path);
        return;
    }

    quoted = g_shell_quote (path);
    command = g_strdup_printf ("cd %s\n", quoted);
    vte_terminal_feed_child (VTE_TERMINAL (terminal->vte), command, -1);

    g_free (command);
    g_free (quoted);
    g_free (path);
}

void
nolphin_terminal_grab_focus (NolphinTerminal *terminal)
{
    g_return_if_fail (NOLPHIN_IS_TERMINAL (terminal));

    gtk_widget_grab_focus (terminal->vte);
}

static void
nolphin_terminal_init (NolphinTerminal *terminal)
{
    GtkWidget *scrolled;

    gtk_orientable_set_orientation (GTK_ORIENTABLE (terminal), GTK_ORIENTATION_VERTICAL);

    terminal->vte = vte_terminal_new ();
    terminal->spawned = FALSE;
    terminal->spawn_pending = FALSE;
    terminal->shell_pid = 0;

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled), terminal->vte);

    gtk_box_pack_start (GTK_BOX (terminal), scrolled, TRUE, TRUE, 0);

    gtk_widget_show_all (scrolled);
}

GtkWidget *
nolphin_terminal_new (void)
{
    return GTK_WIDGET (g_object_new (NOLPHIN_TYPE_TERMINAL, NULL));
}
