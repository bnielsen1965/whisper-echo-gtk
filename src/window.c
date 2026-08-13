#include "window.h"

#include <adwaita.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */
static void sync_config_from_widgets(WhisperEchoWindow *win);
static void on_models_path_browse(GtkButton *btn, void *user_data);
static void on_model_path_browse(GtkButton *btn, void *user_data);
static void on_vad_model_browse(GtkButton *btn, void *user_data);
static void on_commands_browse(GtkButton *btn, void *user_data);

/* Whisper language codes from whisper.cpp */
static const char *whisper_lang_codes[] = {
    "en",
    "zh",
    "de",
    "es",
    "ru",
    "ko",
    "fr",
    "ja",
    "pt",
    "tr",
    "pl",
    "ca",
    "nl",
    "ar",
    "sv",
    "it",
    "id",
    "hi",
    "fi",
    "vi",
    "he",
    "uk",
    "el",
    "ms",
    "cs",
    "ro",
    "da",
    "hu",
    "ta",
    "no",
    "th",
    "ur",
    "hr",
    "bg",
    "lt",
    "la",
    "mi",
    "ml",
    "cy",
    "sk",
    "te",
    "fa",
    "lv",
    "bn",
    "sr",
    "az",
    "sl",
    "kn",
    "et",
    "mk",
    "br",
    "eu",
    "is",
    "hy",
    "ne",
    "mn",
    "bs",
    "kk",
    "sq",
    "sw",
    "gl",
    "mr",
    "pa",
    "si",
    "km",
    "sn",
    "yo",
    "so",
    "af",
    "oc",
    "ka",
    "be",
    "tg",
    "sd",
    "gu",
    "am",
    "yi",
    "lo",
    "uz",
    "fo",
    "ht",
    "ps",
    "tk",
    "nn",
    "mt",
    "sa",
    "lb",
    "my",
    "bo",
    "tl",
    "mg",
    "as",
    "tt",
    "haw",
    "ln",
    "ha",
    "ba",
    "jw",
    "su",
    "yue",
};
static const char *whisper_lang_names[] = {
    "english",
    "chinese",
    "german",
    "spanish",
    "russian",
    "korean",
    "french",
    "japanese",
    "portuguese",
    "turkish",
    "polish",
    "catalan",
    "dutch",
    "arabic",
    "swedish",
    "italian",
    "indonesian",
    "hindi",
    "finnish",
    "vietnamese",
    "hebrew",
    "ukrainian",
    "greek",
    "malay",
    "czech",
    "romanian",
    "danish",
    "hungarian",
    "tamil",
    "norwegian",
    "thai",
    "urdu",
    "croatian",
    "bulgarian",
    "lithuanian",
    "latin",
    "maori",
    "malayalam",
    "welsh",
    "slovak",
    "telugu",
    "persian",
    "latvian",
    "bengali",
    "serbian",
    "azerbaijani",
    "slovenian",
    "kannada",
    "estonian",
    "macedonian",
    "breton",
    "basque",
    "icelandic",
    "armenian",
    "nepali",
    "mongolian",
    "bosnian",
    "kazakh",
    "albanian",
    "swahili",
    "galician",
    "marathi",
    "punjabi",
    "sinhala",
    "khmer",
    "shona",
    "yoruba",
    "somali",
    "afrikaans",
    "occitan",
    "georgian",
    "belarusian",
    "tajik",
    "sindhi",
    "gujarati",
    "amharic",
    "yiddish",
    "lao",
    "uzbek",
    "faroese",
    "haitian creole",
    "pashto",
    "turkmen",
    "nynorsk",
    "maltese",
    "sanskrit",
    "luxembourgish",
    "myanmar",
    "tibetan",
    "tagalog",
    "malagasy",
    "assamese",
    "tatar",
    "hawaiian",
    "lingala",
    "hausa",
    "bashkir",
    "javanese",
    "sundanese",
    "cantonese",
};
static const int whisper_lang_count = 100;

/* ============================================================================
 * Helper macros and utilities
 * ============================================================================ */

static GtkWidget *make_labeled_entry(const char *label, const char *initial, GtkEntry **out_entry) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_hexpand(lbl, TRUE);

    GtkWidget *entry = gtk_entry_new();
    if (initial) {
        GtkEntryBuffer *buf = gtk_entry_buffer_new(initial, -1);
        gtk_entry_set_buffer(GTK_ENTRY(entry), buf);
        g_object_unref(buf);
    }
    if (out_entry) *out_entry = GTK_ENTRY(entry);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), entry);
    return box;
}

static GtkWidget *make_labeled_entry_with_browse(const char *label, const char *initial,
                                                   GtkEntry **out_entry, GtkButton **out_btn,
                                                   GCallback browse_callback, gpointer user_data) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_hexpand(lbl, TRUE);

    GtkWidget *entry = gtk_entry_new();
    if (initial) {
        GtkEntryBuffer *buf = gtk_entry_buffer_new(initial, -1);
        gtk_entry_set_buffer(GTK_ENTRY(entry), buf);
        g_object_unref(buf);
    }
    if (out_entry) *out_entry = GTK_ENTRY(entry);

    GtkWidget *btn = gtk_button_new_with_label("…");
    gtk_widget_set_tooltip_text(btn, "Browse");
    if (out_btn) *out_btn = GTK_BUTTON(btn);
    g_signal_connect(btn, "clicked", browse_callback, user_data);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), entry);
    gtk_box_append(GTK_BOX(box), btn);
    return box;
}

