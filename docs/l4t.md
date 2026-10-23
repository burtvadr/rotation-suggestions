# L4T / Nintendo Switch backend

This backend is for L4T-based Nintendo Switch systems running Plasma 6 on X11.
It drives the rotation mechanism that the L4T BSP already provides; it does not
implement auto-rotation, the X11 touch matrix, or the panel setup itself.

The applet UI and the QML interface are shared with the standard KScreen
backend. The `-l4t` package selects this backend at build time.

## Tested environment

- Fedora 44 aarch64
- Plasma 6.7.5 on X11
- DSI-0 panel at 1280x720
- `stmfts` touch device

## BSP dependencies

The `nvidia-l4t-bsp` package owns these files. Do not ship copies of them:

- `/usr/bin/auto-rotate`
- `/usr/lib/systemd/user/auto-rotate.service`
- `/usr/libexec/switch-x11-session`

The package requires those file paths. `/usr/bin/auto-rotate` owns the `xrandr`
call and the touchscreen transformation matrix; the applet starts and stops
`auto-rotate.service` and calls `auto-rotate --once <orientation>` for a manual
rotation.

## Build

Install the build dependencies:

```sh
sudo dnf install cmake gcc-c++ extra-cmake-modules ninja-build \
    qt6-qtbase-devel qt6-qtdeclarative-devel
```

Build with the L4T backend:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DROTATION_BACKEND=l4t -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j2
sudo cmake --install build
```

The plugin is installed under the Qt QML directory
(`/usr/lib64/qt6/qml/org/kde/plasma/private/rotation` on aarch64) and both
applets to `/usr/share/plasma/plasmoids`. A manual install is for development;
the RPM is the supported install.

## Fedora package

The spec is `packaging/plasma-rotation-suggestions-l4t.spec`. It builds one
aarch64 RPM containing both applets and the QML plugin.

The package was renamed from `plasma-rotation-suggestions`. Remove the old
package before installing the new one:

```sh
sudo dnf remove plasma-rotation-suggestions
```

If you previously ran `cmake --install`, remove the manual applet and plugin
files first.

On the L4T Switch image, `kernel*` is excluded by the repo overrides. Install
the RPM macro package with the override before `rpm-build`:

```sh
sudo dnf --setopt="*.excludepkgs=" install -y kernel-srpm-macros
sudo dnf install -y rpm-build rpmdevtools
sudo dnf builddep -y packaging/plasma-rotation-suggestions-l4t.spec
rpmdev-setuptree
git archive --format=tar.gz --prefix=rotation_suggestions-0.2.0/ \
    -o ~/rpmbuild/SOURCES/rotation_suggestions-0.2.0.tar.gz HEAD
rpmbuild -ba packaging/plasma-rotation-suggestions-l4t.spec
sudo dnf install ~/rpmbuild/RPMS/aarch64/plasma-rotation-suggestions-l4t-0.2.0-1.fc44.aarch64.rpm
```

The spec currently uses a local source tarball. Before publishing, set `URL`
and `Source0` to the public repository archive.

The package does not enable `auto-rotate.service` at login. The Auto-rotate
toggle starts and stops it for the current session.

## Applet setup

- Enable **Rotation Suggestion** in **Configure System Tray → Entries →
  Rotation Suggestion**. It is an `Active` tray item: it appears in the tray
  bar only while a suggestion exists, then hides after 8 seconds. It does not
  need a panel slot.
- Enable **Auto-rotate** in the same place. It is `Passive`, so it appears in
  the tray popup grid (**Status and Notifications**), not the tray bar. It
  hides itself when the BSP rotation service or the accelerometer is missing.
- If an older installation added the suggestion to the panel, remove that
  panel instance after upgrading. The tray item replaces it.
- The suggestion is categorized as `ApplicationStatus`, so it normally sorts
  ahead of the persistent system and hardware icons. Another
  `ApplicationStatus` item can still share the group.
- The suggestion disappears after 8 seconds; rotate the device again to bring
  it back.

## Rotation mapping

Starting from a normal landscape panel:

| Device orientation | Sensor value | Screen rotation |
| --- | --- | --- |
| landscape | `normal` | `normal` |
| 90° counter-clockwise | `left-up` | `left` |
| 90° clockwise | `right-up` | `right` |
| 180° (upside down) | `bottom-up` | `inverted` |

The touch matrix is handled by `/usr/bin/auto-rotate`. The shared mapping also
handles a panel whose native mode is portrait.

## Session behavior

- The Auto-rotate toggle starts/stops `auto-rotate.service` for the current
  session via systemd user D-Bus. It does not enable the unit at login; the
  unit remains disabled unless you enable it yourself.
- The suggestion claims the accelerometer only while Auto-rotate is off. It
  releases the sensor while the service is activating/active, and re-claims it
  when the service stops.
- The 8-second timer is restarted whenever the suggested orientation changes.

## Test

```sh
plasmawindowed org.kde.plasma.rotationsuggestion
plasmawindowed org.kde.plasma.screenrotation
```

Then enable both items in the system tray as described above.

The sensor claim is allowed only in the active graphical session. A claim from
an SSH shell fails with `Not Authorized: Sensor claim not allowed`; test from
the tray or a transient user service.

Tray behavior was verified by forcing a rotation mismatch: the suggestion
appeared in the tray bar and dismissed after 8 seconds. A physical orientation
change uses the same `suggestionVisible` path.

## Limitations

- X11 only. The standard backend is the KScreen/Wayland implementation.
- The package does not implement auto-rotation or ship the X11 touch matrix.
- `auto-rotate.service` is not enabled at login by this package.
