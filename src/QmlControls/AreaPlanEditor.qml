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
    
    // Compact control panel
    Rectangle {
        id: controlPanel
        width: ScreenTools.defaultFontPixelWidth * 25
        height: childrenRect.height + ScreenTools.defaultFontPixelHeight
        color: Qt.rgba(0, 0, 0, 0.8)
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        visible: areaPlanEditor && areaPlanEditor.isDrawingMode
        
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.5
            spacing: ScreenTools.defaultFontPixelHeight * 0.25
            
            // Drawing mode toggle
            QGCButton {
                text: qsTr("Exit Drawing Mode")
                width: parent.width
                height: ScreenTools.defaultFontPixelHeight * 2
                onClicked: areaPlanEditor.setIsDrawingMode(false)
            }
            
            // Quick parameters
            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                
                QGCLabel {
                    text: qsTr("W:")
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaPlanEditor ? areaPlanEditor.areaWidth : 0
                    onTextChanged: if (areaPlanEditor) areaPlanEditor.setAreaWidth(parseFloat(text) || 0)
                    Layout.fillWidth: true
                    placeholderText: "Width"
                }
                
                QGCLabel {
                    text: qsTr("H:")
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaPlanEditor ? areaPlanEditor.areaHeight : 0
                    onTextChanged: if (areaPlanEditor) areaPlanEditor.setAreaHeight(parseFloat(text) || 0)
                    Layout.fillWidth: true
                    placeholderText: "Height"
                }
            }
            
            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                
                QGCLabel {
                    text: qsTr("S:")
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaPlanEditor ? areaPlanEditor.lineSpacing : 0
                    onTextChanged: if (areaPlanEditor) areaPlanEditor.setLineSpacing(parseFloat(text) || 0)
                    Layout.fillWidth: true
                    placeholderText: "Spacing"
                }
                
                QGCLabel {
                    text: qsTr("P:")
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaPlanEditor ? areaPlanEditor.numPoints : 0
                    onTextChanged: if (areaPlanEditor) areaPlanEditor.setNumPoints(parseInt(text) || 0)
                    Layout.fillWidth: true
                    placeholderText: "Points"
                }
            }
            
            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                
                QGCLabel {
                    text: qsTr("Alt:")
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaPlanEditor ? areaPlanEditor.missionAltitude : 0
                    onTextChanged: if (areaPlanEditor) areaPlanEditor.setMissionAltitude(parseFloat(text) || 0)
                    Layout.fillWidth: true
                    placeholderText: "Altitude"
                }
            }
            
            // Movement controls
            Grid {
                columns: 3
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.2
                columnSpacing: ScreenTools.defaultFontPixelWidth * 0.2
                anchors.horizontalCenter: parent.horizontalCenter
                
                Item { width: ScreenTools.defaultFontPixelWidth * 4; height: ScreenTools.defaultFontPixelHeight * 1.5 }
                QGCButton {
                    text: qsTr("↑")
                    width: ScreenTools.defaultFontPixelWidth * 4
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.moveAreaNorth()
                }
                Item { width: ScreenTools.defaultFontPixelWidth * 4; height: ScreenTools.defaultFontPixelHeight * 1.5 }
                
                QGCButton {
                    text: qsTr("←")
                    width: ScreenTools.defaultFontPixelWidth * 4
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.moveAreaWest()
                }
                QGCButton {
                    text: qsTr("Center")
                    width: ScreenTools.defaultFontPixelWidth * 8
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.centerArea()
                }
                QGCButton {
                    text: qsTr("→")
                    width: ScreenTools.defaultFontPixelWidth * 4
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.moveAreaEast()
                }
                
                Item { width: ScreenTools.defaultFontPixelWidth * 4; height: ScreenTools.defaultFontPixelHeight * 1.5 }
                QGCButton {
                    text: qsTr("↓")
                    width: ScreenTools.defaultFontPixelWidth * 4
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.moveAreaSouth()
                }
                Item { width: ScreenTools.defaultFontPixelWidth * 4; height: ScreenTools.defaultFontPixelHeight * 1.5 }
            }
            
            // Rotation controls
            Row {
                spacing: ScreenTools.defaultFontPixelWidth * 0.2
                anchors.horizontalCenter: parent.horizontalCenter
                
                QGCButton {
                    text: qsTr("↶")
                    width: ScreenTools.defaultFontPixelWidth * 6
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.rotateAreaCounterClockwise()
                }
                
                QGCButton {
                    text: qsTr("0°")
                    width: ScreenTools.defaultFontPixelWidth * 6
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.setAreaRotation(0.0)
                }
                
                QGCButton {
                    text: qsTr("↷")
                    width: ScreenTools.defaultFontPixelWidth * 6
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    onClicked: areaPlanEditor.rotateAreaClockwise()
                }
            }
            
            // Action buttons
            QGCButton {
                text: qsTr("Add to Mission")
                width: parent.width
                height: ScreenTools.defaultFontPixelHeight * 2
                primary: true
                onClicked: areaPlanEditor.addWaypointsToMission()
            }
            
            QGCButton {
                text: qsTr("Save Mission")
                width: parent.width
                height: ScreenTools.defaultFontPixelHeight * 2
                onClicked: areaPlanEditor.saveMissionFile()
            }
        }
    }
}
