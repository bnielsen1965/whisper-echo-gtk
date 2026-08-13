#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *config_file_path = NULL;

static void config_set_defaults(WhisperEchoConfig *cfg) {
    const char *home = g_get_home_dir();

    /* Models directory */
    g_strlcpy(cfg->models_path, home, sizeof(cfg->models_path));
    g_strlcat(cfg->models_path, "/.whisper-echo/models", sizeof(cfg->models_path));

    /* Model */
    g_strlcpy(cfg->model_path, "ggml-base.en.bin", sizeof(cfg->model_path));
    g_strlcpy(cfg->language, "en", sizeof(cfg->language));
    cfg->translate = false;

    /* Performance */
    cfg->threads = 4;
    cfg->no_gpu = false;
    cfg->gpu_device = 0;
    cfg->no_flash_attn = false;
    cfg->beam_size = -1;
    cfg->audio_ctx = 0;

    /* VAD */
    g_strlcpy(cfg->vad_model, "ggml-silero-v6.2.0.bin", sizeof(cfg->vad_model));
    cfg->no_silero_vad = false;
    cfg->vad_threshold = 0.5;
    cfg->freq_threshold = 80.0;
    cfg->vad_gain = 1.5;

    /* Output */
    cfg->output_file[0] = '\0';
    cfg->uinput = false;
    g_strlcpy(cfg->commands_file, home, sizeof(cfg->commands_file));
    g_strlcat(cfg->commands_file, "/.whisper-echo/command.json", sizeof(cfg->commands_file));
    cfg->detail = false;
    cfg->no_status = false;
    cfg->print_special = false;
    cfg->tinydiarize = false;
    cfg->save_audio = false;
    cfg->no_fallback = false;

    /* Audio */
    cfg->capture_device = -1;

    /* Binary path */
    g_strlcpy(cfg->binary_path, "whisper-echo", sizeof(cfg->binary_path));

    /* UI */
    cfg->max_transcription_lines = 500;
}

const char *config_path(void) {
    if (config_file_path == NULL) {
        const char *home = g_get_home_dir();
        config_file_path = g_strconcat(home, "/.whisper-echo/whisper-echo.conf", NULL);
    }
    return config_file_path;
}

static bool parse_bool(const char *str) {
    if (g_str_equal(str, "true") || g_str_equal(str, "yes") || g_str_equal(str, "1")) {
        return true;
    }
    return false;
}

