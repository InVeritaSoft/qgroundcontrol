/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.15
import QtLocation       5.15
import QtPositioning    5.15

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.PlanView      1.0

Rectangle {
    id: root
    
    property var planMasterController: null
    property var map: null
    
    color: qgcPal.window
    border.color: qgcPal.colorGrey
    border.width: 1
    radius: ScreenTools.defaultFontPixelHeight / 4
    
    // Mission Area Planner instance
    MissionAreaPlanner {
        id: missionAreaPlanner
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight / 2
        spacing: ScreenTools.defaultFontPixelHeight / 2
        
        // Header
        QGCLabel {
            text: "Mission Area Planner"
            font.pixelSize: ScreenTools.largeFontPixelSize
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }
        // Busy/loading indicator
        TimedProgressTracker {
            id: busyIndicator
            anchors.horizontalCenter: parent.horizontalCenter
            timeoutSeconds: 10
            running: missionAreaPlanner && missionAreaPlanner.busy
            progressLabel: missionAreaPlanner && missionAreaPlanner.busy ? qsTr("Processing...") : ""
            visible: running
        }
        // Inline feedback label
        QGCLabel {
            id: feedbackLabel
            text: missionAreaPlanner && missionAreaPlanner.feedbackText ? missionAreaPlanner.feedbackText : ""
            color: missionAreaPlanner && missionAreaPlanner.feedbackError ? qgcPal.colorRed : qgcPal.colorGreen
            visible: text.length > 0
            anchors.horizontalCenter: parent.horizontalCenter
        }
        // Center coordinate input fields
        RowLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2
            QGCLabel { text: "Lat:" }
            QGCTextField {
                id: latField
                text: missionAreaPlanner.center ? missionAreaPlanner.center.latitude.toFixed(7) : ""
                width: 100
                onEditingFinished: {
                    var lat = parseFloat(text)
                    if (!isNaN(lat) && missionAreaPlanner.center) {
                        var c = missionAreaPlanner.center
                        missionAreaPlanner.center = QtPositioning.coordinate(lat, c.longitude)
                    }
                }
            }
            QGCLabel { text: "Lon:" }
            QGCTextField {
                id: lonField
                text: missionAreaPlanner.center ? missionAreaPlanner.center.longitude.toFixed(7) : ""
                width: 100
                onEditingFinished: {
                    var lon = parseFloat(text)
                    if (!isNaN(lon) && missionAreaPlanner.center) {
                        var c = missionAreaPlanner.center
                        missionAreaPlanner.center = QtPositioning.coordinate(c.latitude, lon)
                    }
                }
            }
        }
        // Home location controls
        RowLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2
            QGCButton {
                text: "Set Home to Device Location"
                onClicked: {
                    var gcsPos = QGroundControl.positionManager.gcsPosition
                    if (gcsPos && gcsPos.isValid) {
                        missionAreaPlanner.center = gcsPos
                    } else {
                        QGroundControl.showMessage("Device location unavailable. Please enable location services or select home on map.")
                    }
                }
            }
            QGCButton {
                text: "Select Home on Map"
                checkable: true
                checked: root.selectingHomeOnMap
                onClicked: root.selectingHomeOnMap = !root.selectingHomeOnMap
            }
        }
        property bool selectingHomeOnMap: false
        
        // Mission Area Planner Component
        MissionAreaPlanner {
            id: plannerComponent
            missionAreaPlanner: missionAreaPlanner
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        // Map Overlay Component (if map is provided)
        MissionAreaMapOverlay {
            id: mapOverlay
            missionAreaPlanner: missionAreaPlanner
            map: root.map
            visible: root.map !== null
            selectingHomeOnMap: root.selectingHomeOnMap
            onHomeLocationSelected: function(coord) {
                missionAreaPlanner.center = coord
                root.selectingHomeOnMap = false
            }
        }
    }
    
    // Initialize with current vehicle position if available
    Component.onCompleted: {
        if (planMasterController && planMasterController.managerVehicle) {
            var vehicle = planMasterController.managerVehicle
            if (vehicle.coordinate.isValid) {
                missionAreaPlanner.center = vehicle.coordinate
            }
        }
    }
} 