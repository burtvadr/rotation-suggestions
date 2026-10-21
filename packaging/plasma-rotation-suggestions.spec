Name:           plasma-rotation-suggestions
Version:        0.2.0
Release:        1%{?dist}
Summary:        Plasma rotation suggestion and auto-rotate applets

License:        GPL-2.0-or-later
# Local source tarball. Point this at the public archive when publishing.
Source0:        rotation_suggestions-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  extra-cmake-modules
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  libkscreen-devel >= 6.7

Requires:       plasma-workspace >= 6.7
Requires:       libplasma >= 6.0
Requires:       kf6-kirigami
Requires:       qt6-qtdeclarative
Requires:       qt6-qtbase
Requires:       kscreen >= 6.7
Requires:       libkscreen >= 6.7
Requires:       iio-sensor-proxy
# Both packages install the same applet and plugin paths.
Conflicts:      plasma-rotation-suggestions-l4t

%description
Two native Plasma applets for KScreen-based Plasma 6.7+ systems.

Rotation Suggestion appears in the panel when the device orientation changes
while automatic rotation is off. Clicking it rotates the screen to match the
device. Auto-rotate is a passive system-tray popup item that turns automatic
rotation on or off.

The applets use KScreen's output configuration and AutoRotatePolicy. KWin
handles touch-input mapping when the output rotation changes on Wayland.

%prep
%autosetup -n rotation_suggestions-%{version}

%build
%cmake -DROTATION_BACKEND=kscreen -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md docs/upstream.md
%{_datadir}/plasma/plasmoids/org.kde.plasma.rotationsuggestion/
%{_datadir}/plasma/plasmoids/org.kde.plasma.screenrotation/
%{_libdir}/qt6/qml/org/kde/plasma/private/rotation/

%changelog
* Tue Oct 20 2026 Rotation Suggestions contributors - 0.2.0-1
- Split the shared applet UI from the L4T and KScreen backends
- Add the standard KScreen/Wayland package
