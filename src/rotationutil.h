// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class RotationUtil : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool autoRotateAvailable READ autoRotateAvailable NOTIFY autoRotateAvailableChanged)
    Q_PROPERTY(bool autoRotateEnabled READ autoRotateEnabled WRITE setAutoRotateEnabled NOTIFY autoRotateEnabledChanged)
    Q_PROPERTY(bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
    Q_PROPERTY(bool sensorEnabled READ sensorEnabled WRITE setSensorEnabled NOTIFY sensorEnabledChanged)
    Q_PROPERTY(QString orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(QString screenRotation READ screenRotation NOTIFY screenRotationChanged)
    Q_PROPERTY(QString suggestedOrientation READ suggestedOrientation NOTIFY suggestionChanged)
    Q_PROPERTY(QString suggestedRotation READ suggestedRotation NOTIFY suggestionChanged)
    Q_PROPERTY(QString suggestedDirection READ suggestedDirection NOTIFY suggestionChanged)
    Q_PROPERTY(bool suggestionVisible READ suggestionVisible NOTIFY suggestionChanged)
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
    void suggestionChanged();
    void rotatingChanged();
    void errorOccurred(const QString &message);

private Q_SLOTS:
    void onSensorPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);
    void onUnitPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void loadUnit();
    void fetchUnitState();
    void setUnitActive(bool active);
    void applyUnitState(const QString &state);
    void fetchSensorProperties();
    void claimSensor();
    void releaseSensor();
    void refreshScreenRotation();
    void parseXrandr(const QString &output);
    void updateSuggestion();
    QString targetRotationFor(const QString &orientation) const;
    QString directionFor(const QString &target, const QString &current) const;
    bool serviceReadyForSensor() const;
    static QVariant unwrap(const QVariant &value);

    QDBusConnection m_sessionBus;
    QDBusConnection m_systemBus;
    QString m_unitPath;
    QString m_unitState;
    bool m_unitStateKnown = false;
    bool m_autoRotateAvailable = false;
    bool m_autoRotateEnabled = false;
    bool m_sensorAvailable = false;
    bool m_sensorEnabled = false;
    bool m_sensorClaimed = false;
    bool m_rotating = false;
    bool m_rotationAvailable = false;
    QString m_orientation = QStringLiteral("undefined");
    QString m_screenRotation;
    QString m_panelMode;
    int m_connectedOutputs = 0;
    QString m_dismissedOrientation;
    QProcess *m_xrandr = nullptr;
    QProcess *m_rotate = nullptr;
};
