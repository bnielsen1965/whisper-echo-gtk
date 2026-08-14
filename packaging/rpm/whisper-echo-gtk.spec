Name:           whisper-echo-gtk
Version:        0.2.0
Release:        1%{?dist}
Summary:        GTK4 GUI frontend for whisper-echo
License:        MIT
URL:            https://github.com/bnielsen1965/whisper-echo
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cmake >= 3.16, gcc-c++, make, gtk4-devel, libadwaita-devel
Requires:       whisper-echo >= 0.1.0, gtk4, libadwaita
BuildArch:      noarch

%description
GTK4 GUI frontend for whisper-echo. Provides a native GNOME desktop interface to configure whisper-echo settings, start/stop the dictation engine, monitor live status and transcription.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DCMAKE_INSTALL_LIBDIR=%{_lib}
cmake --build build %{?_smp_mflags}

%install
rm -rf %{buildroot}
cmake --install build --prefix %{buildroot}%{_prefix}

%files
%{_bindir}/whisper-echo-gtk
%{_datadir}/whisper-echo-gtk/
%{_datadir}/applications/whisper-echo-gtk.desktop
%{_datadir}/icons/hicolor/scalable/apps/whisper-echo-gtk.svg
%{_mandir}/man1/whisper-echo-gtk.1.gz
%{_datadir}/doc/whisper-echo-gtk/

%changelog
* Fri Aug 14 2026 Bryan Nielsen <bnielsen1965@gmail.com> - 0.2.0-1
- Disable Settings and Help buttons while process is running
- Auto-focus transcription view after dialogs close
- Update documentation

* Thu Aug 13 2026 Bryan Nielsen <bnielsen1965@gmail.com> - 0.1.0-1
- Initial RPM spec
