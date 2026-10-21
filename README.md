# Rotation Suggestions

Two native Plasma 6 applets:

- **Rotation Suggestion** — a panel applet that appears only when the device
  orientation changes while automatic rotation is off. Click it within 8
  seconds to rotate the screen to match the device.
- **Auto-rotate** — a passive system-tray popup item that turns automatic
  rotation on or off. It lives in the tray's **Status and Notifications** grid,
  not in the panel or the tray bar.

The applets are a UI for the platform's rotation mechanism. They do not
implement auto-rotation or touch mapping themselves. The UI uses native Plasma
components and follows Plasma Mobile's design language.

## Status

- `l4t`: runtime-tested on a Nintendo Switch running Fedora 44 with Plasma
  6.7.5 on X11.
- `kscreen`: **compile-tested only**. It has not been run on a Wayland session
  with an accelerometer yet. A test on a Lenovo Yoga laptop is pending.

The KScreen backend is the upstream-facing implementation. Treat it as
untested until that hardware test is complete.

A demo recording is planned to show the front-end UI/UX only. It is not a
Wayland test and does not change the KScreen testing status.

## Backends

The applet UI is shared. The platform integration is selected at build time:

| Backend | Platform | Rotation mechanism |
| --- | --- | --- |
| `kscreen` (default) | Plasma 6.7+ with KScreen | KScreen output configuration and `AutoRotatePolicy`; KWin maps touch input on Wayland |
| `l4t` | L4T Nintendo Switch, X11 | The Switch BSP's auto-rotate service and script; the script updates the X11 touch matrix |

The KScreen backend uses the same `AutoRotatePolicy` API that Plasma 6.7
exposes through `kscreen-doctor`; it does not edit private configuration files.

```sh
cmake -S . -B build -DROTATION_BACKEND=kscreen
# or
cmake -S . -B build -DROTATION_BACKEND=l4t
```

The L4T backend and its package are documented in [docs/l4t.md](docs/l4t.md).
The standard KScreen backend is documented in
[docs/upstream.md](docs/upstream.md).

## Build

Requirements:

- CMake 3.20 or newer
- Qt 6.5 or newer
- Extra CMake Modules and KDE Frameworks 6
- For the `kscreen` backend: libkscreen 6.7 or newer development files

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DROTATION_BACKEND=kscreen -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

The plugin is installed in the Qt QML directory under
`org/kde/plasma/private/rotation`. Both applets are installed to
`/usr/share/plasma/plasmoids`.

## Usage

- Add **Rotation Suggestion** to a panel. It is panel-only; its metadata does
  not set `X-Plasma-NotificationArea`, so Plasma does not clone it into the
  system tray.
- Add **Auto-rotate** to the system tray from **Configure System Tray →
  Entries → Auto-rotate**. It is `Passive`, so it appears in the tray popup
  grid (**Status and Notifications**), not the tray bar. The item hides itself
  when the system has no usable accelerometer or no supported auto-rotation
  mechanism.
- The suggestion disappears after 8 seconds. Rotate the device again to bring
  it back.

Starting from a normal landscape panel, the suggestion uses this mapping:

| Device orientation | Sensor value | Screen rotation |
| --- | --- | --- |
| landscape | `normal` | `normal` |
| 90° counter-clockwise | `left-up` | `left` |
| 90° clockwise | `right-up` | `right` |
| 180° (upside down) | `bottom-up` | `inverted` |

The mapping also handles panels whose native mode is portrait.

## Packaging

The standard KScreen package is built from
`packaging/plasma-rotation-suggestions.spec`. The L4T package uses
`packaging/plasma-rotation-suggestions-l4t.spec`; its build and installation
steps are in [docs/l4t.md](docs/l4t.md). The `-l4t` suffix keeps the two
packages distinct.

## Acknowledgements

This project would not have been this simple without Plasma Mobile. It had
already put the rotation and sensor pieces together in Plasma, and its native
mobile design was the reference for these applets. The result fits Plasma
because of that work. Thank you to the Plasma Mobile contributors.

The KScreen backend uses the `AutoRotatePolicy` API added to libkscreen by Amit
Chaudhari in [MR !291](https://invent.kde.org/plasma/libkscreen/-/merge_requests/291).
Thank you to Amit and the KScreen reviewers.

Users have requested this behavior before:

- [Auto-rotate lock on a Lenovo Yoga](https://www.reddit.com/r/kde/comments/1jxo0fa/autorotate_lock/)
- [Unlock screen rotation via a command](https://www.reddit.com/r/kde/comments/1dg1eqq/is_there_a_way_to_unlock_screen_rotation_via/)

## License

GPL-2.0-or-later.
