#ifndef PROCESS_H
#define PROCESS_H

#include <glib.h>
#include <stdbool.h>
#include "config.h"

/* Status values parsed from whisper-echo output */
typedef enum {
    STATUS_IDLE,
    STATUS_LISTENING,
    STATUS_CAPTURING,
    STATUS_PROCESSING,
    STATUS_UNKNOWN
} WhisperStatus;

/* Pause flags parsed from status line */
typedef struct {
    bool print_paused;    /* 'p' flag - stdout+uinput paused */
    bool uinput_paused;   /* 'Si' flag - uinput paused */
} PauseFlags;

/* Callbacks for process events */
typedef void (*StatusChangedCb)(WhisperStatus status, const PauseFlags *flags, void *user_data);
typedef void (*TranscriptionLineCb)(const char *line, void *user_data);
typedef void (*ProcessExitedCb)(int exit_code, void *user_data);
typedef void (*ProcessErrorCb)(const char *error, void *user_data);

/* Opaque handle to a running process */
typedef struct WhisperProcess WhisperProcess;

/* Create a new process handle */
WhisperProcess *whisper_process_new(void);

/* Destroy the process handle (does not kill running process) */
void whisper_process_free(WhisperProcess *proc);

/* Start whisper-echo with the given config */
bool whisper_process_start(WhisperProcess *proc,
                           const WhisperEchoConfig *cfg,
                           const char *working_dir,
                           StatusChangedCb status_cb,
                           TranscriptionLineCb transcription_cb,
                           ProcessExitedCb exited_cb,
                           ProcessErrorCb error_cb,
                           void *user_data);

/* Stop the running process (SIGTERM) */
bool whisper_process_stop(WhisperProcess *proc);

/* Pause the running process (SIGSTOP) */
bool whisper_process_pause(WhisperProcess *proc);

/* Resume a paused process (SIGCONT) */
bool whisper_process_resume(WhisperProcess *proc);

/* Check if process is currently running */
bool whisper_process_is_running(WhisperProcess *proc);

/* Get the PID of the running process, or 0 if not running */
GPid whisper_process_get_pid(WhisperProcess *proc);

#endif /* PROCESS_H */
