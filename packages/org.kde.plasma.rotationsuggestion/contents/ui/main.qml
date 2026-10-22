// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import org.kde.plasma.private.rotation 1.0

PlasmoidItem {
    id: root

    // Active only while a suggestion exists. The system tray owns the slot and
    // placement, so the item disappears cleanly after dismissal.
    Plasmoid.status: backend.suggestionVisible ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.HiddenStatus
    Plasmoid.icon: backend.suggestedDirection === "left" ? "object-rotate-left-symbolic" : "object-rotate-right-symbolic"
    toolTipMainText: i18n("Rotate screen")
    toolTipSubText: i18n("Tap to match the device orientation")

    RotationUtil {
        id: backend
        sensorEnabled: true
        onErrorOccurred: function(message) {
            console.warn("rotation-suggestion:", message)
        }
    }

    Timer {
        id: hideTimer
        interval: 8000
        running: backend.suggestionVisible
        repeat: false
        onTriggered: backend.dismiss()
    }

    Connections {
        target: backend
        function onSuggestedOrientationChanged() {
            if (backend.suggestionVisible) {
                hideTimer.restart()
            }
        }
        function onSuggestionVisibleChanged() {
            if (backend.suggestionVisible) {
                hideTimer.restart()
            }
        }
    }

    compactRepresentation: Item {
        implicitWidth: Kirigami.Units.iconSizes.smallMedium
        implicitHeight: Kirigami.Units.iconSizes.smallMedium
        visible: backend.suggestionVisible
        opacity: backend.suggestionVisible ? 1 : 0
        scale: backend.suggestionVisible ? 1 : 0.8

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
        Behavior on scale {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutCubic
            }
        }

        Kirigami.Icon {
            anchors.fill: parent
            source: backend.suggestedDirection === "left" ? "object-rotate-left-symbolic"
                : backend.suggestedDirection === "flip" ? "transform-rotate-symbolic"
                : "object-rotate-right-symbolic"
            opacity: mouse.containsMouse ? 1 : 0.85
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: !backend.rotating
            Accessible.name: i18n("Rotate screen")
            Accessible.description: i18n("Tap to match the device orientation")
            Accessible.role: Accessible.Button
            Accessible.onPressAction: backend.rotateToSuggested()
            onClicked: backend.rotateToSuggested()
        }
    }

    fullRepresentation: PlasmaExtras.Representation {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 14
        Layout.minimumHeight: Kirigami.Units.gridUnit * 7
        collapseMarginsHint: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                text: i18n("Rotate screen?")
                font.bold: true
            }

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                text: backend.suggestionVisible
                    ? i18n("The device orientation changed while auto-rotate is off.")
                    : i18n("No rotation suggestion right now.")
                wrapMode: Text.WordWrap
            }

            PlasmaComponents3.Button {
                Layout.fillWidth: true
                text: backend.rotating ? i18n("Rotating…") : i18n("Rotate")
                icon.name: backend.suggestedDirection === "left" ? "object-rotate-left-symbolic"
                    : backend.suggestedDirection === "flip" ? "transform-rotate-symbolic"
                    : "object-rotate-right-symbolic"
                enabled: backend.suggestionVisible && !backend.rotating
                onClicked: backend.rotateToSuggested()
            }
        }
    }
}
