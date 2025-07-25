import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.FlightMap
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controllers
import QGroundControl.ShapeFileHelper
import QGroundControl.FlightDisplay
import QGroundControl.UTMSP
import QGroundControl.PlanView

// This is a minimal override that only adds the Area Planner to the existing PlanView
// We inherit from the original PlanView and only override the Area Planner section

Item {
    id: _root

    // Import the original PlanView
    PlanView {
        id: originalPlanView
        anchors.fill: parent
    }

    // Override the Area Planner section in the right panel
    // This will be shown when the Area Planner tab is selected
    Rectangle {
        id: areaPlannerPanel
        anchors.top: originalPlanView.rightPanel.top
        anchors.bottom: originalPlanView.rightPanel.bottom
        anchors.left: originalPlanView.rightPanel.left
        anchors.right: originalPlanView.rightPanel.right
        anchors.topMargin: originalPlanView._toolsMargin
        color: qgcPal.window
        visible: originalPlanView._editingLayer == originalPlanView._layerAreaPlanner

        QGCPalette { id: qgcPal }

        AreaPlannerPanel {
            anchors.fill: parent
            planMasterController: originalPlanView._planMasterController
            map: originalPlanView.editorMap
        }
    }
} 