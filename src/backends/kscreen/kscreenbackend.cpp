// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kscreenbackend.h"

#include "rotationmapping.h"
#include "sensorproxy.h"

#include <kscreen/config.h>
#include <kscreen/configmonitor.h>
#include <kscreen/getconfigoperation.h>
#include <kscreen/mode.h>
#include <kscreen/setconfigoperation.h>

#include <QDebug>
#include <QSize>

KScreenBackend::KScreenBackend(QObject *parent)
    : RotationBackend(parent)
    , m_sensor(new SensorProxy(this))
{
    connect(m_sensor, &SensorProxy::availableChanged, this, [this] {
        Q_EMIT sensorAvailableChanged();
        updateState();
    });
    connect(m_sensor, &SensorProxy::orientationChanged, this, [this] {
        Q_EMIT orientationChanged();
        Q_EMIT suggestionChanged();
    });
    connect(m_sensor, &SensorProxy::claimedChanged, this, &KScreenBackend::suggestionChanged);
    connect(m_sensor, &SensorProxy::errorOccurred, this, &KScreenBackend::errorOccurred);

    connect(KScreen::ConfigMonitor::instance(),
            &KScreen::ConfigMonitor::configurationChanged,
            this,
            &KScreenBackend::reloadConfig);
    reloadConfig();
}

KScreenBackend::~KScreenBackend()
{
    if (m_config) {
        KScreen::ConfigMonitor::instance()->removeConfig(m_config);
    }
}

bool KScreenBackend::autoRotateAvailable() const
{
    return m_autoRotateAvailable;
}

bool KScreenBackend::autoRotateEnabled() const
{
    return m_autoRotateEnabled;
}

void KScreenBackend::setAutoRotateEnabled(bool enabled)
{
    if (!m_autoRotateAvailable) {
        Q_EMIT errorOccurred(tr("Automatic rotation is not available."));
        return;
    }
    if (m_autoRotateEnabled == enabled || m_apply || !m_config) {
        return;
    }

    const KScreen::OutputPtr output = primaryOutput();
    if (output.isNull()) {
        return;
    }

    const auto policy = enabled
        ? (m_config->tabletModeAvailable() ? KScreen::Output::AutoRotatePolicy::InTabletMode
                                           : KScreen::Output::AutoRotatePolicy::Always)
        : KScreen::Output::AutoRotatePolicy::Never;
    output->setAutoRotatePolicy(policy);

    // Update the UI immediately; the config reload confirms it.
    m_autoRotateEnabled = enabled;
    Q_EMIT autoRotateEnabledChanged();
    updateSensorClaim();
    Q_EMIT suggestionChanged();
    applyConfig();
}

bool KScreenBackend::sensorAvailable() const
{
    return m_sensor->available();
}

bool KScreenBackend::sensorEnabled() const
{
    return m_sensorEnabled;
}

void KScreenBackend::setSensorEnabled(bool enabled)
{
    if (m_sensorEnabled == enabled) {
        return;
    }
    m_sensorEnabled = enabled;
    Q_EMIT sensorEnabledChanged();
    updateSensorClaim();
    Q_EMIT suggestionChanged();
}

QString KScreenBackend::orientation() const
{
    return m_sensor->orientation();
}

QString KScreenBackend::screenRotation() const
{
    return m_screenRotation;
}

bool KScreenBackend::suggestionPossible() const
{
    return m_configReady && m_enabledOutputs == 1 && !m_screenRotation.isEmpty()
        && !m_panelMode.isEmpty() && m_sensorEnabled && m_sensor->available()
        && m_sensor->claimed() && !m_autoRotateEnabled;
}

QString KScreenBackend::targetRotationFor(const QString &orientation) const
{
    return RotationMapping::targetRotation(m_panelMode, orientation);
}

bool KScreenBackend::rotating() const
{
    return m_rotating;
}

void KScreenBackend::rotateToOrientation(const QString &orientation)
{
    if (m_rotating || m_apply || !m_configReady) {
        return;
    }

    const KScreen::OutputPtr output = primaryOutput();
    if (output.isNull()) {
        return;
    }

    const auto rotation = rotationForTarget(targetRotationFor(orientation));
    if (!rotation.has_value() || output->rotation() == *rotation) {
        return;
    }

    output->setRotation(*rotation);
    m_rotating = true;
    Q_EMIT rotatingChanged();
    applyConfig();
}

void KScreenBackend::reloadConfig()
{
    if (m_reload) {
        m_reloadPending = true;
        return;
    }

    m_reload = new KScreen::GetConfigOperation(KScreen::ConfigOperation::NoOptions, this);
    connect(m_reload, &KScreen::ConfigOperation::finished, this, [this](KScreen::ConfigOperation *operation) {
        m_reload = nullptr;

        auto *get = qobject_cast<KScreen::GetConfigOperation *>(operation);
        if (!get || get->hasError()) {
            qWarning() << "rotation-suggestions: failed to read KScreen config:"
                       << (get ? get->errorString() : QString());
            if (m_configReady) {
                m_configReady = false;
                updateState();
            }
        } else {
            setConfig(get->config());
        }

        operation->deleteLater();
        if (m_reloadPending) {
            m_reloadPending = false;
            reloadConfig();
        }
    });
}

