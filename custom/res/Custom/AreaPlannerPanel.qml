import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.FactSystem
import QGroundControl.FactControls

Rectangle {
    id: areaPlannerPanel
    color: qgcPal.window
    border.color: qgcPal.colorGrey
    border.width: 1
    radius: 5

    property var planMasterController
    property var map

    QGCPalette { id: qgcPal }

    readonly property real _margin: ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: _margin
        spacing: _margin

        // Title
        QGCLabel {
            text: "Area Planner"
            font.pointSize: ScreenTools.largeFontPointSize
            font.bold: true
            Layout.fillWidth: true
        }

        // Area Definition Section
        SectionHeader {
            text: "Area Definition"
            Layout.fillWidth: true
        }

        GridLayout {
            columns: 2
            columnSpacing: _margin
            rowSpacing: _margin
            Layout.fillWidth: true

            QGCLabel { text: "Width (m):" }
            QGCTextField {
                id: widthField
                text: missionAreaPlanner.width
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseFloat(text)
                    if (!isNaN(value) && value > 0) {
                        missionAreaPlanner.width = value
                    }
                }
            }

            QGCLabel { text: "Height (m):" }
            QGCTextField {
                id: heightField
                text: missionAreaPlanner.height
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseFloat(text)
                    if (!isNaN(value) && value > 0) {
                        missionAreaPlanner.height = value
                    }
                }
            }

            QGCLabel { text: "Line Spacing (m):" }
            QGCTextField {
                id: lineSpacingField
                text: missionAreaPlanner.lineSpacing
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseFloat(text)
                    if (!isNaN(value) && value > 0) {
                        missionAreaPlanner.lineSpacing = value
                    }
                }
            }

            QGCLabel { text: "Points per Line:" }
            QGCTextField {
                id: numPointsField
                text: missionAreaPlanner.numPoints
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseInt(text)
                    if (!isNaN(value) && value > 0) {
                        missionAreaPlanner.numPoints = value
                    }
                }
            }
        }

        // Center Coordinates Section
        SectionHeader {
            text: "Center Coordinates"
            Layout.fillWidth: true
        }

        GridLayout {
            columns: 2
            columnSpacing: _margin
            rowSpacing: _margin
            Layout.fillWidth: true

            QGCLabel { text: "Latitude:" }
            QGCTextField {
                id: latField
                text: missionAreaPlanner.center.latitude.toFixed(6)
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseFloat(text)
                    if (!isNaN(value)) {
                        var newCenter = QtPositioning.coordinate(value, missionAreaPlanner.center.longitude)
                        missionAreaPlanner.center = newCenter
                    }
                }
            }

            QGCLabel { text: "Longitude:" }
            QGCTextField {
                id: lonField
                text: missionAreaPlanner.center.longitude.toFixed(6)
                Layout.preferredWidth: _fieldWidth
                onTextChanged: {
                    var value = parseFloat(text)
                    if (!isNaN(value)) {
                        var newCenter = QtPositioning.coordinate(missionAreaPlanner.center.latitude, value)
                        missionAreaPlanner.center = newCenter
                    }
                }
            }
        }

        // Mission Generation Section
        SectionHeader {
            text: "Mission Generation"
            Layout.fillWidth: true
        }

        ColumnLayout {
            spacing: _margin
            Layout.fillWidth: true

            QGCButton {
                text: "Generate Mission"
                Layout.fillWidth: true
                enabled: !missionAreaPlanner.busy
                onClicked: missionAreaPlanner.generateMission()
            }

            QGCButton {
                text: "Clear Mission"
                Layout.fillWidth: true
                enabled: !missionAreaPlanner.busy
                onClicked: missionAreaPlanner.clearMission()
            }
        }

        // Status Section
        SectionHeader {
            text: "Status"
            Layout.fillWidth: true
        }

        QGCLabel {
            text: missionAreaPlanner.status
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        QGCProgressBar {
            visible: missionAreaPlanner.busy
            Layout.fillWidth: true
            indeterminate: true
        }

        // Spacer to push everything to the top
        Item {
            Layout.fillHeight: true
        }
    }

    // MissionAreaPlanner instance
    MissionAreaPlanner {
        id: missionAreaPlanner
    }
} 