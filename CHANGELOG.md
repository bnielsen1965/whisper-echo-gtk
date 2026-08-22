# Changelog

All notable changes to whisper-echo-gtk will be documented in this file.

## [0.3.0] - 2026-08-21

### Added
* Model info row in the runtime pane showing the active transcription model and VAD model filenames
* Model info row dims when the VAD model is disabled or unset
* Rounded background shape to the application icon

### Changed
* Transcription view now follows the tail only while the cursor is at the end, so scrolling up through earlier text is no longer overridden

### Fixed
* N/A

## [0.2.0] - 2026-08-14

### Added
* Disable Settings and Help buttons while whisper-echo process is running to prevent dictation keystrokes from affecting dialogs
* Auto-focus transcription view when Settings or Help dialogs close
* Help dialog note explaining disabled buttons
* README documentation for disabled buttons
* CHANGELOG.md and changelog sync script

### Changed
* N/A

### Fixed
* N/A

## [0.1.0] - 2026-08-13

### Added
* Initial release
* GTK4/Libadwaita GUI frontend for whisper-echo
* Settings editor with model, performance, VAD, output, audio and general sections
* Live status display and transcription view
* Start/Stop controls with process monitoring
* UInput indicator
* Config persistence to `~/.whisper-echo/whisper-echo.conf`
* Help dialog with UI overview
* Debian and RPM packaging

### Changed
* N/A

### Fixed
* N/A
