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
4. Open one **Draft** MR with two independent commits: the suggestion applet
   first, the Auto-rotate tray toggle second. Lead with the suggestion, and
   state clearly that the toggle commit can be split out or dropped.
5. Treat splitting as the fallback, not the opening offer. The toggle is a
   quick-access entry point for the same policy, like the Display
   Configuration tray item next to the Display & Monitor KCM. Keep the first
   MR focused on the shared applet code and the KScreen backend.

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

A passive tray item turns automatic rotation on or off. It is a quick-access
entry point for the same `AutoRotatePolicy`, like the existing Display
Configuration tray item next to the Display & Monitor KCM. It does not add a
second mechanism.

## Relationship to existing upstream work

Plasma 6.7 already exposes `AutoRotatePolicy` through `kscreen-doctor`. These
applets use the same libkscreen API (`GetConfigOperation` /
`SetConfigOperation`); they do not edit private configuration files or
reimplement auto-rotation. The new part is the orientation-change suggestion
UX and the tray quick toggle. Plasma already provides multiple entry points
for display configuration, so a tray toggle is a convenience UI, not a second
mechanism.

## Implementation

- Shared QML UI with a small C++ QML plugin for sensor and KScreen integration.
- Uses KScreen `AutoRotatePolicy` and `SetConfigOperation`.
- KWin handles touch-input mapping on Wayland.
- No new dependency beyond libkscreen 6.7, already required by Plasma 6.7.

## Testing

- Builds against Plasma 6.7.5 and libkscreen 6.7.5 on Fedora 44.
- The KScreen/Wayland runtime path has **not** been tested on a sensor-equipped
  device yet. The only runtime test so far is the downstream Fedora L4T/X11
  backend on a Nintendo Switch; it is not part of this proposal. A test on a
  Lenovo Yoga laptop is pending.
- Testing on other 2-in-1 hardware would be very welcome.

The demo below shows the front-end UI/UX only. It is not a Wayland test.

## Demo

[front-end demo link]

## Open questions

- Should this live in `kdeplasma-addons`, or should the suggestion be
  integrated into KScreen/KWin?
- The tray Auto-rotate toggle is in a separate commit and can be split into a
  follow-up if preferred. It is a quick-access UI for the same policy, not a
  second mechanism.
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
Draft: applets: add rotation suggestion and auto-rotate tray applets
```

### Body

```markdown
This adds two small UIs for the same KScreen `AutoRotatePolicy`.

1. **Rotation Suggestion** — a panel applet that appears when the device
   orientation changes while automatic rotation is off. The suggestion is
   shown for 8 seconds and applies the matching rotation through KScreen when
   clicked.
2. **Auto-rotate** — a passive tray item that turns automatic rotation on or
   off. It is a quick-access entry point for the same policy, like the
   Display Configuration tray item next to the Display & Monitor KCM. It does
   not add a second mechanism.

The suggestion claims `iio-sensor-proxy` only while needed. Both UIs hide
themselves when the sensor or KScreen's `AutoRotation` feature is unavailable.
They use the existing `AutoRotatePolicy` / `SetConfigOperation` API and do not
reimplement auto-rotation or touch mapping.

The second commit is independent and can be split into a follow-up MR or
dropped if reviewers prefer. The MR contains only the KScreen backend.

This is a draft because the KScreen/Wayland path has not yet been runtime-tested
on a sensor-equipped device.

## Testing

- Build: Plasma 6.7.5, libkscreen 6.7.5, Fedora 44.
- KScreen/Wayland runtime: not tested yet; a Lenovo Yoga test is pending.
- Demo: [front-end demo link] — front-end UI/UX only, not a Wayland test.

## Notes

- Uses no private configuration files and no new dependencies.
- The tray toggle is a quick-access UI, not a second mechanism.
- Happy to split the two commits if that is easier to review.
```

## Split fallback

The suggestion applet remains the first MR; this is the follow-up.

### Title

```text
Draft: applets/screenrotation: add auto-rotate tray toggle
```

### Body

```markdown
This adds a passive tray item that turns automatic rotation on or off. It uses
the same KScreen `AutoRotatePolicy` as the Display & Monitor KCM and
`kscreen-doctor`; it does not add a second mechanism.

The item is a quick-access UI, like the existing Display Configuration tray
item next to the Display & Monitor KCM. It hides itself when no accelerometer
or no KScreen `AutoRotation` support is available.

This is a follow-up to the rotation suggestion applet. It can be reviewed
independently.
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
- [ ] Do not include downstream device code or packaging in the upstream MR.
- [ ] Keep the tray toggle as a separate commit so it can be split out.
- [ ] If reviewers ask to split, do it immediately; that is the intended
      fallback.
- [ ] If the toggle is rejected, keep the suggestion MR alive; do not make it
      conditional on the toggle.
