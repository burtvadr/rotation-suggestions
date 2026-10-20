# Rotation Suggestions

Two native Plasma applets for L4T-based Nintendo Switch systems running X11:

- **Rotation Suggestion** — a panel applet that appears only when the device
  orientation changes while Auto-rotate is off. Click it within 8 seconds to
  rotate the screen and touch input to match the device.
- **Auto-rotate** — a system-tray popup applet that starts or stops the
  existing `auto-rotate.service`. It appears in the tray's **Status and
  Notifications** grid (the same popup as the other status items), not in the
  panel or the tray bar.

This project does not implement auto-rotation. It drives the L4T Switch BSP's
existing `/usr/bin/auto-rotate` script and `auto-rotate.service`. That script
owns the `xrandr` call and the touchscreen transformation matrix.

## Scope

This package is **L4T/Nintendo Switch-specific**, not generic X11. It depends on
the Switch BSP's `/usr/bin/auto-rotate`, `auto-rotate.service`,
`switch-x11-session`, the `DSI-*` panel, and the `stmfts` touch device. The
rotation backend is X11-specific (xrandr plus the XInput touch matrix); the UI
is generic Plasma 6. A standard Plasma version would use KScreen on Wayland
instead.

## Repository layout

One repository holds the shared applet UI, the current L4T/X11 backend, and the
L4T Fedora packaging:

- `packages/` — shared Plasma applets and metadata.
- `src/` — the QML plugin and the current L4T/X11 backend.
- `packaging/` — the L4T Fedora spec.

No separate `nintendo-switch/` copy or second repository is needed. The L4T
package is identified by the `-l4t` suffix. When the KScreen/Wayland backend is
added, keep the applet UI shared and select the backend in CMake instead of
forking the applets.

## Rotation mapping

Starting from normal landscape:

| Device orientation | Sensor | Screen rotation |
| --- | --- | --- |
| landscape | `normal` | `normal` |
| 90° counter-clockwise | `left-up` | `left` |
| 90° clockwise | `right-up` | `right` |
| 180° (upside down) | `bottom-up` | `inverted` |

The suggestion follows the same mapping. The touch matrix is handled by
`/usr/bin/auto-rotate`.

## Layout

- Add **Rotation Suggestion** to the panel. It is panel-only; its metadata
  deliberately does not set `X-Plasma-NotificationArea`, otherwise Plasma
  clones it into the system tray.
- Add **Auto-rotate** to the system tray from **Configure System Tray →
  Entries → Auto-rotate**. The applet is `Passive`, so it appears in the tray
  popup grid (**Status and Notifications**), not in the tray bar. Use **Show
  only in popup** if the tray settings offer it.
- The Auto-rotate metadata sets `X-Plasma-NotificationArea` and
  `X-Plasma-NotificationAreaCategory=SystemServices`; the suggestion metadata
  does not.

The suggestion disappears after 8 seconds; rotate the device again to bring it
back.

## Build

Install the build dependencies, then build and install manually:

```sh
sudo dnf install cmake gcc-c++ extra-cmake-modules ninja-build \
    qt6-qtbase-devel qt6-qtdeclarative-devel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j2
sudo cmake --install build
```

The plugin is installed under the Qt QML directory
(`/usr/lib64/qt6/qml/org/kde/plasma/private/rotation` on aarch64) and both
applets to `/usr/share/plasma/plasmoids`. A manual install is for development;
the RPM is the supported install.

## Packaging (Fedora 44 / L4T Nintendo Switch)

The spec is `packaging/plasma-rotation-suggestions-l4t.spec`. It builds one
aarch64 RPM containing both applets and the QML plugin. The `nvidia-l4t-bsp`
package owns `/usr/bin/auto-rotate`, `/usr/lib/systemd/user/auto-rotate.service`,
and `/usr/libexec/switch-x11-session`; the spec requires those file paths rather
than shipping copies.

The package was renamed from `plasma-rotation-suggestions`; remove the old
package before installing the new one:

```sh
sudo dnf remove plasma-rotation-suggestions
```

If you previously ran `cmake --install`, remove the manual applet and plugin
files first.

On the L4S Switch image, `kernel*` is excluded by the repo overrides. Install
the RPM macro package with the override before `rpm-build`:

```sh
sudo dnf --setopt="*.excludepkgs=" install -y kernel-srpm-macros
sudo dnf install -y rpm-build rpmdevtools
sudo dnf builddep -y packaging/plasma-rotation-suggestions-l4t.spec
rpmdev-setuptree
git archive --format=tar.gz --prefix=rotation_suggestions-0.1.0/ \
    -o ~/rpmbuild/SOURCES/rotation_suggestions-0.1.0.tar.gz HEAD
rpmbuild -ba packaging/plasma-rotation-suggestions-l4t.spec
sudo dnf install ~/rpmbuild/RPMS/aarch64/plasma-rotation-suggestions-l4t-0.1.0-1.fc44.aarch64.rpm
```

The spec currently uses a local source tarball. Before publishing, set `URL`
and `Source0` to the public repository archive.

Then restart Plasmashell and add the applets as described above. The package
does not enable `auto-rotate.service` at login; the Auto-rotate toggle still
starts and stops it for the current session.

## Test

```sh
plasmawindowed org.kde.plasma.rotationsuggestion
plasmawindowed org.kde.plasma.screenrotation
```

Then add the suggestion to a panel and the toggle to the system tray as
described above.

The sensor claim is allowed only in the active graphical session. A claim from
an SSH shell fails with `Not Authorized: Sensor claim not allowed`; test from
the panel, the tray, or a transient user service.

## Session behavior

- The Auto-rotate toggle starts/stops `auto-rotate.service` for the current
  session via systemd user D-Bus. It does not enable the unit at login; the
  unit remains disabled unless you enable it yourself.
- The suggestion claims the accelerometer only while Auto-rotate is off. It
  releases the sensor while the service is activating/active, and re-claims it
  when the service stops.
- The 8-second timer is restarted whenever the suggested orientation changes.

## Upstream direction

The applet UI and the `RotationUtil` QML interface are the portable part. The
systemd service backend is L4T-specific. A standard Plasma version should
replace it with KScreen's rotation API and use KWin's touch mapping on Wayland,
keeping the same applet behaviour. Both backends can live in this repository;
the L4T package selects the systemd/X11 backend.

## License

GPL-2.0-or-later.
