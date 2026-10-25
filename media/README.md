# Media

This directory holds the demo recordings and the reference screenshots used to
choose the tray placement and wording.

The filenames are intentional and must not be renamed. Each OS or desktop uses
its own term for the same action, so the names follow the source platform:

- Windows calls it **Rotation lock**.
- GNOME calls it **Auto Rotate**.
- KDE Plasma Mobile calls it **Auto-rotate**.
- Android 17 shows a **rotation suggestion**.

## demo

| File | Shows |
| --- | --- |
| `demo/rotation suggestions tray.gif` | The Rotation Suggestion appears in the tray bar after the device orientation changes. |
| `demo/rotation suggestions tray click.gif` | Clicking the suggestion rotates the screen to match the device. |
| `demo/auto-rotate tray popup.gif` | The Passive Auto-rotate item in **Status and Notifications**, and toggling it. |

All three are 800x450 GIFs recorded on the L4T/X11 test device. They show the
front-end UI/UX only, not a Wayland test.

## reference

| File | Reference |
| --- | --- |
| `reference/rotation lock action center windows 10.jpeg` | Windows 10 Action Center, **Rotation lock**. |
| `reference/rotation lock quick settings windows 11.jpeg` | Windows 11 Quick Settings, **Rotation lock**. |
| `reference/auto rotate quick settings gnome.png` | GNOME Quick Settings, **Auto Rotate**. |
| `reference/auto-rotate quick settings kde mobile.png` | Plasma Mobile quick settings, **Auto-rotate**; the direct KDE precedent. |
| `reference/rotation suggestions android 17` | Android 17 rotation suggestion. |

The reference screenshots are not part of this project. They document the
familiar quick-settings placement that the Passive Auto-rotate item follows.

Some files do not match their container by extension: `rotation lock quick
settings windows 11.jpeg` is a PNG, and `rotation suggestions android 17` has
no extension. This is intentional; the names are kept exactly as captured.
