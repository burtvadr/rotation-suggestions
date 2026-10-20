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

    // Passive keeps this in the tray popup's grid instead of the tray bar.
    Plasmoid.status: PlasmaCore.Types.PassiveStatus
    Plasmoid.icon: backend.autoRotateEnabled ? "rotation-allowed-symbolic" : "rotation-locked-landscape-symbolic"
    toolTipMainText: i18n("Auto-rotate")
    toolTipSubText: !backend.autoRotateAvailable ? i18n("Not available")
        : backend.autoRotateEnabled ? i18n("On") : i18n("Off")

    RotationUtil {
        id: backend
        sensorEnabled: false
        onErrorOccurred: function(message) {
            console.warn("auto-rotate:", message)
        }
    }

    compactRepresentation: Item {
        implicitWidth: Kirigami.Units.iconSizes.smallMedium
        implicitHeight: Kirigami.Units.iconSizes.smallMedium

        Kirigami.Icon {
            anchors.fill: parent
            source: backend.autoRotateEnabled ? "rotation-allowed-symbolic" : "rotation-locked-landscape-symbolic"
            opacity: mouse.containsMouse ? 1 : 0.85
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: backend.autoRotateAvailable
            Accessible.name: i18n("Auto-rotate")
            Accessible.description: backend.autoRotateEnabled ? i18n("On") : i18n("Off")
            Accessible.role: Accessible.Button
            Accessible.onPressAction: backend.autoRotateEnabled = !backend.autoRotateEnabled
            onClicked: backend.autoRotateEnabled = !backend.autoRotateEnabled
        }
    }

    fullRepresentation: PlasmaExtras.Representation {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 14
        Layout.minimumHeight: Kirigami.Units.gridUnit * 6
        collapseMarginsHint: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                text: i18n("Auto-rotate screen")
                font.bold: true
            }

            PlasmaComponents3.Switch {
                Layout.fillWidth: true
                text: backend.autoRotateEnabled ? i18n("On") : i18n("Off")
                checked: backend.autoRotateEnabled
                enabled: backend.autoRotateAvailable
                onToggled: backend.autoRotateEnabled = checked
            }

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                visible: !backend.autoRotateAvailable
                text: i18n("The auto-rotate service is not available.")
                wrapMode: Text.WordWrap
            }
        }
    }
}
