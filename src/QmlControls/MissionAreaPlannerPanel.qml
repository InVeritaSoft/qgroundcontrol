/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtPositioning 5.15

import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.PlanView 1.0
import QGroundControl.FlightMap 1.0
import QGroundControl.Controllers 1.0

Rectangle {
    id: missionAreaPlannerPanel
    
    property var planMasterController
    property var map
    
    // Mission Area Planner properties
    property real areaWidth: 100.0      // meters
    property real areaHeight: 100.0     // meters
    property real lineSpacing: 10.0     // meters
    property int pointsPerLine: 10
    property var areaCenter: QtPositioning.coordinate(0, 0)
    property bool isVisible: true
    
    width: parent.width
    height: missionAreaPlannerColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
    color: qgcPal.window
    border.color: qgcPal.colorGrey
    border.width: 1
    radius: ScreenTools.defaultFontPixelWidth * 0.5
    
    ColumnLayout {
        id: missionAreaPlannerColumn
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.5
        spacing: ScreenTools.defaultFontPixelHeight * 0.5
        
        // Header
        QGCLabel {
            text: qsTr("Mission Area Planner")
            font.pointSize: ScreenTools.largeFontPointSize
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
        
        // Area Parameters Section
        SectionHeader {
            text: qsTr("Area Parameters")
            Layout.fillWidth: true
        }
        
        GridLayout {
            columns: 2
            columnSpacing: ScreenTools.defaultFontPixelWidth
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.25
            Layout.fillWidth: true
            
            QGCLabel {
                text: qsTr("Width:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: widthTextField
                text: areaWidth.toString()
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    areaWidth = parseFloat(text) || 100.0
                    missionAreaPlanner.updateArea()
                }
            }
            
            QGCLabel {
                text: qsTr("Height:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: heightTextField
                text: areaHeight.toString()
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    areaHeight = parseFloat(text) || 100.0
                    missionAreaPlanner.updateArea()
                }
            }
            
            QGCLabel {
                text: qsTr("Line Spacing:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: spacingTextField
                text: lineSpacing.toString()
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    lineSpacing = parseFloat(text) || 10.0
                    missionAreaPlanner.updateGrid()
                }
            }
            
            QGCLabel {
                text: qsTr("Points per Line:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: pointsTextField
                text: pointsPerLine.toString()
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    pointsPerLine = parseInt(text) || 10
                    missionAreaPlanner.updateGrid()
                }
            }
        }
        
        // Area Center Section
        SectionHeader {
            text: qsTr("Area Center")
            Layout.fillWidth: true
        }
        
        GridLayout {
            columns: 2
            columnSpacing: ScreenTools.defaultFontPixelWidth
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.25
            Layout.fillWidth: true
            
            QGCLabel {
                text: qsTr("Latitude:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: latTextField
                text: areaCenter.latitude.toFixed(8)
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    var newLat = parseFloat(text)
                    if (!isNaN(newLat)) {
                        areaCenter.latitude = newLat
                        missionAreaPlanner.updateArea()
                    }
                }
            }
            
            QGCLabel {
                text: qsTr("Longitude:")
                Layout.alignment: Qt.AlignVCenter
            }
            
            QGCTextField {
                id: lonTextField
                text: areaCenter.longitude.toFixed(8)
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: {
                    var newLon = parseFloat(text)
                    if (!isNaN(newLon)) {
                        areaCenter.longitude = newLon
                        missionAreaPlanner.updateArea()
                    }
                }
            }
        }
        
        // Control Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth
            
            QGCButton {
                text: qsTr("Set Current Location")
                Layout.fillWidth: true
                onClicked: {
                    if (map && map.center) {
                        areaCenter = map.center
                        latTextField.text = areaCenter.latitude.toFixed(8)
                        lonTextField.text = areaCenter.longitude.toFixed(8)
                        missionAreaPlanner.updateArea()
                    }
                }
            }
            
            QGCButton {
                text: qsTr("Generate Mission")
                Layout.fillWidth: true
                enabled: planMasterController && planMasterController.missionController
                onClicked: {
                    missionAreaPlanner.generateMission()
                }
            }
        }
        
        // Directional Movement Controls
        SectionHeader {
            text: qsTr("Move Area")
            Layout.fillWidth: true
        }
        
        GridLayout {
            columns: 3
            columnSpacing: ScreenTools.defaultFontPixelWidth
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.25
            Layout.fillWidth: true
            
            Item { Layout.fillWidth: true }
            
            QGCButton {
                text: qsTr("N")
                Layout.fillWidth: true
                onClicked: missionAreaPlanner.moveArea(0, -10) // North
            }
            
            Item { Layout.fillWidth: true }
            
            QGCButton {
                text: qsTr("W")
                Layout.fillWidth: true
                onClicked: missionAreaPlanner.moveArea(-10, 0) // West
            }
            
            QGCButton {
                text: qsTr("Center")
                Layout.fillWidth: true
                onClicked: {
                    if (map && map.center) {
                        areaCenter = map.center
                        latTextField.text = areaCenter.latitude.toFixed(8)
                        lonTextField.text = areaCenter.longitude.toFixed(8)
                        missionAreaPlanner.updateArea()
                    }
                }
            }
            
            QGCButton {
                text: qsTr("E")
                Layout.fillWidth: true
                onClicked: missionAreaPlanner.moveArea(10, 0) // East
            }
            
            Item { Layout.fillWidth: true }
            
            QGCButton {
                text: qsTr("S")
                Layout.fillWidth: true
                onClicked: missionAreaPlanner.moveArea(0, 10) // South
            }
            
            Item { Layout.fillWidth: true }
        }
        
        // Status Display
        QGCLabel {
            id: statusLabel
            text: qsTr("Ready")
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            color: qgcPal.colorGreen
        }
    }
    
    // Mission Area Planner backend
    MissionAreaPlanner {
        id: missionAreaPlanner
        planMasterController: missionAreaPlannerPanel.planMasterController
        map: missionAreaPlannerPanel.map
        areaCenter: missionAreaPlannerPanel.areaCenter
        areaWidth: missionAreaPlannerPanel.areaWidth
        areaHeight: missionAreaPlannerPanel.areaHeight
        lineSpacing: missionAreaPlannerPanel.lineSpacing
        pointsPerLine: missionAreaPlannerPanel.pointsPerLine
        
        onStatusChanged: {
            statusLabel.text = status
            statusLabel.color = statusColor
        }
    }
} 