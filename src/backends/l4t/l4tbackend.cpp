// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "l4tbackend.h"

#include "rotationmapping.h"
#include "sensorproxy.h"

#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

namespace
{
constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdManager = "org.freedesktop.systemd1.Manager";
constexpr auto unitName = "auto-rotate.service";
}

L4TBackend::L4TBackend(QObject *parent)
    : RotationBackend(parent)
    , m_sessionBus(QDBusConnection::sessionBus())
    , m_sensor(new SensorProxy(this))
{
    m_rotationAvailable = QFileInfo::exists(QStringLiteral("/usr/bin/auto-rotate"));

    connect(m_sensor, &SensorProxy::availableChanged, this, [this] {
        Q_EMIT sensorAvailableChanged();
        updateSensorClaim();
        Q_EMIT suggestionChanged();
    });
    connect(m_sensor, &SensorProxy::orientationChanged, this, [this] {
        Q_EMIT orientationChanged();
        Q_EMIT suggestionChanged();
    });
    connect(m_sensor, &SensorProxy::claimedChanged, this, &L4TBackend::suggestionChanged);
    connect(m_sensor, &SensorProxy::errorOccurred, this, &L4TBackend::errorOccurred);

    if (m_sessionBus.isConnected()) {
        loadUnit();
    } else {
        m_unitState = QStringLiteral("unavailable");
        m_unitStateKnown = true;
        Q_EMIT autoRotateAvailableChanged();
    }

    refreshScreenRotation();
}

L4TBackend::~L4TBackend()
{
    if (m_xrandr && m_xrandr->state() != QProcess::NotRunning) {
        m_xrandr->kill();
    }
    if (m_rotate && m_rotate->state() != QProcess::NotRunning) {
        m_rotate->kill();
    }
}

bool L4TBackend::autoRotateAvailable() const
{
    return m_autoRotateAvailable;
}

bool L4TBackend::autoRotateEnabled() const
{
    return m_autoRotateEnabled;
}

void L4TBackend::setAutoRotateEnabled(bool enabled)
{
    if (!m_autoRotateAvailable) {
        Q_EMIT errorOccurred(tr("The auto-rotate service is not available."));
        return;
    }
    if (m_autoRotateEnabled == enabled) {
        return;
    }

    // Update the UI immediately; systemd's PropertiesChanged will confirm it.
    m_autoRotateEnabled = enabled;
    Q_EMIT autoRotateEnabledChanged();
    updateSensorClaim();
    Q_EMIT suggestionChanged();
    setUnitActive(enabled);
}

bool L4TBackend::sensorAvailable() const
{
    return m_sensor->available();
}

bool L4TBackend::sensorEnabled() const
{
    return m_sensorEnabled;
}

void L4TBackend::setSensorEnabled(bool enabled)
{
    if (m_sensorEnabled == enabled) {
        return;
    }
    m_sensorEnabled = enabled;
    Q_EMIT sensorEnabledChanged();
    updateSensorClaim();
    Q_EMIT suggestionChanged();
}

QString L4TBackend::orientation() const
{
    return m_sensor->orientation();
}

QString L4TBackend::screenRotation() const
{
    return m_screenRotation;
}

bool L4TBackend::suggestionPossible() const
{
    return m_rotationAvailable && m_sensorEnabled && m_sensor->available() && m_sensor->claimed()
        && m_unitStateKnown && serviceReadyForSensor() && !m_autoRotateEnabled && m_connectedOutputs == 1
        && !m_screenRotation.isEmpty();
}

QString L4TBackend::targetRotationFor(const QString &orientation) const
{
    return RotationMapping::targetRotation(m_panelMode, orientation);
}

bool L4TBackend::rotating() const
{
    return m_rotating;
}

