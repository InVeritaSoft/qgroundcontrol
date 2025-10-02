import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id: root
    
    property var mapControl
    property var areaPlanEditor
    property bool interactive: true
    property bool isDragging: false
    
    // Visual properties
    property color interiorColor: "#303030"
    property color borderColor: "#2196F3"
    property real borderWidth: 2
    property real interiorOpacity: 0.7
    property bool showGridLines: true
    property bool showWaypoints: true
    
    // Mouse area for interactive drawing
    MouseArea {
        id: mapAreaMouseArea
        anchors.fill: parent
        enabled: interactive && areaPlanEditor && areaPlanEditor.isDrawingMode
        acceptedButtons: Qt.LeftButton
        
        property point startPos
        property var startCenter
        property var startCoordinate
        property bool hasMoved: false
        
        onPressed: function(mouse) {
            if (mapControl && areaPlanEditor) {
                // Get click coordinate
                var p = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(p.x, p.y), false)
                
                if (clickCoordinate.isValid) {
                    // Set area center on first click
                    if (!areaPlanEditor.areaCenter.isValid ||
                        areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
                        
                        areaPlanEditor.setAreaCenter(clickCoordinate)
                        
                        // Set default area size if not already set
                        if (areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
                            areaPlanEditor.setAreaWidth(10.0)
                            areaPlanEditor.setAreaHeight(10.0)
                        }
                    }
                }
                
                // Store start position for potential dragging
                startPos = Qt.point(mouse.x, mouse.y)
                startCenter = areaPlanEditor.areaCenter
                startCoordinate = clickCoordinate
                isDragging = true
                hasMoved = false
            }
        }
        
        onPositionChanged: function(mouse) {
            if (pressed && areaPlanEditor && mapControl) {
                hasMoved = true
                // Get current mouse position coordinate
                var mapped = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var currentCoordinate = mapControl.toCoordinate(Qt.point(mapped.x, mapped.y), false)
                
                if (currentCoordinate.isValid) {
                    // Move center directly to current mouse position
                    areaPlanEditor.setAreaCenter(currentCoordinate)
                }
            }
        }
        
        onReleased: function(mouse) {
            isDragging = false
            
            // If we didn't move, treat as a click
            if (!hasMoved && areaPlanEditor && mapControl) {
                var mappedClick = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(mappedClick.x, mappedClick.y), false)
                
                if (clickCoordinate.isValid) {
                    // Set center point on first click or if center is not valid
                    if (!areaPlanEditor.areaCenter.isValid ||
                        areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
                        
                        areaPlanEditor.setAreaCenter(clickCoordinate)
                    }
                }
            }
        }
    }
    
    // Area visualization
    Repeater {
        model: areaPlanEditor && areaPlanEditor.isDrawingMode && areaPlanEditor.areaCenter.isValid ? 1 : 0
        
        MapPolygon {
            id: areaPolygon
            color: interiorColor
            border.color: borderColor
            border.width: borderWidth
            opacity: interiorOpacity
            
            // Calculate polygon vertices based on area parameters
            function updateVertices() {
                if (!areaPlanEditor || !areaPlanEditor.areaCenter.isValid) {
                    return
                }
                
                var center = areaPlanEditor.areaCenter
                var width = areaPlanEditor.areaWidth
                var height = areaPlanEditor.areaHeight
                var rotation = areaPlanEditor.areaRotation
                
                // Calculate half dimensions
                var halfWidth = width * 0.5
                var halfHeight = height * 0.5
                
                // Convert rotation to radians
                var theta = -rotation * Math.PI / 180.0
                var cosTheta = Math.cos(theta)
                var sinTheta = Math.sin(theta)
                
                // Define rectangle corners in local coordinates
                var corners = [
                    Qt.point(-halfWidth, -halfHeight),
                    Qt.point(halfWidth, -halfHeight),
                    Qt.point(halfWidth, halfHeight),
                    Qt.point(-halfWidth, halfHeight)
                ]
                
                // Apply rotation and convert to geographic coordinates
                var vertices = []
                for (var i = 0; i < corners.length; i++) {
                    var corner = corners[i]
                    var rotatedX = corner.x * cosTheta - corner.y * sinTheta
                    var rotatedY = corner.x * sinTheta + corner.y * cosTheta
                    
                    // Convert to geographic coordinate
                    var latOffset = rotatedY / 111000.0 // Approximate meters to degrees
                    var lonOffset = rotatedX / (111000.0 * Math.cos(center.latitude() * Math.PI / 180.0))
                    
                    var vertex = QtPositioning.coordinate(
                        center.latitude() + latOffset,
                        center.longitude() + lonOffset,
                        center.altitude()
                    )
                    vertices.push(vertex)
                }
                
                areaPolygon.path = vertices
            }
            
            Component.onCompleted: updateVertices()
            
            Connections {
                target: areaPlanEditor
                function onAreaCenterChanged() { updateVertices() }
                function onAreaWidthChanged() { updateVertices() }
                function onAreaHeightChanged() { updateVertices() }
                function onAreaRotationChanged() { updateVertices() }
            }
        }
    }
    
    // Grid lines visualization
    Repeater {
        model: showGridLines && areaPlanEditor && areaPlanEditor.isDrawingMode && areaPlanEditor.areaCenter.isValid ? 
               Math.floor(areaPlanEditor.areaHeight / areaPlanEditor.lineSpacing) : 0
        
        MapPolyline {
            id: gridLine
            line.color: borderColor
            line.width: 1
            opacity: 0.5
            
            function updateLine() {
                if (!areaPlanEditor || !areaPlanEditor.areaCenter.isValid) {
                    return
                }
                
                var center = areaPlanEditor.areaCenter
                var width = areaPlanEditor.areaWidth
                var height = areaPlanEditor.areaHeight
                var rotation = areaPlanEditor.areaRotation
                var lineSpacing = areaPlanEditor.lineSpacing
                var numPoints = areaPlanEditor.numPoints
                
                // Calculate line position
                var halfHeight = height * 0.5
                var lineOffset = (index * lineSpacing) - halfHeight
                
                // Convert rotation to radians
                var theta = -rotation * Math.PI / 180.0
                var cosTheta = Math.cos(theta)
                var sinTheta = Math.sin(theta)
                
                // Generate points along this line
                var points = []
                for (var pointIndex = 0; pointIndex < numPoints; pointIndex++) {
                    var pointOffset = (pointIndex * (width / Math.max(1, numPoints - 1))) - (width * 0.5)
                    
                    // Apply rotation transformation
                    var rotatedX = pointOffset * cosTheta - lineOffset * sinTheta
                    var rotatedY = pointOffset * sinTheta + lineOffset * cosTheta
                    
                    // Convert to geographic coordinate
                    var latOffset = rotatedY / 111000.0
                    var lonOffset = rotatedX / (111000.0 * Math.cos(center.latitude() * Math.PI / 180.0))
                    
                    var point = QtPositioning.coordinate(
                        center.latitude() + latOffset,
                        center.longitude() + lonOffset,
                        center.altitude()
                    )
                    points.push(point)
                }
                
                gridLine.path = points
            }
            
            Component.onCompleted: updateLine()
            
            Connections {
                target: areaPlanEditor
                function onAreaCenterChanged() { updateLine() }
                function onAreaWidthChanged() { updateLine() }
                function onAreaHeightChanged() { updateLine() }
                function onAreaRotationChanged() { updateLine() }
                function onLineSpacingChanged() { updateLine() }
                function onNumPointsChanged() { updateLine() }
            }
        }
    }
    
    // Waypoint markers
    Repeater {
        model: showWaypoints && areaPlanEditor && areaPlanEditor.isDrawingMode ? 
               areaPlanEditor.generateWaypoints() : []
        
        MapQuickItem {
            id: waypointMarker
            coordinate: modelData
            anchorPoint.x: waypointCircle.width / 2
            anchorPoint.y: waypointCircle.height / 2
            z: 10
            
            sourceItem: Rectangle {
                id: waypointCircle
                width: 8
                height: 8
                radius: 4
                color: "#FF6B6B"
                border.color: "#FFFFFF"
                border.width: 2
            }
        }
    }
}