static GtkWidget *make_labeled_language_combo(const char *label, const char *initial_code, GtkComboBoxText **out_combo) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_hexpand(lbl, TRUE);

    GtkWidget *combo = gtk_combo_box_text_new();
    for (int i = 0; i < whisper_lang_count; i++) {
        char display[256];
        g_snprintf(display, sizeof(display), "%s (%s)", whisper_lang_names[i], whisper_lang_codes[i]);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), whisper_lang_codes[i], display);
    }

    int active = 0;
    for (int i = 0; i < whisper_lang_count; i++) {
        if (g_strcmp0(whisper_lang_codes[i], initial_code ? initial_code : "en") == 0) {
            active = i;
            break;
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);

    if (out_combo) *out_combo = GTK_COMBO_BOX_TEXT(combo);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), combo);
    return box;
}

static GtkWidget *make_labeled_spin(const char *label, double value, double min, double max, double step,
                                     GtkSpinButton **out_spin) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_hexpand(lbl, TRUE);

    GtkAdjustment *adj = gtk_adjustment_new(value, min, max, step, 0, 0);
    GtkWidget *spin = gtk_spin_button_new(adj, 1.0, (step >= 1.0) ? 0 : 1);
    if (out_spin) *out_spin = GTK_SPIN_BUTTON(spin);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), spin);
    return box;
}

static GtkWidget *make_check_row(const char *label, gboolean active, GtkCheckButton **out_check) {
    GtkWidget *check = gtk_check_button_new_with_label(label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), active);
    if (out_check) *out_check = GTK_CHECK_BUTTON(check);
    return check;
}

static GtkWidget *make_expander(const char *title, GtkWidget *content) {
    GtkWidget *expander = gtk_expander_new(title);
    gtk_expander_set_child(GTK_EXPANDER(expander), content);
    gtk_widget_set_hexpand(expander, TRUE);
    return expander;
}

static const char *entry_get_text(GtkEntry *entry) {
    GtkEntryBuffer *buf = gtk_entry_get_buffer(entry);
    return gtk_entry_buffer_get_text(buf);
}

static void entry_set_text(GtkEntry *entry, const char *text) {
    if (text) {
        GtkEntryBuffer *buf = gtk_entry_buffer_new(text, -1);
        gtk_entry_set_buffer(entry, buf);
        g_object_unref(buf);
    }
}

static void load_audio_devices(WhisperEchoWindow *win) {
    const char *binary = win->config.binary_path[0] ? win->config.binary_path : "whisper-echo";
    gchar *binary_copy = g_strdup(binary);
    gchar *argv[3];
    argv[0] = binary_copy;
    argv[1] = "--list-devices";
    argv[2] = NULL;

    gchar *stdout_buf = NULL;
    gchar *stderr_buf = NULL;
    gint exit_status = 0;
    gboolean ok = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout_buf, &stderr_buf, &exit_status, NULL);
    g_free(binary_copy);

    GtkComboBoxText *combo = win->capture_device_combo;
    if (!combo) {
        g_free(stdout_buf);
        g_free(stderr_buf);
        return;
    }

    gtk_combo_box_text_remove_all(combo);

    if (!ok || exit_status != 0) {
        gtk_combo_box_text_append(combo, "-1", "default (unavailable)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
        g_free(stdout_buf);
        g_free(stderr_buf);
        return;
    }

    int active_index = -1;
    int idx = 0;

    /* Simple manual JSON parsing for [{"id": -1, "name": "default"}, ...] */
    const char *p = stdout_buf ? stdout_buf : "";
    while (p && *p) {
        const char *id_pos = strstr(p, "\"id\"");
        if (!id_pos) break;
        int id = 0;
        if (sscanf(id_pos, "\"id\" : %d", &id) != 1 &&
            sscanf(id_pos, "\"id\":%d", &id) != 1 &&
            sscanf(id_pos, "\"id\" : %d", &id) != 1) {
            p = id_pos + 4;
            continue;
        }
        const char *name_pos = strstr(id_pos, "\"name\"");
        if (!name_pos) break;
        char name_buf[512] = {0};
        if (sscanf(name_pos, "\"name\" : \"%511[^\"]\"", name_buf) != 1 &&
            sscanf(name_pos, "\"name\":\"%511[^\"]\"", name_buf) != 1) {
            name_buf[0] = '\0';
        }
        gchar *id_str = g_strdup_printf("%d", id);
        gchar *display = NULL;
        if (id == -1) {
            display = g_strdup("default");
        } else {
            display = g_strdup_printf("%s (%d)", name_buf[0] ? name_buf : "unnamed", id);
        }
        gtk_combo_box_text_append(combo, id_str, display);
        if (id == win->config.capture_device) {
            active_index = idx;
        }
        g_free(id_str);
        g_free(display);
        idx++;
        p = name_pos + 1;
    }

    g_free(stdout_buf);
    g_free(stderr_buf);

    if (active_index < 0 && idx > 0) {
        active_index = 0;
    }

    if (active_index >= 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active_index);
    } else if (idx > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    }
}

