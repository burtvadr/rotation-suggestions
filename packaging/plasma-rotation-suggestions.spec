Name:           plasma-rotation-suggestions
Version:        0.1.0
Release:        1%{?dist}
Summary:        Plasma rotation suggestion and auto-rotate applets for Nintendo Switch

License:        GPL-2.0-or-later
URL:            https://github.com/rotation-suggestions/rotation_suggestions
Source0:        %{url}/archive/%{version}/rotation_suggestions-%{version}.tar.gz

# The backend is the Switch BSP's X11 rotation script and DSI/touch hardware.
ExclusiveArch:  aarch64

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  extra-cmake-modules
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel

Requires:       plasma-workspace >= 6.0
Requires:       libplasma >= 6.0
Requires:       kf6-kirigami
Requires:       qt6-qtdeclarative
Requires:       qt6-qtbase
Requires:       systemd
Requires:       iio-sensor-proxy
Requires:       xrandr
Requires:       xinput
# Owned by the Nintendo Switch BSP; do not duplicate or replace these.
Requires:       /usr/bin/auto-rotate
Requires:       /usr/lib/systemd/user/auto-rotate.service
Requires:       /usr/libexec/switch-x11-session

%description
Two native Plasma applets for the Nintendo Switch running X11.

Rotation Suggestion appears in the panel when the device orientation changes
while auto-rotate is off. Clicking it rotates the screen and touch input to
match the device. Auto-rotate is a passive system-tray popup item that starts
and stops the Switch BSP's auto-rotate.service.

The applets drive the existing /usr/bin/auto-rotate script and user service
from the Nintendo Switch BSP package. They do not implement auto-rotation or
ship the X11 touch matrix themselves.

%prep
%autosetup -n rotation_suggestions-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_datadir}/plasma/plasmoids/org.kde.plasma.rotationsuggestion/
%{_datadir}/plasma/plasmoids/org.kde.plasma.screenrotation/
%{_libdir}/qt6/qml/org/kde/plasma/private/rotation/

%changelog
* Tue Oct 20 2026 Rotation Suggestions contributors - 0.1.0-1
- Initial Nintendo Switch Fedora package
