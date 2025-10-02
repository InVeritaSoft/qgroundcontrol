import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id: root
    
    property var planMasterController
    property bool interactive: true
    
    // Create the AreaPlanEditor C++ instance
    AreaPlanEditor {
        id: areaPlanEditor
        planMasterController: root.planMasterController
        
        onWaypointsAddedToMission: function(count) {
            console.log("Area Plan Editor: Added", count, "waypoints to mission");
        }
        
        onMissionSaved: function() {
            console.log("Area Plan Editor: Mission saved successfully");
        }
    }
    
    // Main content area
    Column {
        anchors.fill: parent
        spacing: 10
        
        // Drawing mode toggle
        QGCButton {
            id: drawingModeButton
            text: {
                if (!areaPlanEditor) return qsTr("Activate Area Definition Mode")
                return areaPlanEditor.isDrawingMode ? 
                    qsTr("Deactivate Area Definition Mode") : 
                    qsTr("Activate Area Definition Mode")
            }
            width: parent.width
            height: 40
            onClicked: {
                if (areaPlanEditor) {
                    var newMode = !areaPlanEditor.isDrawingMode
                    areaPlanEditor.setIsDrawingMode(newMode)
                }
            }
        }
        
        // Area parameters
        Rectangle {
            width: parent.width
            height: areaParamsColumn.height + 20
            color: Qt.rgba(0, 0, 0, 0.1)
            radius: 5
            visible: areaPlanEditor && areaPlanEditor.isDrawingMode
            
            Column {
                id: areaParamsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 5
                
                QGCLabel {
                    text: qsTr("Area Parameters")
                    font.pointSize: 12
                    font.bold: true
                }
                
                // Area Width
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Width (m):")
                        width: 80
                    }
                    
                    QGCTextField {
                        text: areaPlanEditor ? areaPlanEditor.areaWidth.toString() : "100"
                        width: 100
                        onTextChanged: if (areaPlanEditor) areaPlanEditor.setAreaWidth(parseFloat(text))
                    }
                }
                
                // Area Height
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Height (m):")
                        width: 80
                    }
                    
                    QGCTextField {
                        text: areaPlanEditor ? areaPlanEditor.areaHeight.toString() : "100"
                        width: 100
                        onTextChanged: if (areaPlanEditor) areaPlanEditor.setAreaHeight(parseFloat(text))
                    }
                }
                
                // Line Spacing
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Line Spacing (m):")
                        width: 80
                    }
                    
                    QGCTextField {
                        text: areaPlanEditor ? areaPlanEditor.lineSpacing.toString() : "10"
                        width: 100
                        onTextChanged: if (areaPlanEditor) areaPlanEditor.setLineSpacing(parseFloat(text))
                    }
                }
                
                // Number of Points
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Points:")
                        width: 80
                    }
                    
                    QGCTextField {
                        text: areaPlanEditor ? areaPlanEditor.numPoints.toString() : "10"
                        width: 100
                        onTextChanged: if (areaPlanEditor) areaPlanEditor.setNumPoints(parseInt(text))
                    }
                }
                
                // Mission Altitude
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Altitude (m):")
                        width: 80
                    }
                    
                    QGCTextField {
                        text: areaPlanEditor ? areaPlanEditor.missionAltitude.toString() : "50"
                        width: 100
                        onTextChanged: if (areaPlanEditor) areaPlanEditor.setMissionAltitude(parseFloat(text))
                    }
                }
            }
        }
        
        // Water avoidance settings
        Rectangle {
            width: parent.width
            height: waterAvoidanceColumn.height + 20
            color: Qt.rgba(0, 0, 0, 0.1)
            radius: 5
            
            Column {
                id: waterAvoidanceColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 5
                
                QGCLabel {
                    text: qsTr("Safety Settings")
                    font.pointSize: 12
                    font.bold: true
                }
                
                // Water avoidance toggle
                Row {
                    width: parent.width
                    spacing: 10
                    
                    QGCLabel {
                        text: qsTr("Water Avoidance:")
                        width: 120
                    }
                    
                    QGCSwitch {
                        id: waterAvoidanceSwitch
                        checked: QGroundControl.settingsManager ? QGroundControl.settingsManager.appSettings.waterAvoidanceEnabled.rawValue : false
                        onToggled: {
                            if (QGroundControl.settingsManager) {
                                QGroundControl.settingsManager.appSettings.waterAvoidanceEnabled.rawValue = checked
                            }
                        }
                    }
                    
                    QGCLabel {
                        text: waterAvoidanceSwitch.checked ? qsTr("Enabled") : qsTr("Disabled")
                        color: waterAvoidanceSwitch.checked ? "green" : "red"
                    }
                }
                
                QGCLabel {
                    text: qsTr("When enabled, prevents mission generation over water areas")
                    font.pointSize: 10
                    color: "gray"
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
        
        // Area manipulation controls
        Rectangle {
            width: parent.width
            height: manipulationColumn.height + 20
            color: Qt.rgba(0, 0, 0, 0.1)
            radius: 5
            visible: areaPlanEditor && areaPlanEditor.isDrawingMode
            
            Column {
                id: manipulationColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 5
                
                QGCLabel {
                    text: qsTr("Area Manipulation")
                    font.pointSize: 12
                    font.bold: true
                }
                
                // Movement controls
                Grid {
                    columns: 3
                    spacing: 10
                    
                    QGCButton {
                        text: "↑"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaNorth()
                    }
                    
                    QGCButton {
                        text: "Center"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.centerArea()
                    }
                    
                    QGCButton {
                        text: "↻"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaClockwise()
                    }
                    
                    QGCButton {
                        text: "←"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaWest()
                    }
                    
                    QGCButton {
                        text: "↓"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaSouth()
                    }
                    
                    QGCButton {
                        text: "↺"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaCounterClockwise()
                    }
                    
                    QGCButton {
                        text: "→"
                        width: 60
                        height: 30
                        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaEast()
                    }
                }
            }
        }
        
        // Mission generation buttons
        Rectangle {
            width: parent.width
            height: missionButtonsColumn.height + 20
            color: Qt.rgba(0, 0, 0, 0.1)
            radius: 5
            visible: areaPlanEditor && areaPlanEditor.isDrawingMode
            
            Column {
                id: missionButtonsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 5
                
                QGCLabel {
                    text: qsTr("Mission Generation")
                    font.pointSize: 12
                    font.bold: true
                }
                
                QGCButton {
                    text: qsTr("Add Waypoints to Mission")
                    width: parent.width
                    height: 40
                    onClicked: if (areaPlanEditor) areaPlanEditor.addWaypointsToMission()
                }
                
                QGCButton {
                    text: qsTr("Save Mission File")
                    width: parent.width
                    height: 40
                    onClicked: if (areaPlanEditor) areaPlanEditor.saveMissionFile()
                }
            }
        }
    }
}