static void on_settings_dialog_response(GtkDialog *dialog, gint response_id, WhisperEchoWindow *win);
static void on_help_clicked(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Help");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 720, 560);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(content), scroll);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(vbox, 16);
    gtk_widget_set_margin_end(vbox, 16);
    gtk_widget_set_margin_top(vbox, 16);
    gtk_widget_set_margin_bottom(vbox, 16);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), vbox);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b><big>Whisper Echo</big></b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_append(GTK_BOX(vbox), title);

    GtkWidget *desc = gtk_label_new(NULL);
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.0);
    gtk_label_set_markup(GTK_LABEL(desc), "Real-time speech-to-text that listens via microphone, transcribes with OpenAI Whisper, and echoes text via UInput or other outputs.");
    gtk_box_append(GTK_BOX(vbox), desc);

    GtkWidget *ui_head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ui_head), "<b>User Interface</b>");
    gtk_label_set_xalign(GTK_LABEL(ui_head), 0.0);
    gtk_box_append(GTK_BOX(vbox), ui_head);

    /* Mockup of top row */
    GtkWidget *mock_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(mock_row, TRUE);

    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *kbd_img = gtk_image_new_from_icon_name("input-keyboard-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(kbd_img), 24);
    gtk_box_append(GTK_BOX(left), kbd_img);
    GtkWidget *status_lbl = gtk_label_new("STATUS");
    gtk_widget_add_css_class(status_lbl, "title-1");
    gtk_box_append(GTK_BOX(left), status_lbl);
    gtk_widget_set_hexpand(left, TRUE);
    gtk_box_append(GTK_BOX(mock_row), left);

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *start_mock = gtk_button_new_with_label("Start");
    gtk_widget_add_css_class(start_mock, "suggested-action");
    gtk_widget_set_sensitive(start_mock, FALSE);
    GtkWidget *stop_mock = gtk_button_new_with_label("Stop");
    gtk_widget_add_css_class(stop_mock, "destructive-action");
    gtk_widget_set_sensitive(stop_mock, FALSE);
    GtkWidget *settings_mock = gtk_button_new_from_icon_name("preferences-system-symbolic");
    gtk_widget_set_sensitive(settings_mock, FALSE);
    GtkWidget *help_mock = gtk_button_new_from_icon_name("help-about");
    gtk_widget_set_sensitive(help_mock, FALSE);
    gtk_box_append(GTK_BOX(right), start_mock);
    gtk_box_append(GTK_BOX(right), stop_mock);
    gtk_box_append(GTK_BOX(right), settings_mock);
    gtk_box_append(GTK_BOX(right), help_mock);
    gtk_widget_set_halign(right, GTK_ALIGN_END);
    gtk_widget_set_hexpand(right, TRUE);
    gtk_box_append(GTK_BOX(mock_row), right);

    gtk_box_append(GTK_BOX(vbox), mock_row);

    /* Left side description with UInput colors as sub-bullets */
    GtkWidget *left_head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(left_head), "<b>Left side:</b>");
    gtk_label_set_xalign(GTK_LABEL(left_head), 0.0);
    gtk_box_append(GTK_BOX(vbox), left_head);

    GtkWidget *uinput_row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(uinput_row, 12);
    GtkWidget *uinput_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(uinput_label), 0.0);
    gtk_label_set_markup(GTK_LABEL(uinput_label), "• <b>UInput indicator</b> – keyboard icon shows UInput state.");
    gtk_box_append(GTK_BOX(uinput_row), uinput_label);

    GtkWidget *uinput_sub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(uinput_sub, 24);
    const char *color_names[] = {"on", "pending", "stopped", "off"};
    const char *color_classes[] = {"uinput-on", "uinput-pending", "uinput-stopped", "uinput-off"};
    for (int i = 0; i < 4; i++) {
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *img = gtk_image_new_from_icon_name("input-keyboard-symbolic");
        gtk_image_set_pixel_size(GTK_IMAGE(img), 16);
        gtk_widget_add_css_class(img, color_classes[i]);
        GtkWidget *lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl), g_strdup_printf("• <b>%s</b> – %s", color_names[i], 
            i==0?"on":i==1?"pending":i==2?"stopped":"off"));
        gtk_box_append(GTK_BOX(hbox), img);
        gtk_box_append(GTK_BOX(hbox), lbl);
        gtk_box_append(GTK_BOX(uinput_sub), hbox);
    }
    gtk_box_append(GTK_BOX(uinput_row), uinput_sub);

    GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(status_row, 12);
    GtkWidget *status_label_item = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(status_label_item), 0.0);
    gtk_label_set_markup(GTK_LABEL(status_label_item), "• <b>Status label</b> – shows current state.");
    gtk_box_append(GTK_BOX(status_row), status_label_item);

    const char *status_names[] = {"STOPPED", "LISTENING", "CAPTURING", "PROCESSING"};
    const char *status_classes[] = {"status-idle", "status-listening", "status-capturing", "status-processing"};
    GtkWidget *status_sub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(status_sub, 24);
    for (int i = 0; i < 4; i++) {
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl), g_strdup_printf("• <b>%s</b>", status_names[i]));
        gtk_widget_add_css_class(lbl, status_classes[i]);
        gtk_box_append(GTK_BOX(hbox), lbl);
        gtk_box_append(GTK_BOX(status_sub), hbox);
    }
    gtk_box_append(GTK_BOX(status_row), status_sub);
    gtk_box_append(GTK_BOX(uinput_row), status_row);
    gtk_box_append(GTK_BOX(vbox), uinput_row);

    GtkWidget *right_head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(right_head), "<b>Right side:</b>");
    gtk_label_set_xalign(GTK_LABEL(right_head), 0.0);
    gtk_box_append(GTK_BOX(vbox), right_head);

    GtkWidget *right_desc = gtk_label_new(NULL);
    gtk_label_set_wrap(GTK_LABEL(right_desc), TRUE);
    gtk_label_set_xalign(GTK_LABEL(right_desc), 0.0);
    gtk_label_set_markup(GTK_LABEL(right_desc), 
        "• <b>Start</b> – suggested-action button to begin listening.\n"
        "• <b>Stop</b> – destructive-action button to stop listening.\n"
        "• <b>Settings</b> – preferences icon button opens the Settings dialog.\n"
        "• <b>Help</b> – help icon button opens this help dialog.");
    gtk_box_append(GTK_BOX(vbox), right_desc);

    GtkWidget *view_note = gtk_label_new(NULL);
    gtk_label_set_wrap(GTK_LABEL(view_note), TRUE);
    gtk_label_set_xalign(GTK_LABEL(view_note), 0.0);
    gtk_label_set_markup(GTK_LABEL(view_note), "<i>Below the control row is a monospace, read-only scrolled transcription view.</i>");
    gtk_box_append(GTK_BOX(vbox), view_note);

    GtkWidget *set_head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(set_head), "<b>Settings</b>");
    gtk_label_set_xalign(GTK_LABEL(set_head), 0.0);
    gtk_box_append(GTK_BOX(vbox), set_head);

    const char *settings_desc =
        "• <b>Models</b> – Directory containing Whisper models.\n"
        "• <b>Model</b> – Selected Whisper model and language, translate option.\n"
        "• <b>Performance</b> – Threads, GPU usage, attention, beam size, audio context.\n"
        "• <b>VAD</b> – Voice Activity Detection model and thresholds.\n"
        "• <b>Output</b> – Output file, UInput keyboard emulation, command execution, logging options.\n"
        "• <b>Audio</b> – Capture device selection.\n"
        "• <b>General</b> – Whisper binary path, max lines to keep in view.";
    GtkWidget *set_label = gtk_label_new(NULL);
    gtk_label_set_wrap(GTK_LABEL(set_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(set_label), 0.0);
    gtk_label_set_markup(GTK_LABEL(set_label), settings_desc);
    gtk_box_append(GTK_BOX(vbox), set_label);

    gtk_dialog_add_button(GTK_DIALOG(dialog), "Close", GTK_RESPONSE_CLOSE);
    g_signal_connect(dialog, "response", G_CALLBACK(on_settings_dialog_response), win);
    gtk_window_present(GTK_WINDOW(dialog));
}

