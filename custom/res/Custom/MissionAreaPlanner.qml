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

Rectangle {
    id: root
    
    property var missionAreaPlanner: null
    property var missionController: null // <-- Add this line
    property bool busy: false
    property string feedbackText: ""
    property bool feedbackError: false
    
    color: qgcPal.window
    border.color: qgcPal.colorGrey
    border.width: 1
    radius: ScreenTools.defaultFontPixelHeight / 4
    
    implicitWidth: 300
    implicitHeight: 400
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight / 2
        spacing: ScreenTools.defaultFontPixelHeight / 2
        
        // Title
        QGCLabel {
            text: "Mission Area Planner"
            font.pixelSize: ScreenTools.largeFontPixelSize
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }
        
        // Area Parameters Section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: areaParamsColumn.height + ScreenTools.defaultFontPixelHeight
            color: qgcPal.colorGrey
            radius: ScreenTools.defaultFontPixelHeight / 4
            
            ColumnLayout {
                id: areaParamsColumn
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelHeight / 2
                spacing: ScreenTools.defaultFontPixelHeight / 2
                
                QGCLabel {
                    text: "Area Parameters"
                    font.pixelSize: ScreenTools.mediumFontPixelSize
                    font.bold: true
                }
                
                // Center Coordinates
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    
                    QGCLabel { text: "Center Lat:" }
                    QGCTextField {
                        id: centerLatField
                        text: missionAreaPlanner ? missionAreaPlanner.center.latitude.toFixed(6) : "0.000000"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                var coord = missionAreaPlanner.center
                                coord.latitude = parseFloat(text)
                                missionAreaPlanner.center = coord
                            }
                        }
                    }
                    
                    QGCLabel { text: "Center Lon:" }
                    QGCTextField {
                        id: centerLonField
                        text: missionAreaPlanner ? missionAreaPlanner.center.longitude.toFixed(6) : "0.000000"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                var coord = missionAreaPlanner.center
                                coord.longitude = parseFloat(text)
                                missionAreaPlanner.center = coord
                            }
                        }
                    }
                }
                
                // Width and Height
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    
                    QGCLabel { text: "Width (m):" }
                    QGCTextField {
                        id: widthField
                        text: missionAreaPlanner ? missionAreaPlanner.width.toFixed(1) : "100.0"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                missionAreaPlanner.width = parseFloat(text)
                            }
                        }
                    }
                    
                    QGCLabel { text: "Height (m):" }
                    QGCTextField {
                        id: heightField
                        text: missionAreaPlanner ? missionAreaPlanner.height.toFixed(1) : "100.0"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                missionAreaPlanner.height = parseFloat(text)
                            }
                        }
                    }
                }
            }
        }
        
        // Grid Parameters Section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: gridParamsColumn.height + ScreenTools.defaultFontPixelHeight
            color: qgcPal.colorGrey
            radius: ScreenTools.defaultFontPixelHeight / 4
            
            ColumnLayout {
                id: gridParamsColumn
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelHeight / 2
                spacing: ScreenTools.defaultFontPixelHeight / 2
                
                QGCLabel {
                    text: "Grid Parameters"
                    font.pixelSize: ScreenTools.mediumFontPixelSize
                    font.bold: true
                }
                
                // Line Spacing and Points Per Line
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    
                    QGCLabel { text: "Line Spacing (m):" }
                    QGCTextField {
                        id: lineSpacingField
                        text: missionAreaPlanner ? missionAreaPlanner.lineSpacing.toFixed(1) : "10.0"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                missionAreaPlanner.lineSpacing = parseFloat(text)
                            }
                        }
                    }
                    
                    QGCLabel { text: "Points per Line:" }
                    QGCTextField {
                        id: pointsPerLineField
                        text: missionAreaPlanner ? missionAreaPlanner.pointsPerLine.toString() : "10"
                        onEditingFinished: {
                            if (missionAreaPlanner) {
                                missionAreaPlanner.pointsPerLine = parseInt(text)
                            }
                        }
                    }
                }
            }
        }
        
        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 2
            
            QGCButton {
                text: "Set Current Location"
                Layout.fillWidth: true
                onClicked: {
                    if (missionAreaPlanner && _activeVehicle) {
                        var coord = _activeVehicle.coordinate
                        missionAreaPlanner.center = coord
                    }
                }
            }
            
            QGCButton {
                text: "Generate Mission"
                Layout.fillWidth: true
                onClicked: {
                    if (missionAreaPlanner && missionController) {
                        // Optionally clear existing mission items
                        // missionController.removeAllVisualItems(); // Uncomment if you want to clear
                        var waypoints = missionAreaPlanner.generateMissionWaypoints();
                        console.log("Generated " + waypoints.length + " waypoints");
                        for (var i = 0; i < waypoints.length; i++) {
                            var coord = waypoints[i].coordinate;
                            // Insert as a simple mission item at the end
                            missionController.insertSimpleMissionItem(coord, -1, false);
                        }
                        // Upload to vehicle
                        var preCheck = missionController.sendToVehiclePreCheck();
                        if (preCheck === missionController.SendToVehiclePreCheckStateOk) {
                            missionController.sendToVehicle();
                            console.log("Mission upload started");
                        } else {
                            console.log("Mission upload pre-check failed: " + preCheck);
                        }
                    } else {
                        console.log("MissionAreaPlanner or MissionController not available");
                    }
                }
            }
        }
        
        // Status Information
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: statusColumn.height + ScreenTools.defaultFontPixelHeight
            color: qgcPal.colorGrey
            radius: ScreenTools.defaultFontPixelHeight / 4
            
            ColumnLayout {
                id: statusColumn
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelHeight / 2
                spacing: ScreenTools.defaultFontPixelHeight / 2
                
                QGCLabel {
                    text: "Status"
                    font.pixelSize: ScreenTools.mediumFontPixelSize
                    font.bold: true
                }
                
                QGCLabel {
                    text: "Grid Points: " + (missionAreaPlanner ? missionAreaPlanner.gridPoints.length : 0)
                    font.pixelSize: ScreenTools.smallFontPixelSize
                }
                
                QGCLabel {
                    text: "Grid Lines: " + (missionAreaPlanner ? missionAreaPlanner.gridLines.length : 0)
                    font.pixelSize: ScreenTools.smallFontPixelSize
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    // Update text fields when model changes
    Connections {
        target: missionAreaPlanner
        
        function onCenterChanged() {
            if (missionAreaPlanner) {
                centerLatField.text = missionAreaPlanner.center.latitude.toFixed(6)
                centerLonField.text = missionAreaPlanner.center.longitude.toFixed(6)
            }
        }
        
        function onWidthChanged() {
            if (missionAreaPlanner) {
                widthField.text = missionAreaPlanner.width.toFixed(1)
            }
        }
        
        function onHeightChanged() {
            if (missionAreaPlanner) {
                heightField.text = missionAreaPlanner.height.toFixed(1)
            }
        }
        
        function onLineSpacingChanged() {
            if (missionAreaPlanner) {
                lineSpacingField.text = missionAreaPlanner.lineSpacing.toFixed(1)
            }
        }
        
        function onPointsPerLineChanged() {
            if (missionAreaPlanner) {
                pointsPerLineField.text = missionAreaPlanner.pointsPerLine.toString()
            }
        }
    }
} 