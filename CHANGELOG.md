# Changelog

All notable changes to whisper-echo-gtk will be documented in this file.

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
