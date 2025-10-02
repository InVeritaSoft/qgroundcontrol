/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.FlightDisplay
import QGroundControl
import QGroundControl.Controls
import QtQuick.Controls

Item {
    property var   activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle 
    property bool  _initialConnectComplete: activeVehicle ? activeVehicle.initialConnectComplete : false
    property bool  _isVehicleArmed:         _initialConnectComplete ? activeVehicle.armed : false
    property bool  _isVehicleId1:           activeVehicle ? activeVehicle.id === 1 : false
    property bool  _relayState:             false
    property bool  _explodeButtonEnabled:   false
    property var   _missionService:         QGroundControl.missionService
    
    signal triggered()

    // Debug logging
    Component.onCompleted: {
        console.log("=== EXPLODE BUTTON CREATED ===")
        console.log("Vehicle ID:", activeVehicle ? activeVehicle.id : "No vehicle")
        console.log("Initial connect complete:", _initialConnectComplete)
        console.log("Is Vehicle ID 1:", _isVehicleId1)
        console.log("Enabled:", enabled)
        console.log("Visible:", visible)
        console.log("Action ID:", actionID)
        console.log("Mission Service:", _missionService)
        console.log("===============================")
        
        // Connect to mission service signals
        if (_missionService) {
            _missionService.explodeButtonEnabled.connect(function(enabled) {
                _explodeButtonEnabled = enabled
                console.log("Explode button enabled:", enabled)
            })
            
            _missionService.deminingSuccess.connect(function() {
                console.log("Demining successful!")
                // Show success dialogue
                showDeminingSuccessDialogue()
            })
        }
    }

    onActiveVehicleChanged: {
        console.log("Explode button - Vehicle changed to ID:", activeVehicle ? activeVehicle.id : "No vehicle")
    }
    
    // Function to show demining success dialogue
    function showDeminingSuccessDialogue() {
        // Create and show success dialogue
        var component = Qt.createComponent("DeminingSuccessDialogue.qml")
        if (component.status === Component.Ready) {
            var dialogue = component.createObject(parent)
            if (dialogue) {
                dialogue.open()
            }
        }
    }

    // Main button
    GuidedToolStripAction {
        id: explodeButton
        text: "Explode"
        iconSource: "/res/PowerButton.svg"
        visible: true
        enabled: _explodeButtonEnabled && _isVehicleId1
        actionID: _guidedController.actionExplode
        
        onTriggered: {
            if (_explodeButtonEnabled && _missionService) {
                console.log("Executing explode command via mission service")
                _missionService.executeExplode()
            } else {
                relaySlider.visible = !relaySlider.visible
                console.log("Relay slider visible:", relaySlider.visible)
            }
            parent.triggered()
        }
    }

    // Relay slider - only visible when button is pressed
    QGCSwitch {
        id: relaySlider
        anchors.top: explodeButton.bottom
        anchors.horizontalCenter: explodeButton.horizontalCenter
        anchors.topMargin: 5
        text: "Relay1"
        checked: _relayState
        visible: false
        
        onToggled: {
            if (activeVehicle) {
                _relayState = checked
                console.log("Relay1:", checked ? "HIGH" : "LOW")
                
                // Send relay command
                activeVehicle.sendCommand(
                    1, // Component ID
                    181, // MAV_CMD_DO_SET_RELAY
                    true, // showError
                    0, // param1: Relay number (0 for Relay1)
                    checked ? 1.0 : 0.0, // param2: Value (1=HIGH, 0=LOW)
                    0, 0, 0, 0, 0 // param3-7: unused
                )
            }
        }
    }
}
