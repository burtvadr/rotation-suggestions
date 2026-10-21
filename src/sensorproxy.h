// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

// Shared access to the accelerometer exposed by iio-sensor-proxy. Both
// backends use this; they decide when the sensor should be claimed.
class SensorProxy : public QObject
{
    Q_OBJECT

public:
    explicit SensorProxy(QObject *parent = nullptr);
    ~SensorProxy() override;

    bool available() const;
    QString orientation() const;
    bool claimed() const;

    void setClaimed(bool claimed);

Q_SIGNALS:
    void availableChanged();
    void orientationChanged();
    void claimedChanged();
    void errorOccurred(const QString &message);

private Q_SLOTS:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void fetchProperties();
    void claim();
    void release();
    static QVariant unwrap(const QVariant &value);

    QDBusConnection m_bus;
    bool m_available = false;
    bool m_claimed = false;
    QString m_orientation = QStringLiteral("undefined");
};
