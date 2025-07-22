import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

// Area Planner Visual Components (cloned from QGCMapPolygonVisuals patterns)
Item {
    id: _root
    
    property var map: null
    property var areaPlannerPanel: null
    property bool visible: false
    
    // Rectangle polygon (cloned from MapPolygon pattern)
    MapPolygon {
        id: areaPlannerRectangle
        map: _root.map
        visible: _root.visible && areaPlannerPanel && areaPlannerPanel.drawingMode && areaPlannerPanel.rectangleStart && areaPlannerPanel.rectangleEnd
        color: "transparent"
        border.color: qgcPal.colorGreen
        border.width: 3
        z: QGroundControl.zOrderWaypointLines + 3
        
        path: {
            if (!areaPlannerPanel || !areaPlannerPanel.rectangleStart || !areaPlannerPanel.rectangleEnd) {
                return []
            }
            
            var start = areaPlannerPanel.rectangleStart
            var end = areaPlannerPanel.rectangleEnd
            
            // Create rectangle path from start and end points
            var path = []
            path.push(start)
            path.push(QtPositioning.coordinate(start.latitude, end.longitude))
            path.push(end)
            path.push(QtPositioning.coordinate(end.latitude, start.longitude))
            path.push(start)
            
            return path
        }
    }
    
    // Rectangle start marker (cloned from dragHandleComponent pattern)
    MapQuickItem {
        id: areaPlannerStartMarker
        map: _root.map
        visible: _root.visible && areaPlannerPanel && areaPlannerPanel.drawingMode && areaPlannerPanel.rectangleStart
        coordinate: areaPlannerPanel ? areaPlannerPanel.rectangleStart : QtPositioning.coordinate(0, 0)
        z: QGroundControl.zOrderWaypointLines + 4
        
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        
        sourceItem: Rectangle {
            width: ScreenTools.defaultFontPixelHeight * 1.5
            height: width
            radius: width * 0.5
            color: qgcPal.colorGreen
            border.color: "white"
            border.width: 2
        }
    }
    
    // Rectangle end marker (cloned from dragHandleComponent pattern)
    MapQuickItem {
        id: areaPlannerEndMarker
        map: _root.map
        visible: _root.visible && areaPlannerPanel && areaPlannerPanel.drawingMode && areaPlannerPanel.rectangleEnd
        coordinate: areaPlannerPanel ? areaPlannerPanel.rectangleEnd : QtPositioning.coordinate(0, 0)
        z: QGroundControl.zOrderWaypointLines + 4
        
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        
        sourceItem: Rectangle {
            width: ScreenTools.defaultFontPixelHeight * 1.5
            height: width
            radius: width * 0.5
            color: qgcPal.colorOrange
            border.color: "white"
            border.width: 2
        }
    }
    
    // Area Planner Center Marker (when not in drawing mode, cloned from centerDragHandle pattern)
    MapQuickItem {
        id: areaPlannerCenterMarker
        map: _root.map
        visible: _root.visible && areaPlannerPanel && areaPlannerPanel.centerSet && !areaPlannerPanel.drawingMode
        coordinate: areaPlannerPanel ? areaPlannerPanel.areaCenter : QtPositioning.coordinate(0, 0)
        z: QGroundControl.zOrderWaypointLines + 4
        
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        
        sourceItem: Rectangle {
            width: ScreenTools.defaultFontPixelHeight * 1.2
            height: width
            radius: width * 0.5
            color: qgcPal.colorBlue
            border.color: "white"
            border.width: 2
        }
    }
} 