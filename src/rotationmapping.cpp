// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rotationmapping.h"

#include <QStringList>

namespace RotationMapping
{
QString targetRotation(const QString &panelMode, const QString &orientation)
{
    if (panelMode.isEmpty()) {
        return {};
    }

    const QStringList dimensions = panelMode.split(QLatin1Char('x'));
    if (dimensions.size() != 2) {
        return {};
    }

    bool widthOk = false;
    bool heightOk = false;
    const int width = dimensions.at(0).toInt(&widthOk);
    const int height = dimensions.at(1).toInt(&heightOk);
    if (!widthOk || !heightOk) {
        return {};
    }

    const bool landscape = width >= height;
    if (orientation == QLatin1String("normal")) {
        return landscape ? QStringLiteral("normal") : QStringLiteral("left");
    }
    if (orientation == QLatin1String("bottom-up")) {
        return landscape ? QStringLiteral("inverted") : QStringLiteral("right");
    }
    if (orientation == QLatin1String("right-up")) {
        return landscape ? QStringLiteral("right") : QStringLiteral("normal");
    }
    if (orientation == QLatin1String("left-up")) {
        return landscape ? QStringLiteral("left") : QStringLiteral("inverted");
    }
    return {};
}

QString direction(const QString &target, const QString &current)
{
    if (target.isEmpty() || current.isEmpty()) {
        return {};
    }

    const auto degrees = [](const QString &rotation) {
        if (rotation == QLatin1String("right")) {
            return 90;
        }
        if (rotation == QLatin1String("inverted")) {
            return 180;
        }
        if (rotation == QLatin1String("left")) {
            return 270;
        }
        return 0;
    };

    const int delta = (degrees(target) - degrees(current) + 360) % 360;
    if (delta == 90) {
        return QStringLiteral("right");
    }
    if (delta == 270) {
        return QStringLiteral("left");
    }
    if (delta == 180) {
        return QStringLiteral("flip");
    }
    return {};
}
}
