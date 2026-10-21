// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

namespace RotationMapping
{
// Maps an accelerometer orientation to the screen rotation needed to keep the
// image upright. panelMode is the panel's native mode, e.g. "1280x720".
QString targetRotation(const QString &panelMode, const QString &orientation);

// Clockwise direction from current to target: "right", "left" or "flip".
QString direction(const QString &target, const QString &current);
}
