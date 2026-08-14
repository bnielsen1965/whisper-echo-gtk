#ifndef WINDOW_H
#define WINDOW_H

#include <adwaita.h>
#include <gtk/gtk.h>
#include "config.h"
#include "process.h"

G_DECLARE_FINAL_TYPE(WhisperEchoWindow, whisper_echo_window, WHISPER_ECHO, WINDOW,
                      GtkApplicationWindow)

struct _WhisperEchoWindow {
    GtkApplicationWindow parent_instance;

    /* Config */
    WhisperEchoConfig config;

    /* Process */
    WhisperProcess *process;

    /* UInput paused state tracking */
    bool uinput_paused;

    /* Widgets - Header */
    GtkHeaderBar *header_bar;

    /* Widgets - Settings */
    GtkScrolledWindow *settings_scroll;

    /* File dialogs */
    GtkFileDialog *model_file_dialog;
    GtkFileDialog *vad_file_dialog;
    GtkFileDialog *commands_file_dialog;

    /* Models section */
    GtkExpander *models_expander;
    GtkEntry *models_path_entry;
    GtkButton *models_path_browse_btn;

    /* Model section */
    GtkExpander *model_expander;
    GtkEntry *model_path_entry;
    GtkButton *model_path_browse_btn;
    GtkComboBoxText *language_combo;
    GtkCheckButton *translate_check;

    /* Performance section */
    GtkExpander *perf_expander;
    GtkSpinButton *threads_spin;
    GtkCheckButton *no_gpu_check;
    GtkCheckButton *no_flash_attn_check;
    GtkSpinButton *beam_size_spin;
    GtkSpinButton *audio_ctx_spin;

    /* VAD section */
    GtkExpander *vad_expander;
    GtkEntry *vad_model_entry;
    GtkButton *vad_model_browse_btn;
    GtkCheckButton *no_silero_check;
    GtkSpinButton *vad_threshold_spin;
    GtkSpinButton *freq_threshold_spin;
    GtkSpinButton *vad_gain_spin;

    /* Output section */
    GtkExpander *output_expander;
    GtkEntry *output_file_entry;
    GtkCheckButton *uinput_check;
    GtkEntry *commands_entry;
    GtkButton *commands_browse_btn;
    GtkCheckButton *detail_check;
    GtkCheckButton *no_status_check;
    GtkCheckButton *print_special_check;
    GtkCheckButton *tinydiarize_check;
    GtkCheckButton *save_audio_check;
    GtkCheckButton *no_fallback_check;

    /* Audio section */
    GtkExpander *audio_expander;
    GtkComboBoxText *capture_device_combo;

    /* General section */
    GtkExpander *general_expander;
    GtkEntry *binary_path_entry;
    GtkSpinButton *max_lines_spin;

    /* Widgets - Runtime */
    GtkLabel *status_label;
    GtkWidget *uinput_indicator;
    GtkButton *start_btn;
    GtkButton *stop_btn;
    GtkButton *settings_btn;
    GtkButton *help_btn;
    GtkTextView *transcription_view;

    /* Settings dialog */
    GtkWidget *settings_dialog;
};

WhisperEchoWindow *whisper_echo_window_new(AdwApplication *app);

#endif /* WINDOW_H */
