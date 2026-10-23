# KScreen / Wayland backend

This is the default backend. It uses the same display-management stack as
Plasma's own **Display and Monitor** settings.

## Requirements

- Plasma 6.7 or newer running on Wayland
- KScreen and libkscreen 6.7 or newer
- `iio-sensor-proxy` for the accelerometer
- Build dependencies: `extra-cmake-modules`, Qt 6 development files, and
  `libkscreen` development files

## Status

The KScreen backend is **compile-tested only**. It has not been run on a
Wayland session with an accelerometer. A test on a Lenovo Yoga laptop is
pending; until then, treat the Wayland path as untested. The L4T backend is
runtime-tested on the Switch.

A demo recording is planned to show the front-end UI/UX only. It is not a
Wayland test and does not change the KScreen testing status.

## How it works

- The applet reads the display configuration with KScreen's
  `GetConfigOperation` and watches it with `ConfigMonitor`.
- The **Auto-rotate** toggle changes the primary output's
  `AutoRotatePolicy`. Turning it on uses `InTabletMode` when KScreen reports
  tablet-mode support, otherwise `Always`; turning it off uses `Never`.
- This is the same policy API that Plasma 6.7 exposes through
  `kscreen-doctor output.<OUTPUT>.autoRotatePolicy.<POLICY>`.
- The **Rotation Suggestion** is an `Active` tray item. It claims the
  accelerometer through `iio-sensor-proxy` only while automatic rotation is
  off, and appears in the tray bar only when the sensor is available and
  claimed, one output is enabled, and the orientation is known. It applies a
  manual rotation with `SetConfigOperation`.
- The **Auto-rotate** toggle is a `Passive` tray item. It hides itself when no
  accelerometer is available or the compositor does not advertise KScreen's
  `AutoRotation` feature.
- The suggestion uses `HiddenStatus` when it has nothing to show, so it does
  not occupy a permanent panel slot.
- It uses the `ApplicationStatus` tray category, the earliest category, so it
  sorts before persistent system and hardware icons. Plasma has no per-item
  priority flag; a category change is the only metadata-level way to move it
  first.
- KWin handles touch-input mapping when the output rotation changes on Wayland.

## Relationship to kscreen-doctor

Plasma 6.7 can already change the same policy from the command line. This
project does not reimplement that mechanism: it uses libkscreen directly and
adds the orientation-change suggestion UI and the Auto-rotate tray item. The
suggestion tray item is the part that has no direct upstream equivalent.

The KScreen KCM is the **Display & Monitor** page in System Settings. It
already exposes automatic rotation; the Auto-rotate tray item is a quick-access
entry point to the same policy, not a new mechanism. Plasma already does this
for display configuration: the Display Configuration tray item and the Display
& Monitor KCM are two entry points to the same settings.

## Background and acknowledgements

This design would not have been this simple without Plasma Mobile. It had
already put the rotation and sensor pieces together in Plasma, and its native
mobile design showed how clean this could look. Thank you to the Plasma Mobile
contributors.

The underlying `AutoRotatePolicy` API comes from libkscreen
[MR !291](https://invent.kde.org/plasma/libkscreen/-/merge_requests/291) by
Amit Chaudhari. Thank you to Amit and the KScreen reviewers.

Users have requested this behavior before:

- [Auto-rotate lock on a Lenovo Yoga](https://www.reddit.com/r/kde/comments/1jxo0fa/autorotate_lock/)
- [Unlock screen rotation via a command](https://www.reddit.com/r/kde/comments/1dg1eqq/is_there_a_way_to_unlock_screen_rotation_via/)

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DROTATION_BACKEND=kscreen -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

The Fedora spec is `packaging/plasma-rotation-suggestions.spec`.

## Limitations

- **Wayland runtime testing is pending.** The backend compiles, but it has not
  been run on a Wayland session with an accelerometer. A Lenovo Yoga test is
  pending. Verify the applet on the target compositor before relying on it.
- KScreen does not update an X11 touch transformation matrix. Use the L4T
  backend on X11 systems whose rotation script owns touch mapping.
- Claiming the accelerometer requires an active graphical session. A claim from
  an SSH shell is rejected by `iio-sensor-proxy`.