/* ============================================================================
 * Config sync from widgets
 * ============================================================================ */

static void on_folder_selected(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    GError *error = NULL;

    GFile *file = gtk_file_dialog_select_folder_finish(dialog, result, &error);
    if (file && !error) {
        gchar *path = g_file_get_path(file);
        if (path) {
            entry_set_text(win->models_path_entry, path);
            g_free(path);
        }
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
}

static void on_file_selected(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    GError *error = NULL;

    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    if (file && !error) {
        gchar *path = g_file_get_path(file);
        if (path) {
            /* Store as relative to models_path if it's inside models_path */
            gchar *models_path = g_strdup(entry_get_text(win->models_path_entry));
            gchar *relative = NULL;
            if (models_path[0] != '\0') {
                /* Ensure models_path has trailing slash for prefix check */
                gchar *models_path_slash = g_strconcat(models_path, "/", NULL);
                if (g_str_has_prefix(path, models_path_slash)) {
                    relative = g_strdup(path + strlen(models_path_slash));
                }
                g_free(models_path_slash);
            }
            /* Determine which entry to update based on which dialog was used */
            if (dialog == win->model_file_dialog) {
                entry_set_text(win->model_path_entry, relative ? relative : path);
            } else if (dialog == win->vad_file_dialog) {
                entry_set_text(win->vad_model_entry, relative ? relative : path);
            }
            g_free(path);
            g_free(models_path);
            g_free(relative);
        }
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
}

static void on_models_path_browse(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Models Directory");

    const char *current = entry_get_text(win->models_path_entry);
    if (current[0] != '\0') {
        GFile *file = g_file_new_for_path(current);
        gtk_file_dialog_set_initial_folder(dialog, file);
        g_object_unref(file);
    }

    gtk_file_dialog_select_folder(dialog, GTK_WINDOW(win), NULL,
                                  on_folder_selected, win);
}

static void on_model_path_browse(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    if (!win->model_file_dialog) {
        win->model_file_dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(win->model_file_dialog, "Select Whisper Model");
    }

    const char *models_dir = entry_get_text(win->models_path_entry);
    if (models_dir[0] != '\0') {
        GFile *file = g_file_new_for_path(models_dir);
        gtk_file_dialog_set_initial_folder(win->model_file_dialog, file);
        g_object_unref(file);
    }

    gtk_file_dialog_open(win->model_file_dialog, GTK_WINDOW(win), NULL,
                         on_file_selected, win);
}

static void on_vad_model_browse(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    if (!win->vad_file_dialog) {
        win->vad_file_dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(win->vad_file_dialog, "Select VAD Model");
    }

    const char *models_dir = entry_get_text(win->models_path_entry);
    if (models_dir[0] != '\0') {
        GFile *file = g_file_new_for_path(models_dir);
        gtk_file_dialog_set_initial_folder(win->vad_file_dialog, file);
        g_object_unref(file);
    }

    gtk_file_dialog_open(win->vad_file_dialog, GTK_WINDOW(win), NULL,
                         on_file_selected, win);
}

static void on_commands_file_selected(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    GError *error = NULL;

    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    if (file && !error) {
        gchar *path = g_file_get_path(file);
        if (path) {
            entry_set_text(win->commands_entry, path);
            g_free(path);
        }
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
}

static void on_commands_browse(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    if (!win->commands_file_dialog) {
        win->commands_file_dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(win->commands_file_dialog, "Select Commands File");
    }

    const char *current = entry_get_text(win->commands_entry);
    if (current[0] != '\0') {
        gchar *dir = g_path_get_dirname(current);
        GFile *file = g_file_new_for_path(dir);
        gtk_file_dialog_set_initial_folder(win->commands_file_dialog, file);
        g_object_unref(file);
        g_free(dir);
    }

    gtk_file_dialog_open(win->commands_file_dialog, GTK_WINDOW(win), NULL,
                         on_commands_file_selected, win);
}

/* ============================================================================
 * Signal callbacks
 * ============================================================================ */

static void update_uinput_indicator(WhisperEchoWindow *win) {
    GtkWidget *indicator = win->uinput_indicator;
    gtk_widget_remove_css_class(indicator, "uinput-on");
    gtk_widget_remove_css_class(indicator, "uinput-off");
    gtk_widget_remove_css_class(indicator, "uinput-pending");
    gtk_widget_remove_css_class(indicator, "uinput-stopped");
    if (!win->config.uinput) {
        gtk_widget_add_css_class(indicator, "uinput-off");
    } else if (win->uinput_paused) {
        gtk_widget_add_css_class(indicator, "uinput-stopped");
    } else {
        bool running = win->process && whisper_process_is_running(win->process);
        if (running) {
            gtk_widget_add_css_class(indicator, "uinput-on");
        } else {
            gtk_widget_add_css_class(indicator, "uinput-pending");
        }
    }
}

static void on_status_changed(WhisperStatus status, const PauseFlags *flags, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    GtkWidget *label_widget = GTK_WIDGET(win->status_label);

    const char *label;
    const char *css_class;

    switch (status) {
        case STATUS_IDLE:
            label = "IDLE";
            css_class = "status-idle";
            break;
        case STATUS_LISTENING: {
            label = "LISTENING";
            css_class = "status-listening";
            /* Append pause indicators */
            gchar *full = g_strdup(label);
            if (flags->print_paused || flags->uinput_paused) {
                gchar *tmp = full;
                if (flags->print_paused && flags->uinput_paused) {
                    full = g_strconcat(tmp, " (p Si)", NULL);
                } else if (flags->print_paused) {
                    full = g_strconcat(tmp, " (p)", NULL);
                } else {
                    full = g_strconcat(tmp, " (Si)", NULL);
                }
                g_free(tmp);
            }
            gtk_label_set_text(win->status_label, full);
            gtk_widget_remove_css_class(label_widget, "status-idle");
            gtk_widget_remove_css_class(label_widget, "status-capturing");
            gtk_widget_remove_css_class(label_widget, "status-processing");
            gtk_widget_add_css_class(label_widget, css_class);
            g_free(full);
            /* Update paused state from listening status */
            win->uinput_paused = flags->uinput_paused;
            /* Update UInput indicator */
            update_uinput_indicator(win);
            return;
        }
        case STATUS_CAPTURING:
            label = "CAPTURING";
            css_class = "status-capturing";
            break;
        case STATUS_PROCESSING:
            label = "PROCESSING";
            css_class = "status-processing";
            break;
        default:
            label = "UNKNOWN";
            css_class = "status-idle";
            break;
    }

    gtk_label_set_text(win->status_label, label);
    gtk_widget_remove_css_class(label_widget, "status-idle");
    gtk_widget_remove_css_class(label_widget, "status-listening");
    gtk_widget_remove_css_class(label_widget, "status-capturing");
    gtk_widget_remove_css_class(label_widget, "status-processing");
    gtk_widget_add_css_class(label_widget, css_class);

    /* Update UInput indicator */
    update_uinput_indicator(win);
}

static void trim_transcription_buffer(WhisperEchoWindow *win) {
    int max_lines = win->config.max_transcription_lines;
    if (max_lines <= 0) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(win->transcription_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    /* Count lines */
    int line_count = 0;
    GtkTextIter iter = start;
    while (!gtk_text_iter_equal(&iter, &end)) {
        if (gtk_text_iter_starts_line(&iter)) {
            line_count++;
        }
        if (!gtk_text_iter_forward_char(&iter)) break;
    }

    if (line_count <= max_lines) return;

    /* Remove oldest lines */
    GtkTextIter trim_start = start;
    GtkTextIter trim_end = start;
    int lines_to_remove = line_count - max_lines;
    for (int i = 0; i < lines_to_remove; i++) {
        if (!gtk_text_iter_forward_to_line_end(&trim_end)) break;
        if (!gtk_text_iter_forward_char(&trim_end)) break;
    }
    gtk_text_buffer_delete(buffer, &trim_start, &trim_end);
}

static void on_transcription_line(const char *line, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(win->transcription_view);
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, line, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", 1);

    trim_transcription_buffer(win);

    /* Auto-scroll to bottom */
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(win->transcription_view),
                                  gtk_text_buffer_get_insert(buffer), 0.0, FALSE, 0.0, 1.0);
}

static void on_process_exited(int exit_code, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    fprintf(stderr, "[whisper-echo-gtk] Process exited with code %d\n", exit_code);

    win->process = NULL;

    gtk_label_set_text(win->status_label, "STOPPED");
    gtk_widget_remove_css_class(GTK_WIDGET(win->status_label), "status-listening");
    gtk_widget_remove_css_class(GTK_WIDGET(win->status_label), "status-capturing");
    gtk_widget_remove_css_class(GTK_WIDGET(win->status_label), "status-processing");
    gtk_widget_add_css_class(GTK_WIDGET(win->status_label), "status-idle");
    update_uinput_indicator(win);

    /* Re-enable start button, disable others */
    gtk_widget_set_sensitive(GTK_WIDGET(win->start_btn), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(win->stop_btn), FALSE);
}

static void on_process_error(const char *error, void *user_data) {
    (void)user_data;
    fprintf(stderr, "[whisper-echo] %s\n", error);
}

static void on_start_clicked(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    /* Sync config from widgets */
    sync_config_from_widgets(win);
    config_save(&win->config);
    /* Update UInput indicator */
    update_uinput_indicator(win);

    /* Resolve binary path to absolute for reliable spawning */
    gchar *binary_abs = NULL;
    if (g_path_is_absolute(win->config.binary_path)) {
        binary_abs = g_strdup(win->config.binary_path);
    } else if (strchr(win->config.binary_path, '/')) {
        /* Relative path with directory component */
        gchar *cwd_gui = g_get_current_dir();
        binary_abs = g_build_filename(cwd_gui, win->config.binary_path, NULL);
        g_free(cwd_gui);
        /* If not executable, fall back to original */
        if (!g_file_test(binary_abs, G_FILE_TEST_IS_EXECUTABLE)) {
            g_free(binary_abs);
            binary_abs = g_strdup(win->config.binary_path);
        }
    } else {
        /* Bare name, search PATH */
        binary_abs = g_find_program_in_path(win->config.binary_path);
        if (!binary_abs) {
            binary_abs = g_strdup(win->config.binary_path);
        }
    }

    /* Temporarily update config with absolute binary path */
    char saved_binary[512];
    g_strlcpy(saved_binary, win->config.binary_path, sizeof(saved_binary));
    g_strlcpy(win->config.binary_path, binary_abs, sizeof(win->config.binary_path));
    g_free(binary_abs);

    /* Use parent working directory for simplicity */
    const char *cwd = NULL;

    win->process = whisper_process_new();

    /* Log the command that will be executed */
    {
        char **args = NULL;
        int argc = 0;
        config_to_args(&win->config, &args, &argc);
        fprintf(stderr, "[whisper-echo-gtk] Starting:");
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, " %s", args[i]);
        }
        fprintf(stderr, "\n");
        for (int i = 0; i <= argc; i++) {
            g_free(args[i]);
        }
        g_free(args);
    }

    bool ok = whisper_process_start(win->process, &win->config, cwd,
                                    on_status_changed,
                                    on_transcription_line,
                                    on_process_exited,
                                    on_process_error,
                                    win);

    /* Restore original binary path */
    g_strlcpy(win->config.binary_path, saved_binary, sizeof(win->config.binary_path));

    if (!ok) {
        whisper_process_free(win->process);
        win->process = NULL;
        gtk_label_set_text(win->status_label, "START FAILED");
        return;
    }

    /* Update button states */
    gtk_widget_set_sensitive(GTK_WIDGET(win->start_btn), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(win->stop_btn), TRUE);

    /* Clear transcription view */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(win->transcription_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_delete(buffer, &start, &end);
}

static void on_stop_clicked(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    if (win->process && whisper_process_is_running(win->process)) {
        whisper_process_stop(win->process);
    }
}

static void on_settings_dialog_response(GtkDialog *dialog, gint response_id, WhisperEchoWindow *win) {
    (void)response_id;
    sync_config_from_widgets(win);
    config_save(&win->config);
    gtk_widget_hide(GTK_WIDGET(dialog));
}

static void on_settings_clicked(GtkButton *btn, void *user_data) {
    WhisperEchoWindow *win = (WhisperEchoWindow *)user_data;
    (void)btn;

    if (!win->settings_dialog) {
        GtkWidget *dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Settings");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);
        gtk_window_set_hide_on_close(GTK_WINDOW(dialog), TRUE);

        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_box_append(GTK_BOX(content), GTK_WIDGET(win->settings_scroll));

        gtk_widget_set_margin_start(GTK_WIDGET(win->settings_scroll), 16);
        gtk_widget_set_margin_end(GTK_WIDGET(win->settings_scroll), 16);
        gtk_widget_set_margin_top(GTK_WIDGET(win->settings_scroll), 16);
        gtk_widget_set_margin_bottom(GTK_WIDGET(win->settings_scroll), 16);

        gtk_dialog_add_button(GTK_DIALOG(dialog), "Close", GTK_RESPONSE_CLOSE);
        g_signal_connect(dialog, "response", G_CALLBACK(on_settings_dialog_response), win);
        win->settings_dialog = dialog;
    }
    gtk_window_present(GTK_WINDOW(win->settings_dialog));
}



/* ============================================================================
 * Config sync from widgets
 * ============================================================================ */

static void sync_config_from_widgets(WhisperEchoWindow *win) {
    g_strlcpy(win->config.models_path, entry_get_text(win->models_path_entry),
              sizeof(win->config.models_path));
    g_strlcpy(win->config.model_path, entry_get_text(win->model_path_entry),
              sizeof(win->config.model_path));
    {
        const char *lang_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(win->language_combo));
        if (lang_id) {
            g_strlcpy(win->config.language, lang_id, sizeof(win->config.language));
        }
    }
    win->config.translate = gtk_check_button_get_active(win->translate_check);

    win->config.threads = (int)gtk_spin_button_get_value(win->threads_spin);
    win->config.no_gpu = gtk_check_button_get_active(win->no_gpu_check);
    win->config.no_flash_attn = gtk_check_button_get_active(win->no_flash_attn_check);
    win->config.beam_size = (int)gtk_spin_button_get_value(win->beam_size_spin);
    win->config.audio_ctx = (int)gtk_spin_button_get_value(win->audio_ctx_spin);

    g_strlcpy(win->config.vad_model, entry_get_text(win->vad_model_entry),
              sizeof(win->config.vad_model));
    win->config.no_silero_vad = gtk_check_button_get_active(win->no_silero_check);
    win->config.vad_threshold = gtk_spin_button_get_value(win->vad_threshold_spin);
    win->config.freq_threshold = gtk_spin_button_get_value(win->freq_threshold_spin);
    win->config.vad_gain = gtk_spin_button_get_value(win->vad_gain_spin);

    g_strlcpy(win->config.output_file, entry_get_text(win->output_file_entry),
              sizeof(win->config.output_file));
    win->config.uinput = gtk_check_button_get_active(win->uinput_check);
    g_strlcpy(win->config.commands_file, entry_get_text(win->commands_entry),
              sizeof(win->config.commands_file));
    win->config.no_fallback = gtk_check_button_get_active(win->no_fallback_check);

    {
        const char *dev_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(win->capture_device_combo));
        if (dev_id) {
            win->config.capture_device = atoi(dev_id);
        }
    }

    g_strlcpy(win->config.binary_path, entry_get_text(win->binary_path_entry),
              sizeof(win->config.binary_path));
    win->config.max_transcription_lines = (int)gtk_spin_button_get_value(win->max_lines_spin);
}

