# SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
# SPDX-License-Identifier: GPL-2.0-or-later

Name:           plasma-rotation-suggestions-l4t
Version:        0.2.0
Release:        1%{?dist}
Summary:        Plasma rotation tray items for the Nintendo Switch (L4T)
URL:            https://github.com/burtvadr/rotation-suggestions
License:        GPL-2.0-or-later
Source0:        https://github.com/burtvadr/rotation-suggestions/archive/refs/tags/v%{version}.tar.gz

# The backend is the L4T Switch BSP's X11 rotation script and DSI/touch hardware.
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
# Owned by the Nintendo Switch L4T BSP; do not duplicate or replace these.
Requires:       /usr/bin/auto-rotate
Requires:       /usr/lib/systemd/user/auto-rotate.service
Requires:       /usr/libexec/switch-x11-session
# Both packages install the same applet and plugin paths.
Conflicts:      plasma-rotation-suggestions

%description
Two native Plasma tray items for L4T-based Nintendo Switch systems running X11.

Rotation Suggestion is an Active tray item that appears in the tray bar when
the device orientation changes while Auto-rotate is off. Clicking it rotates the
screen and touch input to match the device. Auto-rotate is a Passive tray item
in the popup grid that starts and stops the Switch BSP's auto-rotate.service.

The items drive the existing /usr/bin/auto-rotate script and user service from
the Nintendo Switch L4T BSP package. They do not implement auto-rotation or ship
the X11 touch matrix themselves.

%prep
%autosetup -n rotation-suggestions-%{version}

%build
%cmake -DROTATION_BACKEND=l4t -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md docs/l4t.md docs/upstream.md
%{_datadir}/plasma/plasmoids/org.kde.plasma.rotationsuggestion/
%{_datadir}/plasma/plasmoids/org.kde.plasma.screenrotation/
%{_libdir}/qt6/qml/org/kde/plasma/private/rotation/

%changelog
* Tue Oct 20 2026 Rotation Suggestions contributors - 0.2.0-1
- Split the shared applet UI from the L4T and KScreen backends
- Build the L4T package with the systemd/X11 backend
