// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rotationutil.h"

#include "rotationbackend.h"
#include "rotationmapping.h"

#if defined(ROTATION_BACKEND_L4T)
#include "backends/l4t/l4tbackend.h"
#elif defined(ROTATION_BACKEND_KSCREEN)
#include "backends/kscreen/kscreenbackend.h"
#else
#error "No rotation backend selected"
#endif

RotationUtil::RotationUtil(QObject *parent)
    : QObject(parent)
{
#if defined(ROTATION_BACKEND_L4T)
    m_backend = std::make_unique<L4TBackend>();
#elif defined(ROTATION_BACKEND_KSCREEN)
    m_backend = std::make_unique<KScreenBackend>();
#endif

    connect(m_backend.get(),
            &RotationBackend::autoRotateAvailableChanged,
            this,
            &RotationUtil::autoRotateAvailableChanged);
    connect(m_backend.get(),
            &RotationBackend::autoRotateEnabledChanged,
            this,
            &RotationUtil::autoRotateEnabledChanged);
    connect(m_backend.get(),
            &RotationBackend::sensorAvailableChanged,
            this,
            &RotationUtil::sensorAvailableChanged);
    connect(m_backend.get(),
            &RotationBackend::sensorEnabledChanged,
            this,
            &RotationUtil::sensorEnabledChanged);
    connect(m_backend.get(), &RotationBackend::orientationChanged, this, [this] {
        m_dismissedOrientation.clear();
        Q_EMIT orientationChanged();
    });
    connect(m_backend.get(),
            &RotationBackend::screenRotationChanged,
            this,
            &RotationUtil::screenRotationChanged);
    connect(m_backend.get(), &RotationBackend::suggestionChanged, this, &RotationUtil::refreshSuggestion);
    connect(m_backend.get(), &RotationBackend::rotatingChanged, this, &RotationUtil::rotatingChanged);
    connect(m_backend.get(), &RotationBackend::errorOccurred, this, &RotationUtil::errorOccurred);

    m_lastSuggestedOrientation = suggestedOrientation();
    m_lastSuggestionVisible = suggestionVisible();
}

RotationUtil::~RotationUtil() = default;

bool RotationUtil::autoRotateAvailable() const
{
    return m_backend->autoRotateAvailable();
}

bool RotationUtil::autoRotateEnabled() const
{
    return m_backend->autoRotateEnabled();
}

void RotationUtil::setAutoRotateEnabled(bool enabled)
{
    m_backend->setAutoRotateEnabled(enabled);
}

bool RotationUtil::sensorAvailable() const
{
    return m_backend->sensorAvailable();
}

bool RotationUtil::sensorEnabled() const
{
    return m_backend->sensorEnabled();
}

void RotationUtil::setSensorEnabled(bool enabled)
{
    m_backend->setSensorEnabled(enabled);
}

QString RotationUtil::orientation() const
{
    return m_backend->orientation();
}

QString RotationUtil::screenRotation() const
{
    return m_backend->screenRotation();
}

QString RotationUtil::suggestedOrientation() const
{
    const QString value = m_backend->orientation();
    return m_backend->targetRotationFor(value).isEmpty() ? QString() : value;
}

QString RotationUtil::suggestedRotation() const
{
    return m_backend->targetRotationFor(m_backend->orientation());
}

QString RotationUtil::suggestedDirection() const
{
    return RotationMapping::direction(suggestedRotation(), m_backend->screenRotation());
}

bool RotationUtil::suggestionVisible() const
{
    if (!m_backend->suggestionPossible()) {
        return false;
    }

    const QString value = m_backend->orientation();
    if (value.isEmpty() || value == QLatin1String("undefined")) {
        return false;
    }
    if (m_dismissedOrientation == value) {
        return false;
    }

    const QString target = m_backend->targetRotationFor(value);
    return !target.isEmpty() && target != m_backend->screenRotation();
}

bool RotationUtil::rotating() const
{
    return m_backend->rotating();
}

void RotationUtil::rotateToSuggested()
{
    if (m_backend->rotating()) {
        return;
    }

    const QString value = suggestedOrientation();
    if (value.isEmpty()) {
        return;
    }
    m_backend->rotateToOrientation(value);
}

void RotationUtil::dismiss()
{
    m_dismissedOrientation = m_backend->orientation();
    refreshSuggestion();
}

void RotationUtil::refreshSuggestion()
{
    const QString newOrientation = suggestedOrientation();
    const bool newVisible = suggestionVisible();

    if (newOrientation != m_lastSuggestedOrientation) {
        m_lastSuggestedOrientation = newOrientation;
        Q_EMIT suggestedOrientationChanged();
    }
    if (newVisible != m_lastSuggestionVisible) {
        m_lastSuggestionVisible = newVisible;
        Q_EMIT suggestionVisibleChanged();
    }

    Q_EMIT suggestionChanged();
}
