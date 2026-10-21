// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QString>

#include <memory>

class RotationBackend;

// QML-facing facade. The applets use this type for both backends; the
// backend is selected at build time and owns all platform-specific work.
class RotationUtil : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool autoRotateAvailable READ autoRotateAvailable NOTIFY autoRotateAvailableChanged)
    Q_PROPERTY(bool autoRotateEnabled READ autoRotateEnabled WRITE setAutoRotateEnabled NOTIFY autoRotateEnabledChanged)
    Q_PROPERTY(bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
    Q_PROPERTY(bool sensorEnabled READ sensorEnabled WRITE setSensorEnabled NOTIFY sensorEnabledChanged)
    Q_PROPERTY(QString orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(QString screenRotation READ screenRotation NOTIFY screenRotationChanged)
    Q_PROPERTY(QString suggestedOrientation READ suggestedOrientation NOTIFY suggestedOrientationChanged)
    Q_PROPERTY(QString suggestedRotation READ suggestedRotation NOTIFY suggestionChanged)
    Q_PROPERTY(QString suggestedDirection READ suggestedDirection NOTIFY suggestionChanged)
    Q_PROPERTY(bool suggestionVisible READ suggestionVisible NOTIFY suggestionVisibleChanged)
    Q_PROPERTY(bool rotating READ rotating NOTIFY rotatingChanged)

public:
    explicit RotationUtil(QObject *parent = nullptr);
    ~RotationUtil() override;

    bool autoRotateAvailable() const;
    bool autoRotateEnabled() const;
    void setAutoRotateEnabled(bool enabled);
    bool sensorAvailable() const;
    bool sensorEnabled() const;
    void setSensorEnabled(bool enabled);
    QString orientation() const;
    QString screenRotation() const;
    QString suggestedOrientation() const;
    QString suggestedRotation() const;
    QString suggestedDirection() const;
    bool suggestionVisible() const;
    bool rotating() const;

    Q_INVOKABLE void rotateToSuggested();
    Q_INVOKABLE void dismiss();

Q_SIGNALS:
    void autoRotateAvailableChanged();
    void autoRotateEnabledChanged();
    void sensorAvailableChanged();
    void sensorEnabledChanged();
    void orientationChanged();
    void screenRotationChanged();
    void suggestedOrientationChanged();
    void suggestionChanged();
    void suggestionVisibleChanged();
    void rotatingChanged();
    void errorOccurred(const QString &message);

private:
    void refreshSuggestion();

    std::unique_ptr<RotationBackend> m_backend;
    QString m_dismissedOrientation;
    QString m_lastSuggestedOrientation;
    bool m_lastSuggestionVisible = false;
};
