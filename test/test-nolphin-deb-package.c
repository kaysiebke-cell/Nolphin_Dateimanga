/* Exercises the nolphin-deb-package.c backend end to end: creates a
 * small source tree (an executable "script" plus a data file),
 * builds a .deb from it via dpkg-deb, then uses dpkg-deb itself to
 * inspect the result and verifies the control fields, the payload
 * paths and the preserved executable bit. Exits 0 on success, 1 on
 * the first thing that doesn't match, mirroring test-nolphin-archive.c's
 * gtk_main()-driven async style. */

#include <gtk/gtk.h>
#include <libnolphin-private/nolphin-deb-package.h>
#include <glib/gstdio.h>
#include <string.h>

static gint exit_code = 0;
static GFile *work_dir;
static GFile *source_dir;
static GFile *deb_file;

static const gchar *script_name = "run.sh";
static const gchar *script_content = "#!/bin/sh\necho hello\n";
static const gchar *data_name = "data.txt";
static const gchar *data_content = "some payload data";

static void
fail (const gchar *message)
{
    g_printerr ("FAIL: %s\n", message);
    exit_code = 1;
    gtk_main_quit ();
}

static gboolean
run_and_capture (gchar **argv, gchar **out_stdout)
{
    GSubprocess *subprocess;
    GError *error = NULL;
    gboolean ok;

    subprocess = g_subprocess_newv ((const gchar * const *) argv,
                                    G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error);
    if (subprocess == NULL) {
        g_printerr ("could not spawn %s: %s\n", argv[0], error->message);
        g_clear_error (&error);
        return FALSE;
    }

    ok = g_subprocess_communicate_utf8 (subprocess, NULL, NULL, out_stdout, NULL, &error);
    if (!ok) {
        g_printerr ("communicate with %s failed: %s\n", argv[0], error->message);
        g_clear_error (&error);
    } else if (!g_subprocess_get_successful (subprocess)) {
        g_printerr ("%s exited with an error\n", argv[0]);
        ok = FALSE;
    }

    g_object_unref (subprocess);
    return ok;
}

static void
on_build_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
    GError *error = NULL;
    gchar *deb_path;
    gchar *control_out = NULL;
    gchar *contents_out = NULL;
    gchar *argv_control[] = { "dpkg-deb", "-f", NULL, NULL };
    gchar *argv_contents[] = { "dpkg-deb", "-c", NULL, NULL };
    gchar *script_line;

    if (!nolphin_deb_package_build_finish (result, &error)) {
        gchar *msg = g_strdup_printf ("build failed: %s", error->message);
        g_clear_error (&error);
        fail (msg);
        g_free (msg);
        return;
    }

    if (!g_file_query_exists (deb_file, NULL)) {
        fail (".deb file was not created");
        return;
    }

    deb_path = g_file_get_path (deb_file);

    argv_control[2] = deb_path;
    if (!run_and_capture (argv_control, &control_out)) {
        fail ("dpkg-deb -f (control fields) failed");
        g_free (deb_path);
        return;
    }

    if (strstr (control_out, "Package: nolphin-test-paket") == NULL ||
        strstr (control_out, "Version: 1.2.3") == NULL ||
        strstr (control_out, "Architecture: all") == NULL ||
        strstr (control_out, "Maintainer: Test Maintainer <test@example.com>") == NULL ||
        strstr (control_out, "Depends: coreutils") == NULL ||
        strstr (control_out, "Description: Testpaket") == NULL) {
        g_printerr ("unexpected control fields:\n%s\n", control_out);
        fail ("control file content mismatch");
        g_free (control_out);
        g_free (deb_path);
        return;
    }
    g_free (control_out);

    argv_contents[2] = deb_path;
    if (!run_and_capture (argv_contents, &contents_out)) {
        fail ("dpkg-deb -c (payload listing) failed");
        g_free (deb_path);
        return;
    }
    g_free (deb_path);

    if (strstr (contents_out, "./opt/nolphin-test-paket/run.sh") == NULL ||
        strstr (contents_out, "./opt/nolphin-test-paket/data.txt") == NULL) {
        g_printerr ("unexpected payload listing:\n%s\n", contents_out);
        fail ("payload paths mismatch");
        g_free (contents_out);
        return;
    }

    /* "dpkg-deb -c" prints "tar -tv"-style lines, e.g.
     * "-rwxr-xr-x root/root  22 2024-01-01 00:00 ./opt/.../run.sh" -
     * confirms the executable bit survived the copy into the package. */
    script_line = strstr (contents_out, "./opt/nolphin-test-paket/run.sh");
    while (script_line != NULL && script_line > contents_out && *(script_line - 1) != '\n') {
        script_line--;
    }
    if (script_line == NULL || script_line[0] != '-' ||
        (script_line[3] != 'x' && script_line[3] != 's')) {
        g_printerr ("unexpected payload listing:\n%s\n", contents_out);
        fail ("run.sh lost its executable bit inside the package");
        g_free (contents_out);
        return;
    }
    g_free (contents_out);

    g_print ("PASS: build .deb, verify control fields, payload paths and permissions - all OK\n");
    gtk_main_quit ();
}

