#include "process.h"

#include <glib.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

struct WhisperProcess {
    GPid pid;
    bool running;
    GPid child_pid;

    /* File descriptors */
    int stdout_fd;
    int stderr_fd;

    /* GIO watches */
    guint stdout_watch;
    guint stderr_watch;
    gulong child_watch_id;

    /* Buffer for partial lines */
    GString *stdout_buffer;

    /* Callbacks */
    StatusChangedCb status_cb;
    TranscriptionLineCb transcription_cb;
    ProcessExitedCb exited_cb;
    ProcessErrorCb error_cb;
    void *user_data;

    /* Current status for tracking */
    WhisperStatus current_status;
    PauseFlags current_flags;
};

static WhisperStatus parse_status_label(const char *label) {
    if (g_str_has_prefix(label, "idle")) return STATUS_IDLE;
    if (g_str_has_prefix(label, "listening")) return STATUS_LISTENING;
    if (g_str_has_prefix(label, "capturing")) return STATUS_CAPTURING;
    if (g_str_has_prefix(label, "processing")) return STATUS_PROCESSING;
    return STATUS_UNKNOWN;
}

static void parse_status_line(WhisperProcess *proc, const char *line) {
    /* Expected format: "[status]" or "[status (flags)]" */
    /* Strip leading/trailing whitespace and carriage returns */
    gchar *trimmed = g_strchug(g_strchomp(g_strdup(line)));

    const char *start = strchr(trimmed, '[');
    const char *end = strchr(trimmed, ']');

    if (start && end && end > start) {
        size_t len = end - start - 1;
        char content[128];

        if (len >= sizeof(content)) len = sizeof(content) - 1;
        memcpy(content, start + 1, len);
        content[len] = '\0';

        /* Check for flags in parentheses */
        char *paren = strchr(content, '(');
        if (paren) {
            *paren = '\0';  /* Split at '(' */
        }

        /* Trim the status label */
        gchar *label = g_strchug(g_strchomp(content));
        WhisperStatus status = parse_status_label(label);

        /* Parse flags if present */
        PauseFlags flags = {false, false};
        if (paren) {
            char *flags_str = g_strchug(g_strchomp(paren + 1));
            if (strstr(flags_str, "p")) flags.print_paused = true;
            if (strstr(flags_str, "Si")) flags.uinput_paused = true;
        }

        proc->current_status = status;
        proc->current_flags = flags;

        if (proc->status_cb) {
            proc->status_cb(status, &flags, proc->user_data);
        }
    }

    g_free(trimmed);
}

static bool is_status_line(const char *line) {
    /* Check if line looks like a status indicator: "[...]" */
    /* Search for '[' anywhere in line to handle ANSI escape codes */
    const char *start = strchr(line, '[');
    if (!start) return false;
    const char *close = strchr(start, ']');
    if (!close) return false;
    /* Ensure nothing but whitespace after ']' */
    const char *after = close + 1;
    while (*after == ' ' || *after == '\r' || *after == '\n' || *after == '\t') after++;
    return *after == '\0';
}

static gboolean on_stdout_fd_readable(int fd, GIOCondition condition, gpointer user_data) {
    WhisperProcess *proc = (WhisperProcess *)user_data;
    (void)condition;

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        /* Append to buffer */
        g_string_append_len(proc->stdout_buffer, buf, n);
        /* Replace CR with LF in buffer */
        for (gsize i = 0; i < proc->stdout_buffer->len; i++) {
            if (proc->stdout_buffer->str[i] == '\r') proc->stdout_buffer->str[i] = '\n';
        }
        /* Strip ANSI codes from buffer */
        GRegex *regex = g_regex_new("\x1b\\[[0-9;]*[a-zA-Z]", G_REGEX_OPTIMIZE, 0, NULL);
        if (regex) {
            gchar *stripped = g_regex_replace_literal(regex, proc->stdout_buffer->str, -1, 0, "", 0, NULL);
            g_string_assign(proc->stdout_buffer, stripped);
            g_free(stripped);
            g_regex_unref(regex);
        }
        /* Search for status lines anywhere in buffer */
        char *data = proc->stdout_buffer->str;
        char *status_start = strstr(data, "[");
        while (status_start) {
            char *status_end = strchr(status_start, ']');
            if (status_end) {
                size_t len = status_end - status_start + 1;
                char tmp[256];
                if (len < sizeof(tmp)) {
                    memcpy(tmp, status_start, len);
                    tmp[len] = '\0';
                    parse_status_line(proc, tmp);
                }
            }
            status_start = status_end ? strstr(status_end + 1, "[") : NULL;
        }
        /* Process buffer line by line for transcription */
        char *start = data;
        char *end;
        while ((end = strchr(start, '\n')) != NULL) {
            *end = '\0';
            if (start[0] != '\0') {
                if (!is_status_line(start)) {
                    if (proc->transcription_cb) {
                        proc->transcription_cb(start, proc->user_data);
                    }
                }
            }
            start = end + 1;
        }
        /* Remove processed data from buffer */
        size_t processed = start - data;
        g_string_erase(proc->stdout_buffer, 0, processed);
    }
    if (n == 0) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_stderr_fd_readable(int fd, GIOCondition condition, gpointer user_data) {
    WhisperProcess *proc = (WhisperProcess *)user_data;
    (void)condition;

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }

    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        return G_SOURCE_REMOVE;
    }
    buf[n] = '\0';
    char *line = buf;
    char *end;
    while ((end = strchr(line, '\n')) != NULL) {
        *end = '\0';
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\r') line[len-1] = '\0';
        if (line[0] != '\0' && proc->error_cb) {
            proc->error_cb(line, proc->user_data);
        }
        line = end + 1;
    }
    return G_SOURCE_CONTINUE;
}