static void config_parse_file(WhisperEchoConfig *cfg, const char *path) {
    GKeyFile *kf = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, &error)) {
        g_key_file_free(kf);
        return;
    }

    /* [models] */
    {
        const char *val = g_key_file_get_string(kf, "models", "path", NULL);
        if (val) g_strlcpy(cfg->models_path, val, sizeof(cfg->models_path));
    }

    /* [model] */
    {
        const char *val = g_key_file_get_string(kf, "model", "path", NULL);
        if (val) g_strlcpy(cfg->model_path, val, sizeof(cfg->model_path));
    }
    {
        const char *val = g_key_file_get_string(kf, "model", "language", NULL);
        if (val) g_strlcpy(cfg->language, val, sizeof(cfg->language));
    }
    {
        const char *val = g_key_file_get_string(kf, "model", "translate", NULL);
        if (val) cfg->translate = parse_bool(val);
    }

    /* [performance] */
    {
        gint v = g_key_file_get_integer(kf, "performance", "threads", &error);
        if (!error) cfg->threads = v;
        g_clear_error(&error);
    }
    {
        const char *val = g_key_file_get_string(kf, "performance", "no_gpu", NULL);
        if (val) cfg->no_gpu = parse_bool(val);
    }
    {
        gint v = g_key_file_get_integer(kf, "performance", "gpu_device", &error);
        if (!error) cfg->gpu_device = v;
        g_clear_error(&error);
    }
    {
        const char *val = g_key_file_get_string(kf, "performance", "no_flash_attn", NULL);
        if (val) cfg->no_flash_attn = parse_bool(val);
    }
    {
        gint v = g_key_file_get_integer(kf, "performance", "beam_size", &error);
        if (!error) cfg->beam_size = v;
        g_clear_error(&error);
    }
    {
        gint v = g_key_file_get_integer(kf, "performance", "audio_ctx", &error);
        if (!error) cfg->audio_ctx = v;
        g_clear_error(&error);
    }

    /* [vad] */
    {
        const char *val = g_key_file_get_string(kf, "vad", "vad_model", NULL);
        if (val) g_strlcpy(cfg->vad_model, val, sizeof(cfg->vad_model));
    }
    {
        const char *val = g_key_file_get_string(kf, "vad", "no_silero_vad", NULL);
        if (val) cfg->no_silero_vad = parse_bool(val);
    }
    {
        gdouble v = g_key_file_get_double(kf, "vad", "vad_threshold", &error);
        if (!error) cfg->vad_threshold = v;
        g_clear_error(&error);
    }
    {
        gdouble v = g_key_file_get_double(kf, "vad", "freq_threshold", &error);
        if (!error) cfg->freq_threshold = v;
        g_clear_error(&error);
    }
    {
        gdouble v = g_key_file_get_double(kf, "vad", "vad_gain", &error);
        if (!error) cfg->vad_gain = v;
        g_clear_error(&error);
    }

    /* [output] */
    {
        const char *val = g_key_file_get_string(kf, "output", "file", NULL);
        if (val) g_strlcpy(cfg->output_file, val, sizeof(cfg->output_file));
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "uinput", NULL);
        if (val) cfg->uinput = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "commands", NULL);
        if (val) g_strlcpy(cfg->commands_file, val, sizeof(cfg->commands_file));
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "detail", NULL);
        if (val) cfg->detail = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "no_status", NULL);
        if (val) cfg->no_status = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "print_special", NULL);
        if (val) cfg->print_special = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "tinydiarize", NULL);
        if (val) cfg->tinydiarize = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "save_audio", NULL);
        if (val) cfg->save_audio = parse_bool(val);
    }
    {
        const char *val = g_key_file_get_string(kf, "output", "no_fallback", NULL);
        if (val) cfg->no_fallback = parse_bool(val);
    }

    /* [audio] */
    {
        gint v = g_key_file_get_integer(kf, "audio", "capture_device", &error);
        if (!error) cfg->capture_device = v;
        g_clear_error(&error);
    }

    /* [general] */
    {
        const char *val = g_key_file_get_string(kf, "general", "binary_path", NULL);
        if (val) g_strlcpy(cfg->binary_path, val, sizeof(cfg->binary_path));
    }
    {
        gint v = g_key_file_get_integer(kf, "general", "max_transcription_lines", &error);
        if (!error) cfg->max_transcription_lines = v;
        g_clear_error(&error);
    }

    g_key_file_free(kf);
}

void config_load(WhisperEchoConfig *cfg) {
    config_set_defaults(cfg);
    config_parse_file(cfg, config_path());

    /* Ensure config directory exists */
    gchar *config_dir = g_path_get_dirname((gchar *)config_path());
    g_mkdir_with_parents(config_dir, 0755);
    g_free(config_dir);

    /* Ensure models directory exists */
    g_mkdir_with_parents(cfg->models_path, 0755);
}

