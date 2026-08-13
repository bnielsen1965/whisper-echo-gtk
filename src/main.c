#include <adwaita.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include "window.h"
#include "config.h"
#include "process.h"

static void load_css(AdwApplication *app) {
    (void)app;
    /* Try to load CSS from data directory next to binary, or from install prefix */
    const char *css_paths[] = {
        "data/style.css",
        "../data/style.css",
        "/usr/local/share/whisper-echo-gtk/style.css",
        "/usr/share/whisper-echo-gtk/style.css",
        NULL
    };

    for (int i = 0; css_paths[i] != NULL; i++) {
        if (g_file_test(css_paths[i], G_FILE_TEST_EXISTS)) {
            gchar *css = NULL;
            GError *error = NULL;

            if (g_file_get_contents(css_paths[i], &css, NULL, &error)) {
                GtkCssProvider *provider = gtk_css_provider_new();
                gtk_css_provider_load_from_string(provider, css);
                gtk_style_context_add_provider_for_display(
                    gdk_display_get_default(),
                    GTK_STYLE_PROVIDER(provider),
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
                );
                g_object_unref(provider);
                g_free(css);
            } else {
                fprintf(stderr, "Failed to read CSS file %s: %s\n", css_paths[i], error->message);
                g_error_free(error);
            }
            return;
        }
    }
}

static void on_activate(AdwApplication *app, gpointer user_data) {
    (void)user_data;

    load_css(app);

    WhisperEchoWindow *win = whisper_echo_window_new(app);
    gtk_window_present(GTK_WINDOW(win));
}

static void on_shutdown(AdwApplication *app, gpointer user_data) {
    (void)user_data;
    /* Any cleanup needed on app shutdown */
}

int main(int argc, char **argv) {
    AdwApplication *app;
    int status;

    adw_init();
    app = adw_application_new("com.github.whisper-echo.gui", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}
