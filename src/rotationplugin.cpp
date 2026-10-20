// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rotationplugin.h"

#include "rotationutil.h"

#include <qqml.h>

void RotationPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<RotationUtil>(uri, 1, 0, "RotationUtil");
}
