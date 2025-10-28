/****************************************************************************
 *
 * QGroundControl Waypoint Creation Examples - QML
 * 
 * This file demonstrates how to create waypoints in QGroundControl
 * using QML and the MissionController.
 *
 ****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtPositioning 5.15

import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.PlanView 1.0
import QGroundControl.Vehicle 1.0

/**
 * Example 1: Basic Waypoint Creation Widget
 * 
 * This shows how to create a simple UI for adding waypoints
 */
Rectangle {
    id: waypointCreationWidget
    width: 400
    height: 300
    color: "white"
    border.color: "gray"
    border.width: 1

    property var missionController: QGroundControl.planMasterController.missionController

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "Waypoint Creation Examples"
            font.pixelSize: 16
            font.bold: true
        }

        // Basic waypoint creation
        GroupBox {
            title: "Basic Waypoint Creation"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                RowLayout {
                    QGCLabel { text: "Latitude:" }
                    QGCTextField {
                        id: latField
                        text: "37.7749"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Longitude:" }
                    QGCTextField {
                        id: lonField
                        text: "-122.4194"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Altitude:" }
                    QGCTextField {
                        id: altField
                        text: "100.0"
                        Layout.fillWidth: true
                    }
                }

                QGCButton {
                    text: "Add Waypoint"
                    Layout.fillWidth: true
                    onClicked: {
                        var coordinate = QtPositioning.coordinate(
                            parseFloat(latField.text),
                            parseFloat(lonField.text),
                            parseFloat(altField.text)
                        )
                        var nextIndex = missionController.visualItems.count
                        missionController.insertSimpleMissionItem(coordinate, nextIndex, true)
                    }
                }
            }
        }

        // Advanced waypoint creation
        GroupBox {
            title: "Advanced Waypoint Creation"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                RowLayout {
                    QGCLabel { text: "Hold Time (s):" }
                    QGCTextField {
                        id: holdTimeField
                        text: "5.0"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Acceptance Radius (m):" }
                    QGCTextField {
                        id: radiusField
                        text: "10.0"
                        Layout.fillWidth: true
                    }
                }

                QGCButton {
                    text: "Add Waypoint with Parameters"
                    Layout.fillWidth: true
                    onClicked: {
                        var coordinate = QtPositioning.coordinate(
                            parseFloat(latField.text),
                            parseFloat(lonField.text),
                            parseFloat(altField.text)
                        )
                        var nextIndex = missionController.visualItems.count
                        var waypoint = missionController.insertSimpleMissionItem(coordinate, nextIndex, true)
                        
                        if (waypoint) {
                            // Set custom parameters
                            waypoint.missionItem.param1 = parseFloat(holdTimeField.text)
                            waypoint.missionItem.param2 = parseFloat(radiusField.text)
                        }
                    }
                }
            }
        }
    }
}

/**
 * Example 2: Waypoint Creation with Map Integration
 * 
 * This shows how to create waypoints by clicking on a map
 */
Rectangle {
    id: mapWaypointWidget
    width: 600
    height: 400
    color: "white"
    border.color: "gray"
    border.width: 1

    property var missionController: QGroundControl.planMasterController.missionController
    property bool addWaypointMode: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "Map-Based Waypoint Creation"
            font.pixelSize: 16
            font.bold: true
        }

        // Map control buttons
        RowLayout {
            spacing: 10

            QGCButton {
                text: addWaypointMode ? "Exit Add Mode" : "Add Waypoints"
                onClicked: {
                    addWaypointMode = !addWaypointMode
                }
            }

            QGCButton {
                text: "Add Takeoff"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 50.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertTakeoffItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Add Landing"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 0.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertLandItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Add ROI"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 100.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertROIMissionItem(coordinate, nextIndex, true)
                }
            }
        }

        // Simulated map area (in real implementation, this would be a Map component)
        Rectangle {
            id: mapArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "lightblue"
            border.color: "darkblue"
            border.width: 2

            Text {
                anchors.centerIn: parent
                text: "Map Area\n(Click to add waypoints when in add mode)"
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                anchors.fill: parent
                enabled: addWaypointMode
                onClicked: {
                    if (addWaypointMode) {
                        // Convert click position to coordinates (simplified)
                        var lat = 37.7749 + (mouseY - height/2) * 0.001
                        var lon = -122.4194 + (mouseX - width/2) * 0.001
                        var coordinate = QtPositioning.coordinate(lat, lon, 100.0)
                        
                        var nextIndex = missionController.visualItems.count
                        missionController.insertSimpleMissionItem(coordinate, nextIndex, true)
                    }
                }
            }
        }
    }
}

