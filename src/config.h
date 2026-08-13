#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    /* Models directory */
    char models_path[512];

    /* Model settings */
    char model_path[512];
    char language[16];
    bool translate;

    /* Performance settings */
    int threads;
    bool no_gpu;
    int gpu_device;
    bool no_flash_attn;
    int beam_size;
    int audio_ctx;

    /* VAD settings */
    char vad_model[512];
    bool no_silero_vad;
    double vad_threshold;
    double freq_threshold;
    double vad_gain;

    /* Output settings */
    char output_file[512];
    bool uinput;
    char commands_file[512];
    bool detail;
    bool no_status;
    bool print_special;
    bool tinydiarize;
    bool save_audio;
    bool no_fallback;

    /* Audio settings */
    int capture_device;

    /* Whisper-echo binary path */
    char binary_path[512];

    /* UI settings */
    int max_transcription_lines;
} WhisperEchoConfig;

/* Load config from ~/.whisper-echo.conf, applying defaults for missing values */
void config_load(WhisperEchoConfig *cfg);

/* Save config to ~/.whisper-echo.conf */
void config_save(const WhisperEchoConfig *cfg);

/* Build command-line argument array from config.
 * Caller must free each arg and the array itself. */
void config_to_args(const WhisperEchoConfig *cfg, char ***out_args, int *argc);

/* Get the config file path (~/.whisper-echo.conf) */
const char *config_path(void);

#endif /* CONFIG_H */