static void on_child_exited(GPid pid, gint status, gpointer user_data) {
    WhisperProcess *proc = (WhisperProcess *)user_data;
    (void)pid;

    proc->running = false;
    proc->child_watch_id = 0;

    int exit_code = -1;
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code = -WTERMSIG(status);
    }

    if (proc->exited_cb) {
        proc->exited_cb(exit_code, proc->user_data);
    }
}

WhisperProcess *whisper_process_new(void) {
    WhisperProcess *proc = g_new0(WhisperProcess, 1);
    proc->pid = 0;
    proc->running = false;
    proc->stdout_fd = -1;
    proc->stderr_fd = -1;
    proc->stdout_buffer = g_string_new(NULL);
    proc->current_status = STATUS_IDLE;
    return proc;
}

void whisper_process_free(WhisperProcess *proc) {
    if (!proc) return;

    /* Remove watches */
    if (proc->stdout_watch) {
        g_source_remove(proc->stdout_watch);
    }
    if (proc->stderr_watch) {
        g_source_remove(proc->stderr_watch);
    }
    if (proc->child_watch_id) {
        g_source_remove(proc->child_watch_id);
    }

    /* Close file descriptors */
    if (proc->stdout_fd >= 0) close(proc->stdout_fd);
    if (proc->stderr_fd >= 0) close(proc->stderr_fd);

    if (proc->stdout_buffer) g_string_free(proc->stdout_buffer, TRUE);

    g_free(proc);
}

bool whisper_process_start(WhisperProcess *proc,
                           const WhisperEchoConfig *cfg,
                           const char *working_dir,
                           StatusChangedCb status_cb,
                           TranscriptionLineCb transcription_cb,
                           ProcessExitedCb exited_cb,
                           ProcessErrorCb error_cb,
                           void *user_data) {
    if (proc->running) {
        return false;
    }

    /* Build argument array */
    char **args = NULL;
    int argc = 0;
    config_to_args(cfg, &args, &argc);

    if (!args || argc == 0) {
        return false;
    }

    /* Let g_spawn_async_with_pipes create pipes */
    int stdout_fd = -1;
    int stderr_fd = -1;

    /* Store callbacks */
    proc->status_cb = status_cb;
    proc->transcription_cb = transcription_cb;
    proc->exited_cb = exited_cb;
    proc->error_cb = error_cb;
    proc->user_data = user_data;

    /* Spawn the process */
    GError *error = NULL;

    /* Set working directory to where whisper-echo models are */
    const char *cwd = working_dir ? working_dir : ".";

    GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
    /* Only search PATH if binary path has no directory component */
    if (!g_path_is_absolute(args[0]) && !strchr(args[0], '/')) {
        flags |= G_SPAWN_SEARCH_PATH;
    }

    gboolean ok = g_spawn_async_with_pipes(
        cwd,
        args,
        NULL,
        flags,
        NULL,
        NULL,
        &proc->child_pid,
        NULL,           /* stdin */
        &stdout_fd,     /* stdout */
        &stderr_fd,     /* stderr */
        &error
    );

    /* Clean up args */
    for (int i = 0; i <= argc; i++) {
        g_free(args[i]);
    }
    g_free(args);

    /* Keep write end open to avoid SIGPIPE */

    if (!ok) {
        if (stdout_fd >= 0) close(stdout_fd);
        if (stderr_fd >= 0) close(stderr_fd);
        fprintf(stderr, "Failed to spawn whisper-echo: %s\n", error->message);
        if (error_cb) error_cb(error->message, user_data);
        g_error_free(error);
        return false;
    }

    proc->pid = proc->child_pid;
    proc->running = true;
    proc->stdout_fd = stdout_fd;
    proc->stderr_fd = stderr_fd;
    proc->current_status = STATUS_IDLE;

    /* Make fd non-blocking */
    g_unix_set_fd_nonblocking(proc->stdout_fd, TRUE, NULL);
    g_unix_set_fd_nonblocking(proc->stderr_fd, TRUE, NULL);

    /* Watch stdout via fd watch */
    proc->stdout_watch = g_unix_fd_add(proc->stdout_fd, G_IO_IN | G_IO_HUP | G_IO_ERR,
                                       (GUnixFDSourceFunc)on_stdout_fd_readable, proc);
    /* Watch stderr via fd watch */
    proc->stderr_watch = g_unix_fd_add(proc->stderr_fd, G_IO_IN | G_IO_HUP | G_IO_ERR,
                                       (GUnixFDSourceFunc)on_stderr_fd_readable, proc);

    /* Watch for child exit */
    proc->child_watch_id = g_child_watch_add_full(G_PRIORITY_DEFAULT, proc->child_pid,
                                                   on_child_exited, proc, NULL);

    return true;
}

bool whisper_process_stop(WhisperProcess *proc) {
    if (!proc->running || proc->child_pid == 0) {
        return false;
    }

    if (kill(proc->child_pid, SIGTERM) == 0) {
        return true;
    }
    return false;
}

bool whisper_process_pause(WhisperProcess *proc) {
    if (!proc->running || proc->child_pid == 0) {
        return false;
    }

    if (kill(proc->child_pid, SIGSTOP) == 0) {
        return true;
    }
    return false;
}

bool whisper_process_resume(WhisperProcess *proc) {
    if (!proc->running || proc->child_pid == 0) {
        return false;
    }

    if (kill(proc->child_pid, SIGCONT) == 0) {
        return true;
    }
    return false;
}

bool whisper_process_is_running(WhisperProcess *proc) {
    return proc->running;
}

GPid whisper_process_get_pid(WhisperProcess *proc) {
    return proc->child_pid;
}
