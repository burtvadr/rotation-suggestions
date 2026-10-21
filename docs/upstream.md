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
- The **Rotation Suggestion** applet claims the accelerometer through
  `iio-sensor-proxy` only while automatic rotation is off. It applies a manual
  rotation with `SetConfigOperation`.
- KWin handles touch-input mapping when the output rotation changes on Wayland.
- A suggestion is shown only with one enabled output and a known orientation.
  It is hidden while automatic rotation is on.
- The tray item hides itself when no accelerometer is available or the
  compositor does not advertise KScreen's `AutoRotation` feature.

## Relationship to kscreen-doctor

Plasma 6.7 can already change the same policy from the command line. This
project does not reimplement that mechanism: it uses libkscreen directly and
adds the orientation-change suggestion UI and the tray toggle. The suggestion
applet is the part that has no direct upstream equivalent.

The KScreen KCM is the **Display & Monitor** page in System Settings. It
already exposes automatic rotation; the tray toggle is a shortcut to the same
policy, not a new mechanism.

## Background and acknowledgements

This applet would not have been this simple without Plasma Mobile. It had
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
