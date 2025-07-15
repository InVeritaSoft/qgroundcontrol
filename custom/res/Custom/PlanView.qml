// Resource override for QGroundControl PlanView.qml to add MissionAreaPlannerPanel
// Only the right panel controls section is overridden for custom integration
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.PlanView 1.0
import QGroundControl.FlightMap 1.0
import QGroundControl.Controllers 1.0
import QtPositioning 5.15

// Right Panel Controls (override)
Item {
    anchors.fill:           rightPanel
    anchors.topMargin:      _toolsMargin
    DeadMouseArea {
        anchors.fill:   parent
    }
    Column {
        id:                 rightControls
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        // ... existing QGCTabBar, etc ...
        // Mission Area Planner Panel (custom)
        import "MissionAreaPlannerPanel.qml" as MissionAreaPlannerPanel
        MissionAreaPlannerPanel {
            planMasterController: _planMasterController
            map: editorMap
            visible: _editingLayer == _layerMission
        }
        // ... rest of right panel controls ...
    }
    // ... rest of right panel ...
} 