/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QGroundControl 1.0

Dialog {
    id: deminingSuccessDialogue
    
    title: "Demining Operation Complete"
    width: 400
    height: 200
    modal: true
    
    property bool _showing: false
    
    onOpened: {
        _showing = true
        console.log("Demining success dialogue opened")
    }
    
    onClosed: {
        _showing = false
        console.log("Demining success dialogue closed")
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#2c2c2c"
        radius: 8
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            
            // Success icon
            Rectangle {
                width: 60
                height: 60
                radius: 30
                color: "#4CAF50"
                anchors.horizontalCenter: parent.horizontalCenter
                
                Text {
                    text: "✓"
                    color: "white"
                    font.pixelSize: 30
                    font.bold: true
                    anchors.centerIn: parent
                }
            }
            
            // Success message
            Text {
                text: "Demining successful"
                color: "white"
                font.pixelSize: 18
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // Description
            Text {
                text: "All tripods have been successfully installed and the demining operation has been completed."
                color: "#cccccc"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: 350
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // OK button
            Button {
                text: "OK"
                width: 100
                height: 40
                anchors.horizontalCenter: parent.horizontalCenter
                
                background: Rectangle {
                    color: parent.pressed ? "#1976D2" : "#2196F3"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    deminingSuccessDialogue.close()
                }
            }
        }
    }
}