void KScreenBackend::setConfig(const KScreen::ConfigPtr &config)
{
    if (m_config) {
        KScreen::ConfigMonitor::instance()->removeConfig(m_config);
    }

    m_config = config;
    m_configReady = !m_config.isNull();
    if (m_config) {
        KScreen::ConfigMonitor::instance()->addConfig(m_config);
    }
    updateState();
}

void KScreenBackend::updateState()
{
    const bool oldAutoRotateAvailable = m_autoRotateAvailable;
    const bool oldAutoRotateEnabled = m_autoRotateEnabled;
    const QString oldScreenRotation = m_screenRotation;

    const KScreen::OutputPtr output = primaryOutput();
    m_autoRotateAvailable = !m_config.isNull()
        && m_config->supportedFeatures().testFlag(KScreen::Config::Feature::AutoRotation)
        && !output.isNull() && m_sensor->available();
    m_autoRotateEnabled = !output.isNull()
        && output->autoRotatePolicy() != KScreen::Output::AutoRotatePolicy::Never;
    m_screenRotation = output.isNull() ? QString() : rotationToString(output->rotation());
    m_panelMode = output.isNull() ? QString() : nativeMode(output);

    m_enabledOutputs = 0;
    if (m_config) {
        const auto outputs = m_config->connectedOutputs();
        for (const auto &candidate : outputs) {
            if (candidate->isEnabled()) {
                ++m_enabledOutputs;
            }
        }
    }

    if (oldAutoRotateAvailable != m_autoRotateAvailable) {
        Q_EMIT autoRotateAvailableChanged();
    }
    if (oldAutoRotateEnabled != m_autoRotateEnabled) {
        Q_EMIT autoRotateEnabledChanged();
    }
    if (oldScreenRotation != m_screenRotation) {
        Q_EMIT screenRotationChanged();
    }

    updateSensorClaim();
    Q_EMIT suggestionChanged();
}

void KScreenBackend::updateSensorClaim()
{
    m_sensor->setClaimed(m_sensorEnabled && m_configReady && !m_autoRotateEnabled);
}

void KScreenBackend::applyConfig()
{
    if (!m_config || m_apply) {
        return;
    }

    m_apply = new KScreen::SetConfigOperation(m_config, this);
    connect(m_apply, &KScreen::ConfigOperation::finished, this, [this](KScreen::ConfigOperation *operation) {
        m_apply = nullptr;

        if (m_rotating) {
            m_rotating = false;
            Q_EMIT rotatingChanged();
        }
        if (operation->hasError()) {
            Q_EMIT errorOccurred(operation->errorString());
        }

        operation->deleteLater();
        reloadConfig();
    });
}

KScreen::OutputPtr KScreenBackend::primaryOutput() const
{
    if (!m_config) {
        return {};
    }

    const KScreen::OutputPtr primary = m_config->primaryOutput();
    if (!primary.isNull() && primary->isConnected() && primary->isEnabled()) {
        return primary;
    }

    const auto outputs = m_config->connectedOutputs();
    for (const auto &output : outputs) {
        if (output->isEnabled()) {
            return output;
        }
    }
    return {};
}

QString KScreenBackend::nativeMode(const KScreen::OutputPtr &output) const
{
    if (output.isNull()) {
        return {};
    }

    KScreen::ModePtr mode = output->currentMode();
    if (mode.isNull()) {
        mode = output->preferredMode();
    }
    if (mode.isNull()) {
        return {};
    }

    const QSize size = mode->size();
    if (!size.isValid()) {
        return {};
    }
    return QStringLiteral("%1x%2").arg(size.width()).arg(size.height());
}

QString KScreenBackend::rotationToString(KScreen::Output::Rotation rotation) const
{
    switch (rotation) {
    case KScreen::Output::None:
        return QStringLiteral("normal");
    case KScreen::Output::Left:
        return QStringLiteral("left");
    case KScreen::Output::Inverted:
        return QStringLiteral("inverted");
    case KScreen::Output::Right:
        return QStringLiteral("right");
    default:
        return {};
    }
}

std::optional<KScreen::Output::Rotation> KScreenBackend::rotationForTarget(const QString &target) const
{
    if (target == QLatin1String("normal")) {
        return KScreen::Output::None;
    }
    if (target == QLatin1String("left")) {
        return KScreen::Output::Left;
    }
    if (target == QLatin1String("right")) {
        return KScreen::Output::Right;
    }
    if (target == QLatin1String("inverted")) {
        return KScreen::Output::Inverted;
    }
    return std::nullopt;
}
