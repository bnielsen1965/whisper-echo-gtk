# whisper-echo-gtk

A GTK4/Libadwaita GUI frontend for [whisper-echo](https://github.com/bnielsen1965/whisper-echo), a real-time voice dictation application built on whisper.cpp.

whisper-echo-gtk provides a native GNOME desktop interface to configure whisper-echo settings, start/stop the dictation engine, monitor live status and transcription, and persist preferences to `~/.whisper-echo/whisper-echo.conf`.

## Features

* Full settings editor for model, performance, VAD, output and audio options in a modal Settings dialog
* Live status display with color coding: IDLE, LISTENING, CAPTURING, PROCESSING
* Start/Stop controls for the whisper-echo process
* Live transcription view with scrollback limit
* UInput indicator showing on/pending/off/stopped state
* INI-style config persistence, human editable
* File pickers for models directory, model, VAD model and commands file
* Automatic audio device enumeration via `whisper-echo --list-devices`
* CSS-styled UI with status colors

## Requirements

* C compiler supporting C17
* CMake >= 3.16
* GTK4 >= 4.0
* GLib 2.0 / GObject / Gio
* libadwaita-1
* pkg-config
* whisper-echo binary in `$PATH` or configured via config

Runtime config is stored at:
```
~/.whisper-echo/whisper-echo.conf
~/.whisper-echo/models/
```

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Install:
```bash
sudo cmake --install .
```

The build copies `data/style.css` to the build tree and installs the binary to `bin/` and data to `share/whisper-echo-gtk/`.

## Packaging

The project uses CMake CPack to build both RPM and DEB packages, mirroring the whisper-echo packaging layout.

Build packages:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cpack -G DEB   # produces whisper-echo-gtk-*.deb
cpack -G RPM   # produces whisper-echo-gtk-*.rpm
```

Version is derived from git tags via `git describe`. If no tag is present, `0.1.0-dev` is used.

Debian source packaging is also supported:
```bash
sudo apt install debhelper cmake build-essential libgtk-4-dev libadwaita-1-dev
dpkg-buildpackage -us -uc
```

Installed files:
* `bin/whisper-echo-gtk`
* `share/whisper-echo-gtk/` data
* `share/applications/whisper-echo-gtk.desktop`
* `share/icons/hicolor/scalable/apps/whisper-echo-gtk.svg`
* `share/man/man1/whisper-echo-gtk.1.gz`
* `share/doc/whisper-echo-gtk/`

Runtime dependencies: `whisper-echo >= 0.1.0`, `libgtk-4-1`, `libadwaita-1-0`.

Dependencies via pkg-config:
* `gtk4>=4.0`
* `glib-2.0 gobject-2.0 gio-2.0`
* `libadwaita-1`

## Configuration

On first run a default config is created at `~/.whisper-echo/whisper-echo.conf`. The file is INI-style with sections:

```
[models]
path = ~/.whisper-echo/models

[model]
path = ggml-base.en.bin
language = en
translate = false

[performance]
threads = 4
no_gpu = false
gpu_device = 0
no_flash_attn = false
beam_size = -1
audio_ctx = 0

[vad]
vad_model = ggml-silero-v6.2.0.bin
no_silero_vad = false
vad_threshold = 0.5
freq_threshold = 80.0
vad_gain = 1.5

[output]
file =
uinput = false
commands = ~/.whisper-echo/command.json
detail = false
no_status = false
print_special = false
tinydiarize = false
save_audio = false
no_fallback = false

[audio]
capture_device = -1

[general]
binary_path = whisper-echo
max_transcription_lines = 500
```

The GUI maps each field to widgets and writes changes back on start. Model and VAD paths are resolved relative to `models.path` unless absolute.

## Usage

Run:
```bash
whisper-echo-gtk
```

### User Interface

Main window:
* Header bar with title "Whisper Echo" and window controls
* Runtime area:
  * Status label with color: gray IDLE, green LISTENING, amber CAPTURING, blue PROCESSING
  * UInput indicator icon
  * Start, Stop, Settings and Help buttons
  * Read-only transcription text view with scrollback

Settings dialog opened via Settings button:
* Expanders:
  * Models Directory: `models.path` entry with browse
  * Model & Language: `model.path` with browse, language combo, translate checkbox
  * Performance: threads, CPU only, disable flash attention, beam size, audio ctx tokens
  * Voice Activity Detection: vad_model with browse, disable Silero VAD, vad threshold, freq threshold, vad gain
  * Output: output file, enable uinput typing, commands file with browse, no temperature fallback
  * Audio: capture device combo populated from `whisper-echo --list-devices`
  * General: whisper-echo binary path, max transcription lines

A warning is shown in the Settings dialog that changes apply on next Start.

### Controls

* **Start**: Saves config from the Settings dialog, builds argument vector via `config_to_args`, spawns whisper-echo with stdout/stderr pipes, displays status and transcription. Start is disabled while running.
* **Stop**: Sends SIGTERM to the child process and returns to IDLE. Stop is enabled while running.
* **Settings**: Opens modal Settings dialog. Changes are saved to `~/.whisper-echo/whisper-echo.conf` when Close is pressed and applied on next Start.
* **Help**: Shows an overview of whisper-echo and UI usage.

Status lines from whisper-echo are parsed from `[status]` or `[status (flags)]`. Flags `p` = print paused, `Si` = uinput paused.

## How Settings Are Utilized

All settings are stored in `WhisperEchoConfig` and persisted via GKeyFile. On Start:

1. Widgets → config struct via `sync_config_from_widgets`
2. `config_save` writes INI file
3. `config_to_args` builds CLI argv matching whisper-echo options:
   * `-m model`, `-l language`, `-t threads`, `-tr`, `-ng`, `-gd`, `-nfa`, `-bs`, `-ac`
   * `-vm vad_model` or `-nsv`
   * `-vth`, `-fth`, `-vg`
   * `-f output`, `-ui`, `-cm commands`, `-d`, `-ns`, `-ps`, `-tdrz`, `-sa`, `-nf`
   * `-c capture_device`
4. Process spawned with `g_spawn_async_with_pipes`
5. stdout parsed line-by-line; status lines update UI, others appended to transcription view
6. stderr forwarded as errors

The window is a regular GTK application window. No always-on-top mode is provided.

## Documentation

* Source: `src/main.c` application entry, `src/window.c` UI, `src/config.c` INI load/save/args, `src/process.c` spawn and signal handling
* Styles: `data/style.css`
* Build: `CMakeLists.txt`

## License

MIT License. See `LICENSE` file in this repository.