int
main (int argc, char **argv)
{
    gchar *tmpl;
    GError *error = NULL;
    GFile *script_file, *data_file;
    GList *sources = NULL;
    NolphinDebPackageInfo *info;

    gtk_init (&argc, &argv);

    if (!nolphin_deb_package_is_available ()) {
        g_print ("SKIP: 'dpkg-deb' not installed on this system, cannot exercise the .deb backend\n");
        return 0;
    }

    tmpl = g_dir_make_tmp ("nolphin-deb-package-test-XXXXXX", &error);
    if (tmpl == NULL) {
        g_printerr ("FAIL: could not create temp dir: %s\n", error->message);
        return 1;
    }
    work_dir = g_file_new_for_path (tmpl);
    g_free (tmpl);

    source_dir = g_file_get_child (work_dir, "src");
    deb_file = g_file_get_child (work_dir, "test.deb");

    if (!g_file_make_directory (source_dir, NULL, &error)) {
        g_printerr ("FAIL: could not create source dir: %s\n", error->message);
        return 1;
    }

    script_file = g_file_get_child (source_dir, script_name);
    data_file = g_file_get_child (source_dir, data_name);

    {
        gchar *script_path = g_file_get_path (script_file);
        gchar *data_path = g_file_get_path (data_file);
        gboolean wrote_script = g_file_set_contents (script_path, script_content, -1, &error);
        gboolean wrote_data = wrote_script && g_file_set_contents (data_path, data_content, -1, &error);

        if (wrote_script) {
            g_chmod (script_path, 0755);
        }

        g_free (script_path);
        g_free (data_path);

        if (!wrote_script || !wrote_data) {
            g_printerr ("FAIL: could not write source file: %s\n", error->message);
            return 1;
        }
    }

    sources = g_list_append (sources, script_file);
    sources = g_list_append (sources, data_file);

    info = nolphin_deb_package_info_new ();
    info->package = g_strdup ("nolphin-test-paket");
    info->version = g_strdup ("1.2.3");
    info->architecture = g_strdup ("all");
    info->maintainer = g_strdup ("Test Maintainer <test@example.com>");
    info->description = g_strdup ("Testpaket\nErzeugt vom automatisierten Test.");
    info->section = g_strdup ("utils");
    info->depends = g_strdup ("coreutils");
    info->install_path = g_strdup ("/opt/nolphin-test-paket");

    nolphin_deb_package_build_async (sources, info, deb_file, NULL, on_build_done, NULL);

    gtk_main ();

    nolphin_deb_package_info_free (info);
    g_list_free (sources);
    g_object_unref (script_file);
    g_object_unref (data_file);
    g_object_unref (deb_file);
    g_object_unref (source_dir);
    g_object_unref (work_dir);

    return exit_code;
}
