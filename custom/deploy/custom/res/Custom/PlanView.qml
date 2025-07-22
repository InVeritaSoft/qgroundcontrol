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
import "MissionAreaPlannerPanel.qml" as MissionAreaPlannerPanel

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
        
        // Mission Area Planner Panel (custom)
        MissionAreaPlannerPanel {
            planMasterController: _planMasterController
            map: editorMap
            visible: _editingLayer == _layerMission
        }
        
        // Existing QGCTabBar for layers
        QGCTabBar {
            id:         layerTabBar
            width:      parent.width
            visible:    QGroundControl.corePlugin.options.enablePlanViewSelector  && !_utmspEnabled
            Component.onCompleted: currentIndex = 0
            QGCTabButton {
                text:       qsTr("Mission")
            }
            QGCTabButton {
                text:       qsTr("Fence")
                enabled:    _geoFenceController.supported
            }
            QGCTabButton {
                text:       qsTr("Rally")
                enabled:    _rallyPointController.supported
            }
        }

        QGCTabBar {
            id:         layerTabBarUTMSP
            width:      parent.width
            visible:    QGroundControl.corePlugin.options.enablePlanViewSelector && _utmspEnabled
            QGCTabButton {
                text:       qsTr("Mission")
            }
            QGCTabButton {
                text:       qsTr("Rally")
                enabled:    _rallyPointController.supported
            }
            QGCTabButton {
                id: utmspbutton
                text:       qsTr("UTM-Adapter")
                visible: _utmspEnabled
            }
        }
    }
    
    // Mission Item Editor
    Item {
        id:                     missionItemEditor
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.top:            rightControls.bottom
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
        visible:                _editingLayer == _layerMission && !planControlColapsed
        QGCListView {
            id:                 missionItemEditorListView
            anchors.fill:       parent
            spacing:            ScreenTools.defaultFontPixelHeight / 4
            orientation:        ListView.Vertical
            model:              _missionController.visualItems
            cacheBuffer:        Math.max(height * 2, 0)
            clip:               true
            currentIndex:       _missionController.currentPlanViewSeqNum
            highlightMoveDuration: 250
            visible:            _editingLayer == _layerMission && !planControlColapsed
            delegate: VisualMissionItemEditor {
                missionItem:    _visualItems.get(index)
                readOnly:       false
                rootQgcView:    _root
            }
        }
    }
    
    // GeoFence Editor
    Item {
        id:                     geoFenceEditor
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.top:            rightControls.bottom
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
        visible:                _editingLayer == _layerGeoFence && !planControlColapsed
        GeoFenceEditor {
            anchors.fill:       parent
            geoFenceController: _geoFenceController
            readOnly:           false
            rootQgcView:        _root
        }
    }
    
    // Rally Point Editor
    Item {
        id:                     rallyPointEditor
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.top:            rightControls.bottom
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
        visible:                _editingLayer == _layerRallyPoints && !planControlColapsed
        RallyPointEditor {
            anchors.fill:       parent
            rallyPointController: _rallyPointController
            readOnly:           false
            rootQgcView:        _root
        }
    }
    
    // UTMSP Editor
    Item {
        id:                     utmspEditor
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.top:            rightControls.bottom
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
        visible:                _editingLayer == _layerUTMSP && !planControlColapsed && _utmspEnabled
        UTMSPEditor {
            anchors.fill:       parent
            readOnly:           false
            rootQgcView:        _root
        }
    }
}