void config_save(const WhisperEchoConfig *cfg) {
    GKeyFile *kf = g_key_file_new();
    GError *error = NULL;

    /* [models] */
    g_key_file_set_string(kf, "models", "path", cfg->models_path);

    /* [model] */
    g_key_file_set_string(kf, "model", "path", cfg->model_path);
    g_key_file_set_string(kf, "model", "language", cfg->language);
    g_key_file_set_string(kf, "model", "translate", cfg->translate ? "true" : "false");

    /* [performance] */
    g_key_file_set_integer(kf, "performance", "threads", cfg->threads);
    g_key_file_set_string(kf, "performance", "no_gpu", cfg->no_gpu ? "true" : "false");
    g_key_file_set_integer(kf, "performance", "gpu_device", cfg->gpu_device);
    g_key_file_set_string(kf, "performance", "no_flash_attn", cfg->no_flash_attn ? "true" : "false");
    g_key_file_set_integer(kf, "performance", "beam_size", cfg->beam_size);
    g_key_file_set_integer(kf, "performance", "audio_ctx", cfg->audio_ctx);

    /* [vad] */
    g_key_file_set_string(kf, "vad", "vad_model", cfg->vad_model);
    g_key_file_set_string(kf, "vad", "no_silero_vad", cfg->no_silero_vad ? "true" : "false");
    g_key_file_set_double(kf, "vad", "vad_threshold", cfg->vad_threshold);
    g_key_file_set_double(kf, "vad", "freq_threshold", cfg->freq_threshold);
    g_key_file_set_double(kf, "vad", "vad_gain", cfg->vad_gain);

    /* [output] */
    g_key_file_set_string(kf, "output", "file", cfg->output_file);
    g_key_file_set_string(kf, "output", "uinput", cfg->uinput ? "true" : "false");
    g_key_file_set_string(kf, "output", "commands", cfg->commands_file);
    g_key_file_set_string(kf, "output", "detail", cfg->detail ? "true" : "false");
    g_key_file_set_string(kf, "output", "no_status", cfg->no_status ? "true" : "false");
    g_key_file_set_string(kf, "output", "print_special", cfg->print_special ? "true" : "false");
    g_key_file_set_string(kf, "output", "tinydiarize", cfg->tinydiarize ? "true" : "false");
    g_key_file_set_string(kf, "output", "save_audio", cfg->save_audio ? "true" : "false");
    g_key_file_set_string(kf, "output", "no_fallback", cfg->no_fallback ? "true" : "false");

    /* [audio] */
    g_key_file_set_integer(kf, "audio", "capture_device", cfg->capture_device);

    /* [general] */
    if (cfg->binary_path[0] != '\0') {
        g_key_file_set_string(kf, "general", "binary_path", cfg->binary_path);
    }
    g_key_file_set_integer(kf, "general", "max_transcription_lines", cfg->max_transcription_lines);

    /* Write to file */
    gchar *data = g_key_file_to_data(kf, NULL, &error);
    if (data) {
        g_file_set_contents(config_path(), data, -1, &error);
        g_free(data);
    }

    if (error) {
        fprintf(stderr, "Failed to save config: %s\n", error->message);
        g_error_free(error);
    }

    g_key_file_free(kf);
}

