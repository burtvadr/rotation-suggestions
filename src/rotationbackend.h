// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QString>

// The platform-specific half of the applets. RotationUtil owns one of these
// and exposes the same QML interface regardless of the backend.
class RotationBackend : public QObject
{
    Q_OBJECT

public:
    explicit RotationBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
    ~RotationBackend() override = default;

    virtual bool autoRotateAvailable() const = 0;
    virtual bool autoRotateEnabled() const = 0;
    virtual void setAutoRotateEnabled(bool enabled) = 0;

    virtual bool sensorAvailable() const = 0;
    virtual bool sensorEnabled() const = 0;
    virtual void setSensorEnabled(bool enabled) = 0;

    virtual QString orientation() const = 0;
    virtual QString screenRotation() const = 0;

    // True when the backend has everything it needs to show a suggestion.
    // RotationUtil adds the orientation, dismissal and already-rotated checks.
    virtual bool suggestionPossible() const = 0;
    virtual QString targetRotationFor(const QString &orientation) const = 0;

    virtual bool rotating() const = 0;
    virtual void rotateToOrientation(const QString &orientation) = 0;

Q_SIGNALS:
    void autoRotateAvailableChanged();
    void autoRotateEnabledChanged();
    void sensorAvailableChanged();
    void sensorEnabledChanged();
    void orientationChanged();
    void screenRotationChanged();
    void suggestionChanged();
    void rotatingChanged();
    void errorOccurred(const QString &message);
};
