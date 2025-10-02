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

GuidedToolStripAction {
    property var   activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle 
    property bool  _initialConnectComplete: activeVehicle ? activeVehicle.initialConnectComplete : false
    property bool  _isVehicleId1:           activeVehicle ? activeVehicle.id === 1 : false

    text:       "Explode"
    iconSource: "/res/PowerButton.svg"  // Using power button icon as explosion symbol
    visible:    _isVehicleId1 && _initialConnectComplete  // Only visible for Vehicle ID 1
    enabled:    true  // Always enabled when visible
    actionID:   _guidedController.actionExplode

    // Debug logging
    Component.onCompleted: {
        console.log("=== EXPLODE BUTTON CREATED ===")
        console.log("Vehicle ID:", activeVehicle ? activeVehicle.id : "No vehicle")
        console.log("Initial connect complete:", _initialConnectComplete)
        console.log("Is Vehicle ID 1:", _isVehicleId1)
        console.log("Visible:", visible)
        console.log("Action ID:", actionID)
        console.log("===============================")
    }

    onActiveVehicleChanged: {
        console.log("Explode button - Vehicle changed to ID:", activeVehicle ? activeVehicle.id : "No vehicle")
    }

    // Show/hide relay slider on button click
    onTriggered: {
        console.log("Explode button clicked - showing relay slider")
        
        // Trigger demining success dialog
        if (QGroundControl.missionService) {
            QGroundControl.missionService.triggerDeminingSuccess()
        }
    }
}
