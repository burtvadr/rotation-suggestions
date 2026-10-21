// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sensorproxy.h"

#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>

namespace
{
constexpr auto sensorService = "net.hadess.SensorProxy";
constexpr auto sensorPath = "/net/hadess/SensorProxy";
constexpr auto sensorInterface = "net.hadess.SensorProxy";
}

SensorProxy::SensorProxy(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::systemBus())
{
    if (!m_bus.isConnected()) {
        qWarning() << "rotation-suggestions: system bus unavailable";
        return;
    }

    m_bus.connect(QString::fromLatin1(sensorService),
                  QString::fromLatin1(sensorPath),
                  QStringLiteral("org.freedesktop.DBus.Properties"),
                  QStringLiteral("PropertiesChanged"),
                  this,
                  SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    fetchProperties();
}

SensorProxy::~SensorProxy()
{
    if (m_claimed) {
        m_claimed = false;
        QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                              QString::fromLatin1(sensorPath),
                                                              QString::fromLatin1(sensorInterface),
                                                              QStringLiteral("ReleaseAccelerometer"));
        m_bus.asyncCall(message);
    }
}

bool SensorProxy::available() const
{
    return m_available;
}

QString SensorProxy::orientation() const
{
    return m_orientation;
}

bool SensorProxy::claimed() const
{
    return m_claimed;
}

void SensorProxy::setClaimed(bool claimed)
{
    if (m_claimed == claimed) {
        return;
    }
    if (claimed && !m_available) {
        return;
    }

    m_claimed = claimed;
    if (claimed) {
        claim();
    } else {
        release();
    }
    Q_EMIT claimedChanged();
}

void SensorProxy::onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    if (interface != QLatin1String(sensorInterface)) {
        return;
    }

    if (changed.contains(QStringLiteral("HasAccelerometer"))) {
        const bool available = unwrap(changed.value(QStringLiteral("HasAccelerometer"))).toBool();
        if (available != m_available) {
            m_available = available;
            Q_EMIT availableChanged();
            if (!available && m_claimed) {
                setClaimed(false);
            }
        }
    }

    if (changed.contains(QStringLiteral("AccelerometerOrientation"))) {
        const QString value = unwrap(changed.value(QStringLiteral("AccelerometerOrientation"))).toString();
        if (value != m_orientation) {
            m_orientation = value;
            Q_EMIT orientationChanged();
        }
    }
}

void SensorProxy::fetchProperties()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("GetAll"));
    message << QString::fromLatin1(sensorInterface);

    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<QVariantMap> reply = *call;
        if (reply.isError()) {
            qWarning() << "rotation-suggestions: failed to read sensor properties:" << reply.error().message();
            if (m_available) {
                m_available = false;
                Q_EMIT availableChanged();
            }
            return;
        }

        const QVariantMap properties = reply.value();
        const bool available = unwrap(properties.value(QStringLiteral("HasAccelerometer"))).toBool();
        const QString orientation = unwrap(properties.value(QStringLiteral("AccelerometerOrientation"))).toString();

        if (available != m_available) {
            m_available = available;
            Q_EMIT availableChanged();
        }
        if (orientation != m_orientation) {
            m_orientation = orientation;
            Q_EMIT orientationChanged();
        }
    });
}

void SensorProxy::claim()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QString::fromLatin1(sensorInterface),
                                                          QStringLiteral("ClaimAccelerometer"));

    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<> reply = *call;
        if (reply.isError()) {
            if (m_claimed) {
                m_claimed = false;
                Q_EMIT claimedChanged();
            }
            qWarning() << "rotation-suggestions: failed to claim accelerometer:" << reply.error().message();
            Q_EMIT errorOccurred(reply.error().message());
        }
    });
}

void SensorProxy::release()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QString::fromLatin1(sensorInterface),
                                                          QStringLiteral("ReleaseAccelerometer"));
    m_bus.asyncCall(message);
}

QVariant SensorProxy::unwrap(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}
