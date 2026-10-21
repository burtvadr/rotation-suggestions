# KScreen / Wayland backend

This is the default backend. It uses the same display-management stack as
Plasma's own **Display and Monitor** settings.

## Requirements

- Plasma 6.7 or newer running on Wayland
- KScreen and libkscreen 6.7 or newer
- `iio-sensor-proxy` for the accelerometer
- Build dependencies: `extra-cmake-modules`, Qt 6 development files, and
  `libkscreen` development files

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

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DROTATION_BACKEND=kscreen -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

The Fedora spec is `packaging/plasma-rotation-suggestions.spec`.

## Limitations

- The backend has not been tested on a Wayland session in this repository's
  current test environment. It is a standard KScreen implementation, but the
  applet behavior should be verified on the target compositor.
- KScreen does not update an X11 touch transformation matrix. Use the L4T
  backend on X11 systems whose rotation script owns touch mapping.
- Claiming the accelerometer requires an active graphical session. A claim from
  an SSH shell is rejected by `iio-sensor-proxy`.