void config_to_args(const WhisperEchoConfig *cfg, char ***out_args, int *argc) {
    GPtrArray *args = g_ptr_array_new();

    /* Binary path */
    g_ptr_array_add(args, g_strdup(cfg->binary_path));

    /* Model - resolve relative to models_path */
    g_ptr_array_add(args, g_strdup("-m"));
    {
        gchar *full_model = NULL;
        if (cfg->model_path[0] == '/') {
            full_model = g_strdup(cfg->model_path);
        } else if (cfg->models_path[0] != '\0') {
            full_model = g_strconcat(cfg->models_path, "/", cfg->model_path, NULL);
        } else {
            full_model = g_strdup(cfg->model_path);
        }
        g_ptr_array_add(args, full_model);
    }

    /* Language */
    g_ptr_array_add(args, g_strdup("-l"));
    g_ptr_array_add(args, g_strdup(cfg->language));

    /* Threads */
    if (cfg->threads > 0) {
        g_ptr_array_add(args, g_strdup("-t"));
        g_ptr_array_add(args, g_strdup_printf("%d", cfg->threads));
    }

    /* Translate */
    if (cfg->translate) {
        g_ptr_array_add(args, g_strdup("-tr"));
    }

    /* GPU */
    if (cfg->no_gpu) {
        g_ptr_array_add(args, g_strdup("-ng"));
    } else {
        if (cfg->gpu_device > 0) {
            g_ptr_array_add(args, g_strdup("-gd"));
            g_ptr_array_add(args, g_strdup_printf("%d", cfg->gpu_device));
        }
        if (cfg->no_flash_attn) {
            g_ptr_array_add(args, g_strdup("-nfa"));
        }
    }

    /* Beam size */
    if (cfg->beam_size != -1) {
        g_ptr_array_add(args, g_strdup("-bs"));
        g_ptr_array_add(args, g_strdup_printf("%d", cfg->beam_size));
    }

    /* Audio context */
    if (cfg->audio_ctx > 0) {
        g_ptr_array_add(args, g_strdup("-ac"));
        g_ptr_array_add(args, g_strdup_printf("%d", cfg->audio_ctx));
    }

    /* VAD - resolve relative to models_path */
    if (!cfg->no_silero_vad && cfg->vad_model[0] != '\0') {
        g_ptr_array_add(args, g_strdup("-vm"));
        {
            gchar *full_vad = NULL;
            if (cfg->vad_model[0] == '/') {
                full_vad = g_strdup(cfg->vad_model);
            } else if (cfg->models_path[0] != '\0') {
                full_vad = g_strconcat(cfg->models_path, "/", cfg->vad_model, NULL);
            } else {
                full_vad = g_strdup(cfg->vad_model);
            }
            g_ptr_array_add(args, full_vad);
        }
    } else if (cfg->no_silero_vad) {
        g_ptr_array_add(args, g_strdup("-nsv"));
    }

    /* VAD thresholds (energy-based) */
    if (cfg->vad_threshold != 0.6) {
        g_ptr_array_add(args, g_strdup("-vth"));
        g_ptr_array_add(args, g_strdup_printf("%.1f", cfg->vad_threshold));
    }
    if (cfg->freq_threshold != 100.0) {
        g_ptr_array_add(args, g_strdup("-fth"));
        g_ptr_array_add(args, g_strdup_printf("%.1f", cfg->freq_threshold));
    }
    if (cfg->vad_gain != 1.0) {
        g_ptr_array_add(args, g_strdup("-vg"));
        g_ptr_array_add(args, g_strdup_printf("%.1f", cfg->vad_gain));
    }

    /* Output file */
    if (cfg->output_file[0] != '\0') {
        g_ptr_array_add(args, g_strdup("-f"));
        g_ptr_array_add(args, g_strdup(cfg->output_file));
    }

    /* Uinput */
    if (cfg->uinput) {
        g_ptr_array_add(args, g_strdup("-ui"));
    }

    /* Commands file */
    if (cfg->commands_file[0] != '\0') {
        g_ptr_array_add(args, g_strdup("-cm"));
        g_ptr_array_add(args, g_strdup(cfg->commands_file));
    }

    /* Detail mode */
    if (cfg->detail) {
        g_ptr_array_add(args, g_strdup("-d"));
    }

    /* No status (we capture it in GUI, but user may want to disable) */
    if (cfg->no_status) {
        g_ptr_array_add(args, g_strdup("-ns"));
    }

    /* Print special tokens */
    if (cfg->print_special) {
        g_ptr_array_add(args, g_strdup("-ps"));
    }

    /* Tinydiarize */
    if (cfg->tinydiarize) {
        g_ptr_array_add(args, g_strdup("-tdrz"));
    }

    /* Save audio */
    if (cfg->save_audio) {
        g_ptr_array_add(args, g_strdup("-sa"));
    }

    /* No fallback */
    if (cfg->no_fallback) {
        g_ptr_array_add(args, g_strdup("-nf"));
    }

    /* Capture device */
    if (cfg->capture_device >= 0) {
        g_ptr_array_add(args, g_strdup("-c"));
        g_ptr_array_add(args, g_strdup_printf("%d", cfg->capture_device));
    }

    /* Null-terminate */
    g_ptr_array_add(args, NULL);

    *argc = (int)args->len - 1;
    *out_args = (char **)g_ptr_array_free(args, FALSE);
}