/* ============================================================================
 * Window construction
 * ============================================================================ */

G_DEFINE_TYPE(WhisperEchoWindow, whisper_echo_window, GTK_TYPE_APPLICATION_WINDOW)

static void whisper_echo_window_class_init(WhisperEchoWindowClass *class) {
    /* No template needed */
}

static void build_settings_pane(WhisperEchoWindow *win) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_valign(box, GTK_ALIGN_START);

    /* Warning about settings applying on start */
    GtkWidget *warn = gtk_label_new("Warning: Settings are applied when you Start the whisper-echo process. Changes made while the process is running will not take effect until you Stop and Start again.");
    gtk_label_set_wrap(GTK_LABEL(warn), TRUE);
    gtk_label_set_xalign(GTK_LABEL(warn), 0.0);
    gtk_widget_add_css_class(warn, "warning");
    gtk_widget_set_margin_start(warn, 12);
    gtk_widget_set_margin_end(warn, 12);
    gtk_box_append(GTK_BOX(box), warn);

    /* --- Models Directory --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                       make_labeled_entry_with_browse("Models directory:", win->config.models_path,
                                                      &win->models_path_entry, &win->models_path_browse_btn,
                                                      G_CALLBACK(on_models_path_browse), win));

        win->models_expander = GTK_EXPANDER(make_expander("Models Directory", content));
        gtk_expander_set_expanded(win->models_expander, TRUE);
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->models_expander));
    }

    /* --- Model & Language --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                        make_labeled_entry_with_browse("Model:", win->config.model_path,
                                                       &win->model_path_entry, &win->model_path_browse_btn,
                                                       G_CALLBACK(on_model_path_browse), win));
        gtk_box_append(GTK_BOX(content),
                        make_labeled_language_combo("Language:", win->config.language, &win->language_combo));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("Translate to English", win->config.translate, &win->translate_check));

        win->model_expander = GTK_EXPANDER(make_expander("Model & Language", content));
        gtk_expander_set_expanded(win->model_expander, TRUE);
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->model_expander));
    }

    /* --- Performance --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                        make_labeled_spin("Threads:", (double)win->config.threads, 1, 32, 1, &win->threads_spin));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("CPU only (no GPU)", win->config.no_gpu, &win->no_gpu_check));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("Disable flash attention", win->config.no_flash_attn, &win->no_flash_attn_check));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("Beam size (-1=greedy):", (double)win->config.beam_size, -1, 10, 1, &win->beam_size_spin));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("Audio ctx tokens (0=all):", (double)win->config.audio_ctx, 0, 2048, 1, &win->audio_ctx_spin));

        win->perf_expander = GTK_EXPANDER(make_expander("Performance", content));
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->perf_expander));
    }

    /* --- VAD --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                       make_labeled_entry_with_browse("VAD model:", win->config.vad_model,
                                                      &win->vad_model_entry, &win->vad_model_browse_btn,
                                                      G_CALLBACK(on_vad_model_browse), win));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("Disable Silero VAD (use energy)", win->config.no_silero_vad, &win->no_silero_check));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("VAD threshold:", win->config.vad_threshold, 0.0, 1.0, 0.05, &win->vad_threshold_spin));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("Freq threshold (Hz):", win->config.freq_threshold, 0.0, 500.0, 10.0, &win->freq_threshold_spin));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("VAD gain:", win->config.vad_gain, 0.1, 5.0, 0.1, &win->vad_gain_spin));

        win->vad_expander = GTK_EXPANDER(make_expander("Voice Activity Detection", content));
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->vad_expander));
    }

    /* --- Output --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                       make_labeled_entry("Output file:", win->config.output_file, &win->output_file_entry));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("Enable uinput typing", win->config.uinput, &win->uinput_check));
        gtk_box_append(GTK_BOX(content),
                        make_labeled_entry_with_browse("Commands file:", win->config.commands_file,
                                                       &win->commands_entry, &win->commands_browse_btn,
                                                       G_CALLBACK(on_commands_browse), win));
        gtk_box_append(GTK_BOX(content),
                        make_check_row("No temperature fallback", win->config.no_fallback, &win->no_fallback_check));

        win->output_expander = GTK_EXPANDER(make_expander("Output", content));
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->output_expander));
    }

    /* --- Audio --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        /* Capture device combo - populated dynamically from whisper-echo --list-devices */
        {
            GtkWidget *box_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            GtkWidget *lbl = gtk_label_new("Capture device:");
            gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
            gtk_widget_set_hexpand(lbl, TRUE);

            GtkWidget *combo = gtk_combo_box_text_new();
            win->capture_device_combo = GTK_COMBO_BOX_TEXT(combo);
            /* placeholder entry, will be replaced when devices are loaded */
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "-1", "default (loading...)");

            gtk_box_append(GTK_BOX(box_row), lbl);
            gtk_box_append(GTK_BOX(box_row), combo);
            gtk_box_append(GTK_BOX(content), box_row);
        }

        win->audio_expander = GTK_EXPANDER(make_expander("Audio", content));
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->audio_expander));
    }

    /* --- General --- */
    {
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(content, 12);

        gtk_box_append(GTK_BOX(content),
                       make_labeled_entry("whisper-echo binary:", win->config.binary_path, &win->binary_path_entry));
        gtk_box_append(GTK_BOX(content),
                       make_labeled_spin("Max transcription lines:", (double)win->config.max_transcription_lines, 0, 10000, 1, &win->max_lines_spin));

        win->general_expander = GTK_EXPANDER(make_expander("General", content));
        gtk_box_append(GTK_BOX(box), GTK_WIDGET(win->general_expander));
    }

    /* Wrap in scrolled window */
    win->settings_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(win->settings_scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(win->settings_scroll, box);
    gtk_widget_set_hexpand(GTK_WIDGET(win->settings_scroll), FALSE);
    gtk_widget_set_vexpand(GTK_WIDGET(win->settings_scroll), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(win->settings_scroll), 280, -1);
}

