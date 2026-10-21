// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "rotationbackend.h"

#include <kscreen/output.h>
#include <kscreen/types.h>

#include <QString>

#include <optional>

class SensorProxy;

namespace KScreen
{
class Config;
class GetConfigOperation;
class SetConfigOperation;
}

// Standard Plasma backend: KScreen owns the display configuration and the
// automatic-rotation policy. KWin maps touch input on Wayland.
class KScreenBackend : public RotationBackend
{
    Q_OBJECT

public:
    explicit KScreenBackend(QObject *parent = nullptr);
    ~KScreenBackend() override;

    bool autoRotateAvailable() const override;
    bool autoRotateEnabled() const override;
    void setAutoRotateEnabled(bool enabled) override;

    bool sensorAvailable() const override;
    bool sensorEnabled() const override;
    void setSensorEnabled(bool enabled) override;

    QString orientation() const override;
    QString screenRotation() const override;

    bool suggestionPossible() const override;
    QString targetRotationFor(const QString &orientation) const override;

    bool rotating() const override;
    void rotateToOrientation(const QString &orientation) override;

private:
    void reloadConfig();
    void setConfig(const KScreen::ConfigPtr &config);
    void updateState();
    void updateSensorClaim();
    void applyConfig();
    KScreen::OutputPtr primaryOutput() const;
    QString nativeMode(const KScreen::OutputPtr &output) const;
    QString rotationToString(KScreen::Output::Rotation rotation) const;
    std::optional<KScreen::Output::Rotation> rotationForTarget(const QString &target) const;

    KScreen::ConfigPtr m_config;
    KScreen::GetConfigOperation *m_reload = nullptr;
    KScreen::SetConfigOperation *m_apply = nullptr;
    SensorProxy *m_sensor = nullptr;
    bool m_configReady = false;
    bool m_reloadPending = false;
    bool m_autoRotateAvailable = false;
    bool m_autoRotateEnabled = false;
    bool m_sensorEnabled = false;
    bool m_rotating = false;
    QString m_screenRotation;
    QString m_panelMode;
    int m_enabledOutputs = 0;
};
