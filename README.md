# Rotation Suggestions

Two native Plasma applets for the Nintendo Switch running X11:

- **Rotation Suggestion** — a panel applet that appears only when the device
  orientation changes while auto-rotate is off. Click it within 8 seconds to
  rotate the screen and touch input to match the device.
- **Auto Rotate** — a system-tray popup applet that starts or stops the
  existing `auto-rotate.service`. It appears in the tray's **Status and
  Notifications** grid (the same popup as the other status items), not in the
  panel or the tray bar.

This project does not implement auto-rotation. On the Switch it drives the
existing `/usr/bin/auto-rotate` script and `auto-rotate.service`. That script
owns the `xrandr` call and the touchscreen transformation matrix.

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
- Add **Auto Rotate** to the system tray from **Configure System Tray →
  Entries → Auto Rotate**. The applet is `Passive`, so it appears in the tray
  popup grid (**Status and Notifications**), not in the tray bar. Use **Show
  only in popup** if the tray settings offer it.
- The Auto Rotate metadata sets `X-Plasma-NotificationArea` and
  `X-Plasma-NotificationAreaCategory=SystemServices`; the suggestion metadata
  does not.

The suggestion disappears after 8 seconds; rotate the device again to bring it
back.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j2
sudo cmake --install build
```

The plugin is installed to the Qt QML directory and both applets to
`/usr/share/plasma/plasmoids`.

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

- The Auto Rotate toggle starts/stops `auto-rotate.service` for the current
  session via systemd user D-Bus. It does not enable the unit at login; the
  unit remains disabled unless you enable it yourself.
- The suggestion claims the accelerometer only while auto-rotate is off. It
  releases the sensor while the service is activating/active, and re-claims it
  when the service stops.
- The 8-second timer is restarted whenever the suggested orientation changes.

## Upstream direction

The applet UI and the `RotationUtil` interface are the portable part. The
systemd service backend is Switch-specific. A standard Plasma version should
replace it with KScreen's `autoRotatePolicy` / rotation API and use KScreen's
touch mapping, keeping the same applet behaviour.

## License

GPL-2.0-or-later.
