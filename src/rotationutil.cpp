// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rotationutil.h"

#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

namespace
{
constexpr auto sensorService = "net.hadess.SensorProxy";
constexpr auto sensorPath = "/net/hadess/SensorProxy";
constexpr auto sensorInterface = "net.hadess.SensorProxy";
constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdManager = "org.freedesktop.systemd1.Manager";
constexpr auto unitName = "auto-rotate.service";
}

RotationUtil::RotationUtil(QObject *parent)
    : QObject(parent)
    , m_sessionBus(QDBusConnection::sessionBus())
    , m_systemBus(QDBusConnection::systemBus())
{
    m_rotationAvailable = QFileInfo::exists(QStringLiteral("/usr/bin/auto-rotate"));

    if (m_sessionBus.isConnected()) {
        loadUnit();
    } else {
        m_unitState = QStringLiteral("unavailable");
        m_unitStateKnown = true;
        Q_EMIT autoRotateAvailableChanged();
    }

    if (m_systemBus.isConnected()) {
        m_systemBus.connect(QString::fromLatin1(sensorService),
                            QString::fromLatin1(sensorPath),
                            QStringLiteral("org.freedesktop.DBus.Properties"),
                            QStringLiteral("PropertiesChanged"),
                            this,
                            SLOT(onSensorPropertiesChanged(QString, QVariantMap, QStringList)));
        fetchSensorProperties();
    } else {
        qWarning() << "rotation-suggestions: system bus unavailable";
    }

    refreshScreenRotation();
    updateSuggestion();
}

RotationUtil::~RotationUtil()
{
    if (m_sensorClaimed) {
        m_sensorClaimed = false;
        QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                              QString::fromLatin1(sensorPath),
                                                              QString::fromLatin1(sensorInterface),
                                                              QStringLiteral("ReleaseAccelerometer"));
        m_systemBus.asyncCall(message);
    }

    if (m_xrandr && m_xrandr->state() != QProcess::NotRunning) {
        m_xrandr->kill();
    }
    if (m_rotate && m_rotate->state() != QProcess::NotRunning) {
        m_rotate->kill();
    }
}

bool RotationUtil::autoRotateAvailable() const
{
    return m_autoRotateAvailable;
}

bool RotationUtil::autoRotateEnabled() const
{
    return m_autoRotateEnabled;
}

void RotationUtil::setAutoRotateEnabled(bool enabled)
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
    if (enabled) {
        releaseSensor();
    }
    updateSuggestion();
    setUnitActive(enabled);
}

bool RotationUtil::sensorAvailable() const
{
    return m_sensorAvailable;
}

bool RotationUtil::sensorEnabled() const
{
    return m_sensorEnabled;
}

void RotationUtil::setSensorEnabled(bool enabled)
{
    if (m_sensorEnabled == enabled) {
        return;
    }
    m_sensorEnabled = enabled;
    Q_EMIT sensorEnabledChanged();
    if (enabled) {
        claimSensor();
    } else {
        releaseSensor();
    }
    updateSuggestion();
}

QString RotationUtil::orientation() const
{
    return m_orientation;
}

QString RotationUtil::screenRotation() const
{
    return m_screenRotation;
}

QString RotationUtil::suggestedOrientation() const
{
    return targetRotationFor(m_orientation).isEmpty() ? QString() : m_orientation;
}

QString RotationUtil::suggestedRotation() const
{
    return targetRotationFor(m_orientation);
}

QString RotationUtil::suggestedDirection() const
{
    return directionFor(suggestedRotation(), m_screenRotation);
}

bool RotationUtil::suggestionVisible() const
{
    if (!m_rotationAvailable || !m_sensorEnabled || !m_sensorAvailable || !m_sensorClaimed) {
        return false;
    }
    if (!m_unitStateKnown || !serviceReadyForSensor() || m_autoRotateEnabled) {
        return false;
    }
    if (m_connectedOutputs != 1 || m_screenRotation.isEmpty()) {
        return false;
    }
    if (m_orientation.isEmpty() || m_orientation == QLatin1String("undefined")) {
        return false;
    }
    if (m_dismissedOrientation == m_orientation) {
        return false;
    }

    const QString target = targetRotationFor(m_orientation);
    return !target.isEmpty() && target != m_screenRotation;
}

bool RotationUtil::rotating() const
{
    return m_rotating;
}