static GtkWidget *build_runtime_pane(WhisperEchoWindow *win) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, TRUE);

    /* Status label */
    win->status_label = GTK_LABEL(gtk_label_new("STOPPED"));
    gtk_label_set_xalign(win->status_label, 0.0);
    gtk_widget_set_halign(GTK_WIDGET(win->status_label), GTK_ALIGN_START);
    gtk_widget_add_css_class(GTK_WIDGET(win->status_label), "title-1");
    gtk_widget_add_css_class(GTK_WIDGET(win->status_label), "status-idle");

    /* UInput indicator */
    win->uinput_indicator = gtk_image_new_from_icon_name("input-keyboard-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(win->uinput_indicator), 24);
    gtk_widget_set_halign(win->uinput_indicator, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(win->uinput_indicator, "uinput-icon");
    update_uinput_indicator(win);

    /* Control buttons */
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);

    win->start_btn = GTK_BUTTON(gtk_button_new_with_label("Start"));
    win->stop_btn = GTK_BUTTON(gtk_button_new_with_label("Stop"));
    win->settings_btn = GTK_BUTTON(gtk_button_new_from_icon_name("preferences-system-symbolic"));

    gtk_widget_add_css_class(GTK_WIDGET(win->start_btn), "suggested-action");
    gtk_widget_add_css_class(GTK_WIDGET(win->stop_btn), "destructive-action");

    /* Reduce buttons to 80% of previous size */
    gtk_widget_set_size_request(GTK_WIDGET(win->start_btn), -1, 17);
    gtk_widget_set_size_request(GTK_WIDGET(win->stop_btn), -1, 17);
    gtk_widget_set_size_request(GTK_WIDGET(win->settings_btn), 19, 17);

    gtk_widget_set_sensitive(GTK_WIDGET(win->stop_btn), FALSE);

    gtk_box_append(GTK_BOX(btn_box), GTK_WIDGET(win->start_btn));
    gtk_box_append(GTK_BOX(btn_box), GTK_WIDGET(win->stop_btn));
    gtk_box_append(GTK_BOX(btn_box), GTK_WIDGET(win->settings_btn));

    /* Help button */
    GtkWidget *help_btn = gtk_button_new_from_icon_name("help-about");
    gtk_widget_set_size_request(help_btn, 19, 17);
    gtk_box_append(GTK_BOX(btn_box), help_btn);
    g_signal_connect(help_btn, "clicked", G_CALLBACK(on_help_clicked), win);

    /* Transcription view */
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    win->transcription_view = GTK_TEXT_VIEW(tv);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    /* Status row with indicator */
    GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(status_row, GTK_ALIGN_START);
    gtk_widget_set_hexpand(GTK_WIDGET(status_row), TRUE);
    gtk_box_append(GTK_BOX(status_row), GTK_WIDGET(win->uinput_indicator));
    gtk_box_append(GTK_BOX(status_row), GTK_WIDGET(win->status_label));

    /* Top row: status left 50%, buttons right 50% */
    GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(top_row, TRUE);
    gtk_box_append(GTK_BOX(top_row), status_row);
    gtk_box_append(GTK_BOX(top_row), btn_box);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    gtk_widget_set_hexpand(GTK_WIDGET(btn_box), TRUE);

    gtk_box_append(GTK_BOX(box), top_row);
    gtk_box_append(GTK_BOX(box), scroll);

    return box;
}

