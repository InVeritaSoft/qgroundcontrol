/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtLocation       5.15
import QtPositioning    5.15

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: root
    
    property var missionAreaPlanner: null
    property var map: null
    property bool selectingHomeOnMap: false
    signal homeLocationSelected(var coordinate)
    
    // Area boundary polygon
    MapPolygon {
        id: areaPolygon
        map: root.map
        color: Qt.rgba(0, 0.5, 1, 0.2)  // Semi-transparent blue
        border.color: Qt.rgba(0, 0.5, 1, 0.8)
        border.width: 2
        
        // Update polygon when bounds change
        Connections {
            target: missionAreaPlanner
            
            function onBoundsChanged() {
                if (missionAreaPlanner && missionAreaPlanner.bounds.isValid) {
                    var bounds = missionAreaPlanner.bounds
                    var path = []
                    path.push(QtPositioning.coordinate(bounds.topLeft.latitude, bounds.topLeft.longitude))
                    path.push(QtPositioning.coordinate(bounds.topLeft.latitude, bounds.bottomRight.longitude))
                    path.push(QtPositioning.coordinate(bounds.bottomRight.latitude, bounds.bottomRight.longitude))
                    path.push(QtPositioning.coordinate(bounds.bottomRight.latitude, bounds.topLeft.longitude))
                    areaPolygon.path = path
                }
            }
        }
    }
    
    // Grid lines
    Repeater {
        id: gridLinesRepeater
        model: missionAreaPlanner ? missionAreaPlanner.gridLines : []
        
        MapPolyline {
            map: root.map
            line.color: Qt.rgba(0.5, 0.5, 0.5, 0.6)  // Semi-transparent gray
            line.width: 1
            
            path: {
                var lineCoords = modelData
                var path = []
                for (var i = 0; i < lineCoords.length; i++) {
                    var coord = lineCoords[i]
                    path.push(QtPositioning.coordinate(coord.latitude, coord.longitude))
                }
                return path
            }
        }
    }
    
    // Mission points
    Repeater {
        id: missionPointsRepeater
        model: missionAreaPlanner ? missionAreaPlanner.gridPoints : []
        
        MapCircle {
            map: root.map
            center: modelData
            radius: 2  // 2 meter radius
            color: Qt.rgba(1, 0, 0, 0.8)  // Red
            border.color: Qt.rgba(1, 1, 1, 1)  // White border
            border.width: 1
        }
    }
    
    // Center point marker
    MapCircle {
        id: centerMarker
        map: root.map
        center: missionAreaPlanner ? missionAreaPlanner.center : QtPositioning.coordinate(0, 0)
        radius: 5  // 5 meter radius
        color: Qt.rgba(0, 1, 0, 0.8)  // Green
        border.color: Qt.rgba(1, 1, 1, 1)  // White border
        border.width: 2
        z: 10
        
        // Update center when it changes
        Connections {
            target: missionAreaPlanner
            function onCenterChanged() {
                if (missionAreaPlanner) {
                    centerMarker.center = missionAreaPlanner.center
                }
            }
        }
        
        // Drag-and-drop MouseArea overlay (disabled if selectingHomeOnMap)
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.OpenHandCursor
            drag.target: null // We'll handle drag manually
            property bool dragging: false
            enabled: !root.selectingHomeOnMap
            onPressed: dragging = true
            onReleased: dragging = false
            onPositionChanged: {
                if (dragging && root.map && missionAreaPlanner) {
                    var mousePoint = Qt.point(mouse.x, mouse.y)
                    var coord = root.map.toCoordinate(mousePoint)
                    missionAreaPlanner.center = coord
                }
            }
        }
    }
    
    // Corner handles for interactive resizing (future enhancement)
    // These would be draggable points at the corners of the rectangle
    // For now, they're just visual indicators
    
    // Top-left corner
    MapCircle {
        id: topLeftHandle
        map: root.map
        center: missionAreaPlanner && missionAreaPlanner.bounds.isValid ? 
                missionAreaPlanner.bounds.topLeft : QtPositioning.coordinate(0, 0)
        radius: 3
        color: Qt.rgba(1, 1, 0, 0.8)  // Yellow
        border.color: Qt.rgba(0, 0, 0, 1)  // Black border
        border.width: 1
        
        Connections {
            target: missionAreaPlanner
            
            function onBoundsChanged() {
                if (missionAreaPlanner && missionAreaPlanner.bounds.isValid) {
                    topLeftHandle.center = missionAreaPlanner.bounds.topLeft
                }
            }
        }
    }
    
    // Top-right corner
    MapCircle {
        id: topRightHandle
        map: root.map
        center: missionAreaPlanner && missionAreaPlanner.bounds.isValid ? 
                QtPositioning.coordinate(missionAreaPlanner.bounds.topLeft.latitude, 
                                       missionAreaPlanner.bounds.bottomRight.longitude) : 
                QtPositioning.coordinate(0, 0)
        radius: 3
        color: Qt.rgba(1, 1, 0, 0.8)  // Yellow
        border.color: Qt.rgba(0, 0, 0, 1)  // Black border
        border.width: 1
        
        Connections {
            target: missionAreaPlanner
            
            function onBoundsChanged() {
                if (missionAreaPlanner && missionAreaPlanner.bounds.isValid) {
                    topRightHandle.center = QtPositioning.coordinate(
                        missionAreaPlanner.bounds.topLeft.latitude,
                        missionAreaPlanner.bounds.bottomRight.longitude
                    )
                }
            }
        }
    }
    
    // Bottom-right corner
    MapCircle {
        id: bottomRightHandle
        map: root.map
        center: missionAreaPlanner && missionAreaPlanner.bounds.isValid ? 
                missionAreaPlanner.bounds.bottomRight : QtPositioning.coordinate(0, 0)
        radius: 3
        color: Qt.rgba(1, 1, 0, 0.8)  // Yellow
        border.color: Qt.rgba(0, 0, 0, 1)  // Black border
        border.width: 1
        
        Connections {
            target: missionAreaPlanner
            
            function onBoundsChanged() {
                if (missionAreaPlanner && missionAreaPlanner.bounds.isValid) {
                    bottomRightHandle.center = missionAreaPlanner.bounds.bottomRight
                }
            }
        }
    }
    
    // Bottom-left corner
    MapCircle {
        id: bottomLeftHandle
        map: root.map
        center: missionAreaPlanner && missionAreaPlanner.bounds.isValid ? 
                QtPositioning.coordinate(missionAreaPlanner.bounds.bottomRight.latitude, 
                                       missionAreaPlanner.bounds.topLeft.longitude) : 
                QtPositioning.coordinate(0, 0)
        radius: 3
        color: Qt.rgba(1, 1, 0, 0.8)  // Yellow
        border.color: Qt.rgba(0, 0, 0, 1)  // Black border
        border.width: 1
        
        Connections {
            target: missionAreaPlanner
            
            function onBoundsChanged() {
                if (missionAreaPlanner && missionAreaPlanner.bounds.isValid) {
                    bottomLeftHandle.center = QtPositioning.coordinate(
                        missionAreaPlanner.bounds.bottomRight.latitude,
                        missionAreaPlanner.bounds.topLeft.longitude
                    )
                }
            }
        }
    }

    // Overlay MouseArea for selecting home location
    MouseArea {
        anchors.fill: parent
        enabled: root.selectingHomeOnMap
        z: 100
        cursorShape: Qt.CrossCursor
        onClicked: {
            if (root.map) {
                var coord = root.map.toCoordinate(Qt.point(mouse.x, mouse.y))
                root.homeLocationSelected(coord)
            }
        }
    }
} 