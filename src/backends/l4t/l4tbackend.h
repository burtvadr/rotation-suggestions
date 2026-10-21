// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "rotationbackend.h"

#include <QDBusConnection>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QProcess;
class SensorProxy;

// L4T Nintendo Switch backend: drives the BSP's auto-rotate user service and
// X11 script. The script owns the xrandr call and the touch transformation.
class L4TBackend : public RotationBackend
{
    Q_OBJECT

public:
    explicit L4TBackend(QObject *parent = nullptr);
    ~L4TBackend() override;

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

private Q_SLOTS:
    void onUnitPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void loadUnit();
    void fetchUnitState();
    void setUnitActive(bool active);
    void applyUnitState(const QString &state);
    void updateSensorClaim();
    void refreshScreenRotation();
    void parseXrandr(const QString &output);
    bool serviceReadyForSensor() const;

    QDBusConnection m_sessionBus;
    QString m_unitPath;
    QString m_unitState;
    bool m_unitStateKnown = false;
    bool m_autoRotateAvailable = false;
    bool m_autoRotateEnabled = false;
    bool m_sensorEnabled = false;
    bool m_rotating = false;
    bool m_rotationAvailable = false;
    QString m_screenRotation;
    QString m_panelMode;
    int m_connectedOutputs = 0;
    QProcess *m_xrandr = nullptr;
    QProcess *m_rotate = nullptr;
    SensorProxy *m_sensor = nullptr;
};
