/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FlightMap

/// AreaPlanMapVisuals provides map visualization for the AreaPlanEditor
Item {
    id: _root

    property var mapControl                                  ///< Map control to place item in
    property var areaPlanEditor                             ///< AreaPlanEditor object
    property bool interactive: true
    property color interiorColor: "#80FF0000"               ///< More visible red
    property color borderColor: "#FFFF0000"                 ///< Solid red border
    property int borderWidth: 5
    property real interiorOpacity: 0.7

    // Z-order management following QGC patterns
    property real _zorderRectangle:     QGroundControl.zOrderMapItems
    property real _zorderCenterMarker:  QGroundControl.zOrderMapItems + 1
    property real _zorderWaypoints:     QGroundControl.zOrderMapItems + 3

    // Interactive drawing properties
    property bool isDrawingMode: areaPlanEditor ? areaPlanEditor.isDrawingMode : false
    property bool showGridLines: true
    property bool showWaypoints: true



    // Object managers following QGC patterns
    QGCDynamicObjectManager { id: _objMgrRectangle }
    QGCDynamicObjectManager { id: _objMgrCenterMarker }
    QGCDynamicObjectManager { id: _objMgrGridLines }
    QGCDynamicObjectManager { id: _objMgrWaypointMarkers }

    // Calculate rectangle corners based on area parameters
    property var rectangleCorners: {
        if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.areaWidth || !areaPlanEditor.areaHeight) {
            console.log("AreaPlanMapVisuals: No areaPlanEditor or areaCenter")
            return []
        }
        
        var center = areaPlanEditor.areaCenter
        var width = areaPlanEditor.areaWidth
        var height = areaPlanEditor.areaHeight
        
        console.log("AreaPlanMapVisuals: Calculating corners - center:", center.latitude, center.longitude, "width:", width, "height:", height)
        console.log("AreaPlanMapVisuals: Center valid:", center.isValid)
        console.log("AreaPlanMapVisuals: Width > 0:", width > 0, "Height > 0:", height > 0)
        
        var corners = []
        
        try {
            // Validate inputs
            if (!center.isValid) {
                console.log("AreaPlanMapVisuals: ERROR - Invalid center coordinate")
                return []
            }
            
            if (width <= 0 || height <= 0) {
                console.log("AreaPlanMapVisuals: ERROR - Invalid dimensions:", width, "x", height)
                return []
            }
            
            // Calculate corners using geodesic calculations
            // North and South points (along the meridian)
            var northPoint = areaPlanEditor.calculateOffsetCoordinate(center, height/2, 0)  // North
            var southPoint = areaPlanEditor.calculateOffsetCoordinate(center, height/2, 180) // South
            
            console.log("AreaPlanMapVisuals: North point:", northPoint.latitude, northPoint.longitude)
            console.log("AreaPlanMapVisuals: South point:", southPoint.latitude, southPoint.longitude)
            
            // Top-left corner (North-West)
            var topLeft = areaPlanEditor.calculateOffsetCoordinate(northPoint, width/2, 270)
            corners.push(topLeft)
            
            // Top-right corner (North-East)
            var topRight = areaPlanEditor.calculateOffsetCoordinate(northPoint, width/2, 90)
            corners.push(topRight)
            
            // Bottom-right corner (South-East)
            var bottomRight = areaPlanEditor.calculateOffsetCoordinate(southPoint, width/2, 90)
            corners.push(bottomRight)
            
            // Bottom-left corner (South-West)
            var bottomLeft = areaPlanEditor.calculateOffsetCoordinate(southPoint, width/2, 270)
            corners.push(bottomLeft)
            
            console.log("AreaPlanMapVisuals: Calculated", corners.length, "corners")
            if (corners.length > 0) {
                console.log("First corner:", corners[0].latitude, corners[0].longitude)
                console.log("Last corner:", corners[corners.length-1].latitude, corners[corners.length-1].longitude)
                
                // Validate all corners
                for (var i = 0; i < corners.length; i++) {
                    if (!corners[i].isValid) {
                        console.log("AreaPlanMapVisuals: ERROR - Invalid corner at index", i)
                        return []
                    }
                }
                console.log("AreaPlanMapVisuals: All corners are valid")
            }
        } catch (e) {
            console.log("Error calculating rectangle corners:", e)
            return []
        }
        
        return corners
    }

    // Component for the area rectangle polygon
    Component {
        id: areaRectangleComponent

        MapPolygon {
            id: areaRectangle
            path: rectangleCorners
            color: interiorColor
            border.color: borderColor
            border.width: borderWidth
            opacity: interiorOpacity
            z: _zorderRectangle
            
            // Debug: Add console logging to track polygon updates
            onPathChanged: {
                console.log("AreaPlanMapVisuals: Polygon path changed, corners count:", path.length)
                if (path.length > 0) {
                    console.log("First corner:", path[0].latitude, path[0].longitude)
                }
            }
            
            // Smooth opacity transition for real-time feedback
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
            
            // MouseArea for dragging the rectangle
            MouseArea {
                id: rectangleMouseArea
                anchors.fill: parent
                enabled: interactive
                hoverEnabled: true
                
                property point startPos
                property var startCenter
                
                onPressed: {
                    console.log("Rectangle pressed")
                    startPos = Qt.point(mouse.x, mouse.y)
                    startCenter = areaPlanEditor ? areaPlanEditor.areaCenter : null
                }
                
                onPositionChanged: {
                    if (pressed && startCenter && areaPlanEditor) {
                        // Calculate offset from start position
                        var deltaX = mouse.x - startPos.x
                        var deltaY = mouse.y - startPos.y
                        
                        // Convert pixel offset to coordinate offset (approximate)
                        var latOffset = -deltaY * 0.00001  // Rough conversion
                        var lonOffset = deltaX * 0.00001   // Rough conversion
                        
                        var newLat = startCenter.latitude + latOffset
                        var newLon = startCenter.longitude + lonOffset
                        
                        var newCenter = QtPositioning.coordinate(newLat, newLon)
                        areaPlanEditor.setAreaCenter(newCenter)
                    }
                }
                
                onReleased: {
                    console.log("Rectangle released")
                }
                
                onEntered: {
                    parent.opacity = 0.9
                }
                
                onExited: {
                    parent.opacity = interiorOpacity
                }
            }
        }
    }

    // Component for the center marker
    Component {
        id: centerMarkerComponent

        MapQuickItem {
            id: centerMarker
            coordinate: areaPlanEditor ? areaPlanEditor.areaCenter : QtPositioning.coordinate()
            z: _zorderCenterMarker
            
            sourceItem: Rectangle {
                id: centerMarkerRect
                width: ScreenTools.defaultFontPixelHeight * 3.0
                height: width
                radius: width / 2
                color: "#FFFF0000"
                border.color: "#FFFFFFFF"
                border.width: 4
                
                // Add a cross inside to make it more visible
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.5
                    height: 4
                    color: "#FFFFFFFF"
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 4
                    height: parent.height * 0.5
                    color: "#FFFFFFFF"
                }
                
                // Smooth scale animation
                Behavior on scale {
                    NumberAnimation { duration: 150 }
                }
                
                // MouseArea for dragging the center marker
                MouseArea {
                    id: centerMouseArea
                    anchors.fill: parent
                    enabled: interactive
                    hoverEnabled: true
                    
                    property point startPos
                    property var startCenter
                    
                    onPressed: {
                        console.log("Center marker pressed")
                        startPos = Qt.point(mouse.x, mouse.y)
                        startCenter = areaPlanEditor ? areaPlanEditor.areaCenter : null
                    }
                    
                    onPositionChanged: {
                        if (pressed && startCenter && areaPlanEditor) {
                            // Calculate offset from start position
                            var deltaX = mouse.x - startPos.x
                            var deltaY = mouse.y - startPos.y
                            
                            // Convert pixel offset to coordinate offset (approximate)
                            var latOffset = -deltaY * 0.00001  // Rough conversion
                            var lonOffset = deltaX * 0.00001   // Rough conversion
                            
                            var newLat = startCenter.latitude + latOffset
                            var newLon = startCenter.longitude + lonOffset
                            
                            var newCenter = QtPositioning.coordinate(newLat, newLon)
                            areaPlanEditor.setAreaCenter(newCenter)
                        }
                    }
                    
                    onReleased: {
                        console.log("Center marker released")
                    }
                    
                    onEntered: {
                        parent.scale = 1.2
                    }
                    
                    onExited: {
                        parent.scale = 1.0
                    }
                }
            }
        }
    }

    // Component for grid lines
    Component {
        id: gridLineComponent

        MapPolyline {
            id: gridLine
            line.color: "#FF00FF00"  // Green
            line.width: 3
            z: QGroundControl.zOrderMapItems - 1
        }
    }

    // Component for waypoint markers
    Component {
        id: waypointMarkerComponent

        MapQuickItem {
            id: waypointMarker
            z: _zorderWaypoints
            
            sourceItem: Rectangle {
                width: ScreenTools.defaultFontPixelHeight * 1.5
                height: width
                radius: width / 2
                color: "#FF00FF00"  // Green
                border.color: "#FFFFFFFF"
                border.width: 3
                
                // Add a small dot in the center
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.4
                    height: width
                    radius: width / 2
                    color: "#FFFFFFFF"
                }
                
                // Smooth scale animation
                Behavior on scale {
                    NumberAnimation { duration: 150 }
                }
            }
        }
    }

    // Calculate waypoint positions based on area parameters
    property var waypointPositions: {
        if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.lineSpacing || !areaPlanEditor.numPoints) {
            return []
        }
        
        var center = areaPlanEditor.areaCenter
        var width = areaPlanEditor.areaWidth
        var height = areaPlanEditor.areaHeight
        var lineSpacing = areaPlanEditor.lineSpacing
        var numPoints = areaPlanEditor.numPoints
        
        var waypoints = []
        
        try {
            // Calculate number of lines based on height and spacing
            var numLines = Math.max(1, Math.floor(height / lineSpacing))
            console.log("AreaPlanMapVisuals: Creating waypoints for", numLines, "lines with", numPoints, "points per line")
            
            for (var i = 0; i < numLines; i++) {
                // Calculate offset from center for this line
                var offset = (-height/2) + (i + 0.5) * lineSpacing
                
                // Calculate line center point
                var lineCenter = areaPlanEditor.calculateOffsetCoordinate(center, offset, 180)
                
                // Calculate waypoints along this line
                for (var j = 0; j < numPoints; j++) {
                    // Calculate position along the line (0 to 1)
                    var fraction = (j + 0.5) / numPoints
                    
                    // Calculate offset from line center (-width/2 to width/2)
                    var pointOffset = (fraction - 0.5) * width
                    
                    // Calculate waypoint position
                    var waypoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, pointOffset, 90)
                    
                    waypoints.push({
                        coordinate: waypoint,
                        lineIndex: i,
                        pointIndex: j
                    })
                    
                    console.log("AreaPlanMapVisuals: Waypoint", i, "-", j, ":", waypoint.latitude, waypoint.longitude)
                }
            }
        } catch (e) {
            console.log("Error calculating waypoint positions:", e)
            return []
        }
        
        return waypoints
    }

    // Calculate grid lines based on area parameters
    property var gridLines: {
        if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.lineSpacing) {
            return []
        }
        
        var center = areaPlanEditor.areaCenter
        var width = areaPlanEditor.areaWidth
        var height = areaPlanEditor.areaHeight
        var lineSpacing = areaPlanEditor.lineSpacing
        
        var lines = []
        
        try {
            // Calculate number of lines based on height and spacing
            var numLines = Math.max(1, Math.floor(height / lineSpacing))
            console.log("AreaPlanMapVisuals: Creating", numLines, "grid lines")
            
            for (var i = 0; i < numLines; i++) {
                // Calculate offset from center for this line
                var offset = (-height/2) + (i + 0.5) * lineSpacing
                
                // Calculate line center point
                var lineCenter = areaPlanEditor.calculateOffsetCoordinate(center, offset, 180)
                
                // Calculate line endpoints (full width)
                var startPoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, width/2, 270)
                var endPoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, width/2, 90)
                
                lines.push({
                    start: startPoint,
                    end: endPoint
                })
                
                console.log("AreaPlanMapVisuals: Line", i, "- start:", startPoint.latitude, startPoint.longitude, "end:", endPoint.latitude, endPoint.longitude)
            }
        } catch (e) {
            console.log("Error calculating grid lines:", e)
            return []
        }
        
        return lines
    }

    // Functions to manage map items following QGC patterns
    function addMapItems() {
        console.log("AreaPlanMapVisuals: Adding map items")
        console.log("Rectangle corners count:", rectangleCorners.length)
        console.log("Area center valid:", areaPlanEditor ? areaPlanEditor.areaCenter.isValid : false)
        console.log("Area dimensions:", areaPlanEditor ? areaPlanEditor.areaWidth + "x" + areaPlanEditor.areaHeight : "null")
        
        // Clear existing items first to prevent duplication
        removeMapItems()
        
        // Only add items if we have valid data
        if (!areaPlanEditor || !areaPlanEditor.areaCenter.isValid || rectangleCorners.length < 3) {
            console.log("AreaPlanMapVisuals: Skipping map item creation - invalid data")
            console.log("  areaPlanEditor:", !!areaPlanEditor)
            console.log("  areaCenter valid:", areaPlanEditor ? areaPlanEditor.areaCenter.isValid : false)
            console.log("  rectangleCorners length:", rectangleCorners.length)
            return
        }
        
        // Create area rectangle
        var areaRect = _objMgrRectangle.createObject(areaRectangleComponent, mapControl, true)
        console.log("Area rectangle created:", !!areaRect)
        if (areaRect) {
            console.log("Area rectangle path length:", areaRect.path.length)
            console.log("Area rectangle visible:", areaRect.visible)
            console.log("Area rectangle opacity:", areaRect.opacity)
        }
        
        // Create center marker
        var centerMarker = _objMgrCenterMarker.createObject(centerMarkerComponent, mapControl, true)
        console.log("Center marker created:", !!centerMarker)
        if (centerMarker) {
            console.log("Center marker coordinate:", centerMarker.coordinate.latitude, centerMarker.coordinate.longitude)
            console.log("Center marker visible:", centerMarker.visible)
        }
        
        // Add grid lines
        addGridLines()
        
        // Add waypoint markers
        addWaypointMarkers()
        console.log("AreaPlanMapVisuals: Map items added successfully")
    }
    
    function addGridLines() {
        // Remove existing grid lines first
        removeGridLines()
        
        // Create new grid lines based on current parameters
        for (var i = 0; i < gridLines.length; i++) {
            var lineData = gridLines[i]
            var gridLine = _objMgrGridLines.createObject(gridLineComponent, mapControl, true)
            if (gridLine) {
                gridLine.path = [lineData.start, lineData.end]
            }
        }
    }
    
    function removeGridLines() {
        // Remove all grid line objects
        _objMgrGridLines.destroyObjects()
    }
    
    function addWaypointMarkers() {
        // Remove existing waypoint markers first
        removeWaypointMarkers()
        
        // Create new waypoint markers based on current parameters
        for (var i = 0; i < waypointPositions.length; i++) {
            var waypointData = waypointPositions[i]
            var waypointMarker = _objMgrWaypointMarkers.createObject(waypointMarkerComponent, mapControl, true)
            if (waypointMarker) {
                waypointMarker.coordinate = waypointData.coordinate
            }
        }
    }
    
    function removeWaypointMarkers() {
        // Remove all waypoint marker objects
        _objMgrWaypointMarkers.destroyObjects()
    }

    function removeMapItems() {
        console.log("AreaPlanMapVisuals: Removing map items")
        _objMgrRectangle.destroyObjects()
        _objMgrCenterMarker.destroyObjects()
        _objMgrGridLines.destroyObjects()
        _objMgrWaypointMarkers.destroyObjects()
        console.log("AreaPlanMapVisuals: Map items removed successfully")
    }

    // Monitor area property changes and trigger map updates
    Connections {
        target: areaPlanEditor
        
        function onAreaWidthChanged() {
            console.log("AreaPlanMapVisuals: Area width changed, updating map items")
            addMapItems()
        }
        
        function onAreaHeightChanged() {
            console.log("AreaPlanMapVisuals: Area height changed, updating map items")
            addMapItems()
        }
        
        function onAreaCenterChanged() {
            console.log("AreaPlanMapVisuals: Area center changed, updating map items")
            addMapItems()
        }
        
        function onLineSpacingChanged() {
            console.log("AreaPlanMapVisuals: Line spacing changed, updating map items")
            addGridLines()
            addWaypointMarkers()
        }
        
        function onNumPointsChanged() {
            console.log("AreaPlanMapVisuals: Number of points changed, updating map items")
            addWaypointMarkers()
        }
    }

    // Monitor drawing mode changes
    onIsDrawingModeChanged: {
        console.log("AreaPlanMapVisuals: isDrawingMode changed to:", isDrawingMode)
    }
    
    // Monitor C++ backend drawing mode changes
    Connections {
        target: areaPlanEditor
        
        function onIsDrawingModeChanged() {
            console.log("C++ backend isDrawingMode changed to:", areaPlanEditor.isDrawingMode)
        }
    }

    // Initialize map items when component is ready
    Component.onCompleted: {
        console.log("AreaPlanMapVisuals: Component completed")
        console.log("Interactive:", interactive)
        console.log("MapControl valid:", !!mapControl)
        console.log("AreaPlanEditor valid:", !!areaPlanEditor)
        console.log("IsDrawingMode:", isDrawingMode)
        
        // Force initial map item creation
        if (mapControl && areaPlanEditor) {
            console.log("AreaPlanMapVisuals: Forcing initial map item creation")
            addMapItems()
        } else {
            console.log("AreaPlanMapVisuals: Skipping initial map item creation - missing dependencies")
            console.log("  mapControl:", !!mapControl)
            console.log("  areaPlanEditor:", !!areaPlanEditor)
        }
    }

    Component.onDestruction: {
        removeMapItems()
    }
} 