/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.15
import QtPositioning 5.15

import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.PlanView 1.0
import QGroundControl.FlightMap 1.0
import QGroundControl.Controllers 1.0

QtObject {
    id: missionAreaPlanner
    
    property var planMasterController
    property var map
    property var areaCenter: QtPositioning.coordinate(0, 0)
    property real areaWidth: 100.0
    property real areaHeight: 100.0
    property real lineSpacing: 10.0
    property int pointsPerLine: 10
    
    // Status properties
    property string status: qsTr("Ready")
    property color statusColor: qgcPal.colorGreen
    property bool busy: false
    
    // Generated waypoints
    property var waypoints: []
    property var gridLines: []
    
    // Signals
    signal statusChanged(string status, color statusColor)
    signal areaUpdated()
    signal gridUpdated()
    signal missionGenerated()
    
    // Geodesic calculation constants
    readonly property real earthRadius: 6371000.0 // meters
    readonly property real pi: Math.PI
    
    // Update area when parameters change
    onAreaCenterChanged: updateArea()
    onAreaWidthChanged: updateArea()
    onAreaHeightChanged: updateArea()
    onLineSpacingChanged: updateGrid()
    onPointsPerLineChanged: updateGrid()
    
    // Calculate geodesic distance between two coordinates
    function geodesicDistance(coord1, coord2) {
        var lat1 = coord1.latitude * pi / 180.0
        var lon1 = coord1.longitude * pi / 180.0
        var lat2 = coord2.latitude * pi / 180.0
        var lon2 = coord2.longitude * pi / 180.0
        
        var dLat = lat2 - lat1
        var dLon = lon2 - lon1
        
        var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
                Math.cos(lat1) * Math.cos(lat2) *
                Math.sin(dLon/2) * Math.sin(dLon/2)
        var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a))
        
        return earthRadius * c
    }
    
    // Calculate coordinate at distance and bearing from reference point
    function coordinateAtDistance(referenceCoord, distance, bearing) {
        var lat1 = referenceCoord.latitude * pi / 180.0
        var lon1 = referenceCoord.longitude * pi / 180.0
        var brng = bearing * pi / 180.0
        
        var angularDistance = distance / earthRadius
        
        var lat2 = Math.asin(Math.sin(lat1) * Math.cos(angularDistance) +
                           Math.cos(lat1) * Math.sin(angularDistance) * Math.cos(brng))
        
        var lon2 = lon1 + Math.atan2(Math.sin(brng) * Math.sin(angularDistance) * Math.cos(lat1),
                                   Math.cos(angularDistance) - Math.sin(lat1) * Math.sin(lat2))
        
        return QtPositioning.coordinate(lat2 * 180.0 / pi, lon2 * 180.0 / pi)
    }
    
    // Calculate bearing between two coordinates
    function calculateBearing(coord1, coord2) {
        var lat1 = coord1.latitude * pi / 180.0
        var lon1 = coord1.longitude * pi / 180.0
        var lat2 = coord2.latitude * pi / 180.0
        var lon2 = coord2.longitude * pi / 180.0
        
        var dLon = lon2 - lon1
        
        var y = Math.sin(dLon) * Math.cos(lat2)
        var x = Math.cos(lat1) * Math.sin(lat2) - Math.sin(lat1) * Math.cos(lat2) * Math.cos(dLon)
        
        var bearing = Math.atan2(y, x) * 180.0 / pi
        return (bearing + 360) % 360
    }
    
    // Update area calculations
    function updateArea() {
        if (!areaCenter || !areaCenter.isValid) {
            setStatus(qsTr("Invalid area center"), qgcPal.colorRed)
            return
        }
        
        setStatus(qsTr("Updating area..."), qgcPal.colorOrange)
        busy = true
        
        // Calculate area corners
        var halfWidth = areaWidth / 2.0
        var halfHeight = areaHeight / 2.0
        
        // Calculate corners using geodesic calculations
        var northWest = coordinateAtDistance(areaCenter, halfWidth, 270)
        northWest = coordinateAtDistance(northWest, halfHeight, 0)
        
        var northEast = coordinateAtDistance(areaCenter, halfWidth, 90)
        northEast = coordinateAtDistance(northEast, halfHeight, 0)
        
        var southWest = coordinateAtDistance(areaCenter, halfWidth, 270)
        southWest = coordinateAtDistance(southWest, halfHeight, 180)
        
        var southEast = coordinateAtDistance(areaCenter, halfWidth, 90)
        southEast = coordinateAtDistance(southEast, halfHeight, 180)
        
        // Store area corners for map overlay
        missionAreaPlanner.areaCorners = [northWest, northEast, southEast, southWest]
        
        setStatus(qsTr("Area updated"), qgcPal.colorGreen)
        busy = false
        areaUpdated()
        
        // Update grid after area update
        updateGrid()
    }
    
    // Update grid calculations
    function updateGrid() {
        if (!areaCenter || !areaCenter.isValid || !missionAreaPlanner.areaCorners) {
            return
        }
        
        setStatus(qsTr("Generating grid..."), qgcPal.colorOrange)
        busy = true
        
        var corners = missionAreaPlanner.areaCorners
        var gridLines = []
        var waypoints = []
        
        // Calculate grid lines parallel to width (North-South lines)
        var numLines = Math.ceil(areaHeight / lineSpacing) + 1
        var lineSpacingActual = areaHeight / (numLines - 1)
        
        for (var i = 0; i < numLines; i++) {
            var progress = i / (numLines - 1)
            var startCoord = interpolateCoordinate(corners[0], corners[3], progress) // NW to SW
            var endCoord = interpolateCoordinate(corners[1], corners[2], progress)   // NE to SE
            
            var line = {
                start: startCoord,
                end: endCoord,
                points: []
            }
            
            // Generate points along this line
            var numPoints = pointsPerLine
            for (var j = 0; j < numPoints; j++) {
                var pointProgress = j / (numPoints - 1)
                var pointCoord = interpolateCoordinate(startCoord, endCoord, pointProgress)
                
                line.points.push(pointCoord)
                waypoints.push(pointCoord)
            }
            
            gridLines.push(line)
        }
        
        missionAreaPlanner.gridLines = gridLines
        missionAreaPlanner.waypoints = waypoints
        
        setStatus(qsTr("Grid generated: " + waypoints.length + " waypoints"), qgcPal.colorGreen)
        busy = false
        gridUpdated()
    }
    
    // Interpolate between two coordinates
    function interpolateCoordinate(coord1, coord2, progress) {
        var lat = coord1.latitude + (coord2.latitude - coord1.latitude) * progress
        var lon = coord1.longitude + (coord2.longitude - coord1.longitude) * progress
        return QtPositioning.coordinate(lat, lon)
    }
    
    // Move area by specified distance in meters
    function moveArea(deltaX, deltaY) {
        if (!areaCenter || !areaCenter.isValid) {
            return
        }
        
        // Calculate new center position
        var bearingX = deltaX >= 0 ? 90 : 270
        var bearingY = deltaY >= 0 ? 180 : 0
        
        var newCenter = areaCenter
        
        if (Math.abs(deltaX) > 0) {
            newCenter = coordinateAtDistance(newCenter, Math.abs(deltaX), bearingX)
        }
        
        if (Math.abs(deltaY) > 0) {
            newCenter = coordinateAtDistance(newCenter, Math.abs(deltaY), bearingY)
        }
        
        areaCenter = newCenter
    }
    
    // Generate mission from current grid
    function generateMission() {
        if (!planMasterController || !planMasterController.missionController) {
            setStatus(qsTr("No mission controller available"), qgcPal.colorRed)
            return
        }
        
        if (!waypoints || waypoints.length === 0) {
            setStatus(qsTr("No waypoints to generate"), qgcPal.colorRed)
            return
        }
        
        setStatus(qsTr("Generating mission..."), qgcPal.colorOrange)
        busy = true
        
        try {
            var missionController = planMasterController.missionController
            
            // Clear existing mission items (except home)
            var currentItems = missionController.visualItems
            while (currentItems.count > 1) {
                missionController.removeVisualItem(1)
            }
            
            // Add waypoints to mission
            for (var i = 0; i < waypoints.length; i++) {
                var waypoint = waypoints[i]
                missionController.insertSimpleMissionItem(waypoint, i + 1, false)
            }
            
            setStatus(qsTr("Mission generated: " + waypoints.length + " waypoints"), qgcPal.colorGreen)
            missionGenerated()
            
        } catch (error) {
            setStatus(qsTr("Error generating mission: " + error), qgcPal.colorRed)
        }
        
        busy = false
    }
    
    // Set status with color
    function setStatus(newStatus, newColor) {
        status = newStatus
        statusColor = newColor
        statusChanged(status, statusColor)
    }
    
    // Initialize with current map center if available
    Component.onCompleted: {
        if (map && map.center && map.center.isValid) {
            areaCenter = map.center
        }
    }
} 