# Upstream proposal draft

This is a draft for an upstream issue/RFC and a later merge request. Fill in
the repository and demo links before posting, and confirm the target repository
with a maintainer.

## Recommended path

1. Open an issue/RFC in `plasma/kdeplasma-addons` first. The applet is a new
   UX, and the KScreen/Wayland path is not runtime-tested yet.
2. Ask whether the suggestion belongs in `kdeplasma-addons` or should be
   integrated into KScreen/KWin instead.
3. Publish a public fork/branch, add a small test for `RotationMapping`, and
   record the front-end demo.
4. Open a **Draft** merge request only after a maintainer agrees on the scope.
5. Keep the Auto-rotate tray toggle out of the first MR. It overlaps the
   KScreen KCM and `kscreen-doctor`. Keep the L4T backend downstream only.

## Issue / RFC

### Title

Rotation suggestion applet for 2-in-1 devices

### Body

```markdown
## Problem

On 2-in-1 laptops and tablets running Plasma Wayland, automatic rotation can be
off. When the device is physically rotated, the screen stays in the old
orientation until the user opens Display and Monitor settings or runs
`kscreen-doctor`. That is a lot of friction for a common action.

Users have requested this before:

- https://www.reddit.com/r/kde/comments/1jxo0fa/autorotate_lock/
- https://www.reddit.com/r/kde/comments/1dg1eqq/is_there_a_way_to_unlock_screen_rotation_via/

## Proposal

Add a small panel applet that appears only when:

- automatic rotation is off,
- the device orientation changes,
- the session has a usable accelerometer and one enabled output.

It shows a single rotate suggestion for 8 seconds. Clicking it applies the
matching rotation through KScreen. If the user does nothing, it disappears. It
does not continuously animate or poll.

The applet claims `iio-sensor-proxy` only while automatic rotation is off, and
hides itself when the sensor or KScreen's `AutoRotation` feature is unavailable.

## Relationship to existing upstream work

Plasma 6.7 already exposes `AutoRotatePolicy` through `kscreen-doctor`. This
applet uses the same libkscreen API (`GetConfigOperation` /
`SetConfigOperation`); it does not edit private configuration files or
reimplement auto-rotation. The new part is the orientation-change suggestion
UX.

## Implementation

- Shared QML UI with a small C++ QML plugin for sensor and KScreen integration.
- Uses KScreen `AutoRotatePolicy` and `SetConfigOperation`.
- KWin handles touch-input mapping on Wayland.
- No new dependency beyond libkscreen 6.7, already required by Plasma 6.7.
- The L4T/Switch backend is downstream and is not part of this proposal.

## Testing

- Builds against Plasma 6.7.5 and libkscreen 6.7.5 on Fedora 44.
- The KScreen/Wayland runtime path has **not** been tested on a sensor-equipped
  device yet. A test on a Lenovo Yoga laptop is pending.
- The L4T/X11 backend was runtime-tested on a Nintendo Switch, but it is not
  part of this proposal.
- Testing on other 2-in-1 hardware would be very welcome.

The demo below shows the front-end UI/UX only. It is not a Wayland test.

## Demo

[front-end demo link]

## Open questions

- Should this live in `kdeplasma-addons`, or should the suggestion be
  integrated into KScreen/KWin?
- Is the tray Auto-rotate toggle wanted upstream, or is the KScreen KCM /
  `kscreen-doctor` enough? The downstream package includes it; it can be left
  out of the upstream MR.
- The 8-second timeout and panel-only placement are open to tuning.

## Credits

This would not have been this simple without Plasma Mobile. It had already put
the rotation and sensor pieces together in Plasma, and its native mobile design
was the reference for this applet. Thank you to the Plasma Mobile contributors.

The underlying `AutoRotatePolicy` API comes from libkscreen MR !291 by Amit
Chaudhari. Thank you to Amit and the KScreen reviewers.
```

## Merge request

### Title

```text
Draft: applets/rotation-suggestion: add orientation-change suggestion
```

### Body

```markdown
This adds a panel applet that suggests a rotation when the device orientation
changes while automatic rotation is off. The suggestion is shown for 8 seconds
and applies the matching rotation through KScreen when clicked.

It uses the existing KScreen `AutoRotatePolicy` / `SetConfigOperation` API and
does not reimplement auto-rotation or touch mapping. The applet claims
`iio-sensor-proxy` only while needed and hides itself when the sensor or
KScreen's `AutoRotation` feature is unavailable.

This is a draft because the KScreen/Wayland path has not yet been runtime-tested
on a sensor-equipped device. The L4T/Switch backend and the Auto-rotate tray
toggle are intentionally not part of this MR.

## Testing

- Build: Plasma 6.7.5, libkscreen 6.7.5, Fedora 44.
- KScreen/Wayland runtime: not tested yet; a Lenovo Yoga test is pending.
- Demo: [front-end demo link] — front-end UI/UX only, not a Wayland test.

## Notes

- Uses no private configuration files and no new dependencies.
- The L4T backend stays downstream.
- The Auto-rotate tray toggle can be a separate proposal if wanted.
```

## Demo caption

```text
Front-end preview only — the KScreen/Wayland backend is not runtime-tested yet.
```

## Before posting

- [ ] Confirm the target repository with a maintainer.
- [ ] Publish a public fork and branch.
- [ ] Add a small unit test for `RotationMapping`.
- [ ] Add the demo link and keep the front-end-only disclaimer.
- [ ] Keep the MR in Draft until the Wayland test or maintainer feedback.
- [ ] Do not include L4T code, packaging, or the tray toggle in the first MR.