void RotationUtil::rotateToSuggested()
{
    if (m_rotating) {
        return;
    }

    const QString orientation = suggestedOrientation();
    if (orientation.isEmpty()) {
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

void RotationUtil::dismiss()
{
    m_dismissedOrientation = m_orientation;
    Q_EMIT suggestionChanged();
}

void RotationUtil::onSensorPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    if (interface != QLatin1String(sensorInterface)) {
        return;
    }

    if (changed.contains(QStringLiteral("HasAccelerometer"))) {
        const bool available = unwrap(changed.value(QStringLiteral("HasAccelerometer"))).toBool();
        if (available != m_sensorAvailable) {
            m_sensorAvailable = available;
            Q_EMIT sensorAvailableChanged();
            if (available) {
                claimSensor();
            } else {
                releaseSensor();
            }
        }
    }

    if (changed.contains(QStringLiteral("AccelerometerOrientation"))) {
        const QString value =
            unwrap(changed.value(QStringLiteral("AccelerometerOrientation"))).toString();
        if (value != m_orientation) {
            m_orientation = value;
            m_dismissedOrientation.clear();
            Q_EMIT orientationChanged();
            updateSuggestion();
        }
    }
}

void RotationUtil::onUnitPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    if (interface != QLatin1String("org.freedesktop.systemd1.Unit")) {
        return;
    }
    if (!changed.contains(QStringLiteral("ActiveState"))) {
        return;
    }
    applyUnitState(unwrap(changed.value(QStringLiteral("ActiveState"))).toString());
}

void RotationUtil::loadUnit()
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
            claimSensor();
            updateSuggestion();
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

void RotationUtil::fetchUnitState()
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
            qWarning() << "rotation-suggestions: failed to read auto-rotate.service state:"
                       << reply.error().message();
            return;
        }
        applyUnitState(reply.value().variant().toString());
    });
}

void RotationUtil::setUnitActive(bool active)
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

void RotationUtil::applyUnitState(const QString &state)
{
    m_unitState = state;
    m_unitStateKnown = true;

    const bool enabled = state == QLatin1String("active") || state == QLatin1String("activating")
        || state == QLatin1String("reloading");

    if (enabled) {
        releaseSensor();
    } else if (serviceReadyForSensor()) {
        claimSensor();
    }

    if (enabled != m_autoRotateEnabled) {
        m_autoRotateEnabled = enabled;
        Q_EMIT autoRotateEnabledChanged();
    }
    updateSuggestion();
}

void RotationUtil::fetchSensorProperties()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("GetAll"));
    message << QString::fromLatin1(sensorInterface);

    auto *watcher = new QDBusPendingCallWatcher(m_systemBus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<QVariantMap> reply = *call;
        if (reply.isError()) {
            qWarning() << "rotation-suggestions: failed to read sensor properties:"
                       << reply.error().message();
            m_sensorAvailable = false;
            Q_EMIT sensorAvailableChanged();
            updateSuggestion();
            return;
        }

        const QVariantMap properties = reply.value();
        const bool available = unwrap(properties.value(QStringLiteral("HasAccelerometer"))).toBool();
        const QString orientation =
            unwrap(properties.value(QStringLiteral("AccelerometerOrientation"))).toString();

        if (available != m_sensorAvailable) {
            m_sensorAvailable = available;
            Q_EMIT sensorAvailableChanged();
        }
        if (orientation != m_orientation) {
            m_orientation = orientation;
            Q_EMIT orientationChanged();
        }

        if (available) {
            claimSensor();
        } else {
            releaseSensor();
        }
        updateSuggestion();
    });
}

void RotationUtil::claimSensor()
{
    if (!m_sensorEnabled || !m_sensorAvailable || m_sensorClaimed || !serviceReadyForSensor()) {
        return;
    }

    m_sensorClaimed = true;
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QString::fromLatin1(sensorInterface),
                                                          QStringLiteral("ClaimAccelerometer"));

    auto *watcher = new QDBusPendingCallWatcher(m_systemBus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        QDBusPendingReply<> reply = *call;
        if (reply.isError()) {
            m_sensorClaimed = false;
            qWarning() << "rotation-suggestions: failed to claim accelerometer:" << reply.error().message();
            Q_EMIT errorOccurred(reply.error().message());
            updateSuggestion();
        }
    });
}

void RotationUtil::releaseSensor()
{
    if (!m_sensorClaimed) {
        return;
    }

    m_sensorClaimed = false;
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(sensorService),
                                                          QString::fromLatin1(sensorPath),
                                                          QString::fromLatin1(sensorInterface),
                                                          QStringLiteral("ReleaseAccelerometer"));
    m_systemBus.asyncCall(message);
    updateSuggestion();
}

void RotationUtil::refreshScreenRotation()
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
                    updateSuggestion();
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
        updateSuggestion();
    });

    m_xrandr->start(QStringLiteral("xrandr"), {QStringLiteral("--query")});
}

void RotationUtil::parseXrandr(const QString &output)
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
    updateSuggestion();
}

void RotationUtil::updateSuggestion()
{
    Q_EMIT suggestionChanged();
}

QString RotationUtil::targetRotationFor(const QString &orientation) const
{
    if (m_panelMode.isEmpty()) {
        return {};
    }

    const QStringList dimensions = m_panelMode.split(QLatin1Char('x'));
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

QString RotationUtil::directionFor(const QString &target, const QString &current) const
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

bool RotationUtil::serviceReadyForSensor() const
{
    return m_unitState == QLatin1String("inactive") || m_unitState == QLatin1String("failed")
        || m_unitState == QLatin1String("unavailable");
}

QVariant RotationUtil::unwrap(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}