void L4TBackend::rotateToOrientation(const QString &orientation)
{
    if (m_rotating || orientation.isEmpty()) {
        return;
    }
    if (!m_rotationAvailable) {
        Q_EMIT errorOccurred(tr("The auto-rotate script is not available."));
        return;
    }

    m_rotating = true;
    Q_EMIT rotatingChanged();

    if (m_rotate) {
        m_rotate->deleteLater();
        m_rotate = nullptr;
    }

    m_rotate = new QProcess(this);
    connect(m_rotate,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus status) {
                QProcess *process = m_rotate;
                m_rotate = nullptr;
                if (process) {
                    process->deleteLater();
                }
                m_rotating = false;
                Q_EMIT rotatingChanged();

                if (status == QProcess::NormalExit && exitCode == 0) {
                    refreshScreenRotation();
                } else {
                    Q_EMIT errorOccurred(tr("Failed to rotate the screen."));
                }
            });
    connect(m_rotate, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }
        QProcess *process = m_rotate;
        m_rotate = nullptr;
        if (process) {
            process->deleteLater();
        }
        m_rotating = false;
        Q_EMIT rotatingChanged();
        Q_EMIT errorOccurred(tr("Could not start the auto-rotate script."));
    });

    m_rotate->start(QStringLiteral("/usr/bin/auto-rotate"), {QStringLiteral("--once"), orientation});
}

void L4TBackend::onUnitPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    if (interface != QLatin1String("org.freedesktop.systemd1.Unit")) {
        return;
    }
    if (!changed.contains(QStringLiteral("ActiveState"))) {
        return;
    }

    const QVariant value = changed.value(QStringLiteral("ActiveState"));
    const QVariant unwrapped = value.canConvert<QDBusVariant>() ? value.value<QDBusVariant>().variant() : value;
    applyUnitState(unwrapped.toString());
}

void L4TBackend::loadUnit()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(systemdService),
                                                          QString::fromLatin1(systemdPath),
                                                          QString::fromLatin1(systemdManager),
                                                          QStringLiteral("LoadUnit"));
    message << QString::fromLatin1(unitName);

    auto *watcher = new QDBusPendingCallWatcher(m_sessionBus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (reply.isError()) {
            m_unitState = QStringLiteral("unavailable");
            m_unitStateKnown = true;
            m_autoRotateAvailable = false;
            Q_EMIT autoRotateAvailableChanged();
            updateSensorClaim();
            Q_EMIT suggestionChanged();
            return;
        }

        m_unitPath = reply.value().path();
        m_autoRotateAvailable = true;
        Q_EMIT autoRotateAvailableChanged();

        m_sessionBus.connect(QString::fromLatin1(systemdService),
                             m_unitPath,
                             QStringLiteral("org.freedesktop.DBus.Properties"),
                             QStringLiteral("PropertiesChanged"),
                             this,
                             SLOT(onUnitPropertiesChanged(QString, QVariantMap, QStringList)));

        fetchUnitState();
    });
}

void L4TBackend::fetchUnitState()
{
    if (m_unitPath.isEmpty()) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(systemdService),
                                                          m_unitPath,
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("Get"));
    message << QStringLiteral("org.freedesktop.systemd1.Unit") << QStringLiteral("ActiveState");

    auto *watcher = new QDBusPendingCallWatcher(m_sessionBus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<QDBusVariant> reply = *call;
        if (reply.isError()) {
            qWarning() << "rotation-suggestions: failed to read auto-rotate.service state:" << reply.error().message();
            return;
        }
        applyUnitState(reply.value().variant().toString());
    });
}

void L4TBackend::setUnitActive(bool active)
{
    QDBusMessage message =
        QDBusMessage::createMethodCall(QString::fromLatin1(systemdService),
                                       QString::fromLatin1(systemdPath),
                                       QString::fromLatin1(systemdManager),
                                       active ? QStringLiteral("StartUnit") : QStringLiteral("StopUnit"));
    message << QString::fromLatin1(unitName) << QStringLiteral("replace");

    auto *watcher = new QDBusPendingCallWatcher(m_sessionBus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, active](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (reply.isError()) {
            qWarning() << "rotation-suggestions: failed to" << (active ? "start" : "stop")
                       << "auto-rotate.service:" << reply.error().message();
            Q_EMIT errorOccurred(reply.error().message());
            fetchUnitState();
        }
    });
}

void L4TBackend::applyUnitState(const QString &state)
{
    m_unitState = state;
    m_unitStateKnown = true;

    const bool enabled = state == QLatin1String("active") || state == QLatin1String("activating")
        || state == QLatin1String("reloading");

    if (enabled != m_autoRotateEnabled) {
        m_autoRotateEnabled = enabled;
        Q_EMIT autoRotateEnabledChanged();
    }

    updateSensorClaim();
    Q_EMIT suggestionChanged();
}

