import QtQuick 2.15
import QtQuick.Controls 2.15

import QGroundControl 1.0
import QGroundControl.Controls 1.0

QGCButton {
    text: "Test Collision Detection"
    
    onClicked: {
        // Test collision alert
        QGroundControl.collisionDetectionService.showCollisionAlert("TEST: Collision detected between Vehicle 1 and Vehicle 2 - Proximity Collision")
        
        // Start monitoring (if not already started)
        QGroundControl.collisionDetectionService.startCollisionMonitoring()
    }
}