static gboolean on_window_close_request(GtkWindow *window, gpointer user_data) {
    WhisperEchoWindow *win = WHISPER_ECHO_WINDOW(window);
    sync_config_from_widgets(win);
    config_save(&win->config);
    return FALSE;
}

static void whisper_echo_window_init(WhisperEchoWindow *win) {
    /* Load config */
    config_load(&win->config);
    win->uinput_paused = false;
    win->settings_dialog = NULL;

    gtk_window_set_title(GTK_WINDOW(win), "Whisper Echo");

    /* Header bar */
    win->header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
    GtkWidget *title_label = gtk_label_new("Whisper Echo");
    gtk_header_bar_set_title_widget(win->header_bar, title_label);
    gtk_header_bar_set_show_title_buttons(win->header_bar, TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(win), GTK_WIDGET(win->header_bar));

    /* Build settings widgets for dialog use */
    build_settings_pane(win);
    load_audio_devices(win);

    GtkWidget *runtime_box = build_runtime_pane(win);
    gtk_window_set_child(GTK_WINDOW(win), runtime_box);

    /* Connect button signals */
    g_signal_connect(win->start_btn, "clicked", G_CALLBACK(on_start_clicked), win);
    g_signal_connect(win->stop_btn, "clicked", G_CALLBACK(on_stop_clicked), win);
    g_signal_connect(win->settings_btn, "clicked", G_CALLBACK(on_settings_clicked), win);

    /* Save config on window close */
    g_signal_connect(win, "close-request", G_CALLBACK(on_window_close_request), NULL);

    /* Set window size - enough for ~5 lines of transcription */
    gtk_window_set_default_size(GTK_WINDOW(win), 720, 260);
}

WhisperEchoWindow *whisper_echo_window_new(AdwApplication *app) {
    return g_object_new(whisper_echo_window_get_type(),
                        "application", app,
                        NULL);
}
