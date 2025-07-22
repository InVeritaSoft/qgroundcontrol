/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.15
import QtLocation 5.15
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

Item {
    id: missionAreaMapOverlay
    
    property var map
    property var missionAreaPlanner
    
    // Map overlay components
    MapPolygon {
        id: areaPolygon
        map: missionAreaMapOverlay.map
        color: Qt.rgba(0, 0.5, 1, 0.3)
        border.color: Qt.rgba(0, 0.5, 1, 0.8)
        border.width: 2
        visible: missionAreaPlanner && missionAreaPlanner.areaCorners && missionAreaPlanner.areaCorners.length >= 3
        
        // Update polygon path when area corners change
        onVisibleChanged: {
            if (visible && missionAreaPlanner.areaCorners) {
                updatePolygonPath()
            }
        }
        
        Connections {
            target: missionAreaPlanner
            function onAreaUpdated() {
                updatePolygonPath()
            }
        }
        
        function updatePolygonPath() {
            if (missionAreaPlanner.areaCorners && missionAreaPlanner.areaCorners.length >= 3) {
                var path = []
                for (var i = 0; i < missionAreaPlanner.areaCorners.length; i++) {
                    path.push(missionAreaPlanner.areaCorners[i])
                }
                areaPolygon.path = path
            }
        }
    }
    
    // Grid lines
    Repeater {
        id: gridLinesRepeater
        model: missionAreaPlanner ? missionAreaPlanner.gridLines : []
        
        MapPolyline {
            map: missionAreaMapOverlay.map
            line.color: Qt.rgba(1, 1, 0, 0.6)
            line.width: 1
            visible: missionAreaPlanner && missionAreaPlanner.gridLines
            
            path: modelData ? [modelData.start, modelData.end] : []
        }
    }
    
    // Waypoint markers
    Repeater {
        id: waypointMarkersRepeater
        model: missionAreaPlanner ? missionAreaPlanner.waypoints : []
        
        MapQuickItem {
            map: missionAreaMapOverlay.map
            coordinate: modelData
            visible: missionAreaPlanner && missionAreaPlanner.waypoints
            
            anchorPoint.x: waypointImage.width / 2
            anchorPoint.y: waypointImage.height / 2
            
            sourceItem: Image {
                id: waypointImage
                source: "qrc:/qmlimages/MapMission.svg"
                width: ScreenTools.defaultFontPixelHeight
                height: ScreenTools.defaultFontPixelHeight
            }
        }
    }
    
    // Area center marker
    MapQuickItem {
        id: centerMarker
        map: missionAreaMapOverlay.map
        coordinate: missionAreaPlanner ? missionAreaPlanner.areaCenter : QtPositioning.coordinate(0, 0)
        visible: missionAreaPlanner && missionAreaPlanner.areaCenter && missionAreaPlanner.areaCenter.isValid
        
        anchorPoint.x: centerImage.width / 2
        anchorPoint.y: centerImage.height / 2
        
        sourceItem: Rectangle {
            id: centerImage
            width: ScreenTools.defaultFontPixelHeight * 1.5
            height: ScreenTools.defaultFontPixelHeight * 1.5
            radius: width / 2
            color: Qt.rgba(1, 0, 0, 0.8)
            border.color: Qt.rgba(1, 1, 1, 1)
            border.width: 2
            
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.3
                height: parent.height * 0.3
                radius: width / 2
                color: Qt.rgba(1, 1, 1, 1)
            }
        }
    }
    
    // Interactive area manipulation
    MouseArea {
        id: mapMouseArea
        anchors.fill: parent
        enabled: missionAreaPlanner && missionAreaPlanner.areaCenter
        hoverEnabled: true
        
        property bool dragging: false
        property var dragStartCoord: null
        property var originalCenter: null
        
        onPressed: {
            if (missionAreaPlanner && missionAreaPlanner.areaCenter) {
                dragging = true
                dragStartCoord = map.toCoordinate(Qt.point(mouse.x, mouse.y), false)
                originalCenter = missionAreaPlanner.areaCenter
            }
        }
        
        onReleased: {
            dragging = false
            dragStartCoord = null
            originalCenter = null
        }
        
        onPositionChanged: {
            if (dragging && dragStartCoord && originalCenter) {
                var currentCoord = map.toCoordinate(Qt.point(mouse.x, mouse.y), false)
                if (currentCoord && currentCoord.isValid) {
                    // Calculate displacement
                    var deltaLat = currentCoord.latitude - dragStartCoord.latitude
                    var deltaLon = currentCoord.longitude - dragStartCoord.longitude
                    
                    // Update area center
                    var newCenter = QtPositioning.coordinate(
                        originalCenter.latitude + deltaLat,
                        originalCenter.longitude + deltaLon
                    )
                    missionAreaPlanner.areaCenter = newCenter
                }
            }
        }
        
        onClicked: {
            if (!dragging && missionAreaPlanner) {
                var clickedCoord = map.toCoordinate(Qt.point(mouse.x, mouse.y), false)
                if (clickedCoord && clickedCoord.isValid) {
                    missionAreaPlanner.areaCenter = clickedCoord
                }
            }
        }
    }
    
    // Update overlays when mission area planner changes
    Connections {
        target: missionAreaPlanner
        
        function onAreaUpdated() {
            // Trigger repaints
            areaPolygon.updatePolygonPath()
        }
        
        function onGridUpdated() {
            // Grid lines and waypoints will update automatically through Repeater
        }
    }
} 