/**
 * Example 3: Batch Waypoint Creation
 * 
 * This shows how to create multiple waypoints at once
 */
Rectangle {
    id: batchWaypointWidget
    width: 500
    height: 400
    color: "white"
    border.color: "gray"
    border.width: 1

    property var missionController: QGroundControl.planMasterController.missionController

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "Batch Waypoint Creation"
            font.pixelSize: 16
            font.bold: true
        }

        // Grid pattern creation
        GroupBox {
            title: "Grid Pattern"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                RowLayout {
                    QGCLabel { text: "Center Lat:" }
                    QGCTextField {
                        id: gridCenterLat
                        text: "37.7749"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Center Lon:" }
                    QGCTextField {
                        id: gridCenterLon
                        text: "-122.4194"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Altitude:" }
                    QGCTextField {
                        id: gridAlt
                        text: "100.0"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Spacing (m):" }
                    QGCTextField {
                        id: gridSpacing
                        text: "100.0"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Rows:" }
                    QGCTextField {
                        id: gridRows
                        text: "3"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Cols:" }
                    QGCTextField {
                        id: gridCols
                        text: "3"
                        Layout.fillWidth: true
                    }
                }

                QGCButton {
                    text: "Create Grid Pattern"
                    Layout.fillWidth: true
                    onClicked: {
                        var centerLat = parseFloat(gridCenterLat.text)
                        var centerLon = parseFloat(gridCenterLon.text)
                        var altitude = parseFloat(gridAlt.text)
                        var spacing = parseFloat(gridSpacing.text)
                        var rows = parseInt(gridRows.text)
                        var cols = parseInt(gridCols.text)
                        
                        var spacingDeg = spacing / 111320.0 // Convert meters to degrees
                        var startLat = centerLat - (rows - 1) * spacingDeg / 2.0
                        var startLon = centerLon - (cols - 1) * spacingDeg / 2.0
                        
                        for (var row = 0; row < rows; row++) {
                            for (var col = 0; col < cols; col++) {
                                var lat = startLat + row * spacingDeg
                                var lon = startLon + col * spacingDeg
                                var coordinate = QtPositioning.coordinate(lat, lon, altitude)
                                var nextIndex = missionController.visualItems.count
                                missionController.insertSimpleMissionItem(coordinate, nextIndex, false)
                            }
                        }
                    }
                }
            }
        }

        // Circular pattern creation
        GroupBox {
            title: "Circular Pattern"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                RowLayout {
                    QGCLabel { text: "Center Lat:" }
                    QGCTextField {
                        id: circleCenterLat
                        text: "37.7749"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Center Lon:" }
                    QGCTextField {
                        id: circleCenterLon
                        text: "-122.4194"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Altitude:" }
                    QGCTextField {
                        id: circleAlt
                        text: "100.0"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Radius (m):" }
                    QGCTextField {
                        id: circleRadius
                        text: "500.0"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    QGCLabel { text: "Points:" }
                    QGCTextField {
                        id: circlePoints
                        text: "8"
                        Layout.fillWidth: true
                    }
                }

                QGCButton {
                    text: "Create Circular Pattern"
                    Layout.fillWidth: true
                    onClicked: {
                        var centerLat = parseFloat(circleCenterLat.text)
                        var centerLon = parseFloat(circleCenterLon.text)
                        var altitude = parseFloat(circleAlt.text)
                        var radius = parseFloat(circleRadius.text)
                        var points = parseInt(circlePoints.text)
                        
                        for (var i = 0; i < points; i++) {
                            var angle = 2.0 * Math.PI * i / points
                            var lat = centerLat + radius * Math.cos(angle) / 111320.0
                            var lon = centerLon + radius * Math.sin(angle) / (111320.0 * Math.cos(centerLat * Math.PI / 180.0))
                            var coordinate = QtPositioning.coordinate(lat, lon, altitude)
                            var nextIndex = missionController.visualItems.count
                            missionController.insertSimpleMissionItem(coordinate, nextIndex, false)
                        }
                    }
                }
            }
        }
    }
}

/**
 * Example 4: Waypoint Management
 * 
 * This shows how to manage existing waypoints
 */
Rectangle {
    id: waypointManagementWidget
    width: 600
    height: 300
    color: "white"
    border.color: "gray"
    border.width: 1

    property var missionController: QGroundControl.planMasterController.missionController

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "Waypoint Management"
            font.pixelSize: 16
            font.bold: true
        }

        // Waypoint list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: waypointList
                model: missionController.visualItems
                spacing: 5

                delegate: Rectangle {
                    width: waypointList.width
                    height: 60
                    color: index === missionController.currentPlanViewVIIndex ? "lightblue" : "white"
                    border.color: "gray"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 10

                        Text {
                            text: "WP " + (index + 1)
                            font.bold: true
                            Layout.preferredWidth: 40
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "Lat: " + model.coordinate.latitude.toFixed(6)
                                font.pixelSize: 12
                            }

                            Text {
                                text: "Lon: " + model.coordinate.longitude.toFixed(6)
                                font.pixelSize: 12
                            }

                            Text {
                                text: "Alt: " + model.coordinate.altitude.toFixed(1) + "m"
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            spacing: 5

                            QGCButton {
                                text: "Select"
                                onClicked: {
                                    missionController.setCurrentPlanViewSeqNum(model.sequenceNumber, true)
                                }
                            }

                            QGCButton {
                                text: "Delete"
                                onClicked: {
                                    missionController.removeItem(index)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Management buttons
        RowLayout {
            spacing: 10

            QGCButton {
                text: "Clear All"
                onClicked: {
                    missionController.removeAll()
                }
            }

            QGCButton {
                text: "Move Up"
                onClicked: {
                    var currentIndex = missionController.currentPlanViewVIIndex
                    if (currentIndex > 0) {
                        missionController.moveItem(currentIndex, currentIndex - 1)
                    }
                }
            }

            QGCButton {
                text: "Move Down"
                onClicked: {
                    var currentIndex = missionController.currentPlanViewVIIndex
                    if (currentIndex < missionController.visualItems.count - 1) {
                        missionController.moveItem(currentIndex, currentIndex + 1)
                    }
                }
            }
        }
    }
}

/**
 * Example 5: Mission Planning Interface
 * 
 * This shows a complete mission planning interface
 */
Rectangle {
    id: missionPlanningWidget
    width: 800
    height: 600
    color: "white"
    border.color: "gray"
    border.width: 1

    property var missionController: QGroundControl.planMasterController.missionController

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "Mission Planning Interface"
            font.pixelSize: 18
            font.bold: true
        }

        // Mission statistics
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "lightgray"
            border.color: "gray"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 20

                Text {
                    text: "Waypoints: " + missionController.visualItems.count
                    font.bold: true
                }

                Text {
                    text: "Total Distance: " + missionController.missionTotalDistance.toFixed(1) + "m"
                    font.bold: true
                }

                Text {
                    text: "Mission Time: " + missionController.missionTime.toFixed(1) + "s"
                    font.bold: true
                }
            }
        }

        // Mission actions
        RowLayout {
            spacing: 10

            QGCButton {
                text: "Add Takeoff"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 50.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertTakeoffItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Add Waypoint"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 100.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertSimpleMissionItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Add Landing"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 0.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertLandItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Add ROI"
                onClicked: {
                    var coordinate = QtPositioning.coordinate(37.7749, -122.4194, 100.0)
                    var nextIndex = missionController.visualItems.count
                    missionController.insertROIMissionItem(coordinate, nextIndex, true)
                }
            }

            QGCButton {
                text: "Clear Mission"
                onClicked: {
                    missionController.removeAll()
                }
            }
        }

        // Mission list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: missionList
                model: missionController.visualItems
                spacing: 2

                delegate: Rectangle {
                    width: missionList.width
                    height: 50
                    color: index === missionController.currentPlanViewVIIndex ? "lightblue" : "white"
                    border.color: "gray"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 10

                        Text {
                            text: (index + 1).toString()
                            font.bold: true
                            Layout.preferredWidth: 30
                        }

                        Text {
                            text: model.commandName || "Waypoint"
                            Layout.fillWidth: true
                        }

                        Text {
                            text: model.coordinate.latitude.toFixed(4) + ", " + model.coordinate.longitude.toFixed(4)
                            Layout.preferredWidth: 150
                        }

                        Text {
                            text: model.coordinate.altitude.toFixed(1) + "m"
                            Layout.preferredWidth: 60
                        }

                        QGCButton {
                            text: "Edit"
                            onClicked: {
                                missionController.setCurrentPlanViewSeqNum(model.sequenceNumber, true)
                            }
                        }

                        QGCButton {
                            text: "Delete"
                            onClicked: {
                                missionController.removeItem(index)
                            }
                        }
                    }
                }
            }
        }
    }
}