void L4TBackend::updateSensorClaim()
{
    m_sensor->setClaimed(m_sensorEnabled && !m_autoRotateEnabled && m_unitStateKnown
                         && serviceReadyForSensor());
}

void L4TBackend::refreshScreenRotation()
{
    if (m_xrandr) {
        if (m_xrandr->state() != QProcess::NotRunning) {
            return;
        }
        m_xrandr->deleteLater();
        m_xrandr = nullptr;
    }

    m_xrandr = new QProcess(this);
    connect(m_xrandr,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus status) {
                QProcess *process = m_xrandr;
                m_xrandr = nullptr;
                if (!process) {
                    return;
                }

                const QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
                if (status == QProcess::NormalExit && exitCode == 0) {
                    parseXrandr(output);
                } else {
                    qWarning() << "rotation-suggestions: xrandr failed:" << process->readAllStandardError();
                    m_screenRotation.clear();
                    m_panelMode.clear();
                    m_connectedOutputs = 0;
                    Q_EMIT screenRotationChanged();
                    Q_EMIT suggestionChanged();
                }
                process->deleteLater();
            });
    connect(m_xrandr, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }
        QProcess *process = m_xrandr;
        m_xrandr = nullptr;
        if (process) {
            process->deleteLater();
        }
        m_screenRotation.clear();
        m_panelMode.clear();
        m_connectedOutputs = 0;
        Q_EMIT screenRotationChanged();
        Q_EMIT suggestionChanged();
    });

    m_xrandr->start(QStringLiteral("xrandr"), {QStringLiteral("--query")});
}

void L4TBackend::parseXrandr(const QString &output)
{
    const QStringList lines = output.split(QLatin1Char('\n'));
    int connected = 0;
    QString panelLine;
    QStringList panelModes;
    bool inPanel = false;

    for (const QString &line : lines) {
        if (line.isEmpty()) {
            continue;
        }

        if (!line.at(0).isSpace()) {
            const QStringList fields =
                line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            const bool isConnected =
                fields.size() >= 2 && fields.at(1) == QLatin1String("connected");
            if (isConnected) {
                ++connected;
            }
            inPanel = isConnected && fields.at(0).startsWith(QLatin1String("DSI-"));
            if (inPanel) {
                panelLine = line;
            }
            continue;
        }

        if (!inPanel) {
            continue;
        }

        const QStringList fields = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (fields.isEmpty()) {
            continue;
        }
        const QString mode = fields.first();
        if (mode.contains(QLatin1Char('x')) && mode.at(0).isDigit()) {
            panelModes.append(line.trimmed());
        }
    }

    QString panelMode;
    for (const QString &line : panelModes) {
        if (line.contains(QLatin1Char('+'))) {
            panelMode = line.section(QLatin1Char(' '), 0, 0);
            break;
        }
    }
    if (panelMode.isEmpty() && !panelModes.isEmpty()) {
        panelMode = panelModes.first().section(QLatin1Char(' '), 0, 0);
    }

    QString rotation;
    if (!panelLine.isEmpty()) {
        static const QRegularExpression rotationExpression(QStringLiteral(
            "^\\S+\\s+connected(?:\\s+primary)?\\s+\\d+x\\d+[+-]\\d+[+-]\\d+(?:\\s+(\\w+))?\\s*\\("));
        const QRegularExpressionMatch match = rotationExpression.match(panelLine);
        if (match.hasMatch()) {
            rotation = match.captured(1);
            if (rotation.isEmpty()) {
                rotation = QStringLiteral("normal");
            }
        }
    }

    m_connectedOutputs = connected;
    m_panelMode = panelMode;
    if (rotation != m_screenRotation) {
        m_screenRotation = rotation;
        Q_EMIT screenRotationChanged();
    }
    Q_EMIT suggestionChanged();
}

bool L4TBackend::serviceReadyForSensor() const
{
    return m_unitState == QLatin1String("inactive") || m_unitState == QLatin1String("failed")
        || m_unitState == QLatin1String("unavailable");
}
