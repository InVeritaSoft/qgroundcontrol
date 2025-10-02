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
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Demining zone visualization on map
Item {
    id: _root

    property var    mapControl                                  ///< Map control to place item in
    property var    waypoints:                                  [] ///< List of waypoint coordinates
    property color  zoneColor:                  "#4CAF50"      ///< Green color for demining zone
    property real   zoneOpacity:                0.4            ///< 40% opacity as specified
    property real   zoneWidth:                  4.5            ///< Demining zone width in meters
    property real   zoneRadius:                 2.25           ///< Demining zone radius in meters (half width)

    property var    _deminingZonePolygon:       null
    property var    _missionService:            QGroundControl.missionService

    Component.onCompleted: {
        console.log("DeminingZoneVisuals created")
        if (_missionService) {
            _missionService.waypointsGenerated.connect(updateDeminingZone)
            _missionService.missionGenerationCompleted.connect(function(success, message) {
                if (success) {
                    console.log("Mission generation completed, updating demining zone")
                    updateDeminingZone(waypoints)
                }
            })
        }
    }

    function updateDeminingZone(generatedWaypoints) {
        console.log("Updating demining zone with", generatedWaypoints.length, "waypoints")
        waypoints = generatedWaypoints
        
        if (waypoints.length < 2) {
            console.log("Not enough waypoints for demining zone visualization")
            return
        }
        
        // Calculate center point of all waypoints
        var centerLat = 0
        var centerLng = 0
        var minLat = waypoints[0].latitude
        var maxLat = waypoints[0].latitude
        var minLng = waypoints[0].longitude
        var maxLng = waypoints[0].longitude
        
        for (var i = 0; i < waypoints.length; i++) {
            var wp = waypoints[i]
            centerLat += wp.latitude
            centerLng += wp.longitude
            
            minLat = Math.min(minLat, wp.latitude)
            maxLat = Math.max(maxLat, wp.latitude)
            minLng = Math.min(minLng, wp.longitude)
            maxLng = Math.max(maxLng, wp.longitude)
        }
        
        centerLat /= waypoints.length
        centerLng /= waypoints.length
        
        console.log("Demining zone center:", centerLat, centerLng)
        console.log("Zone bounds - Lat:", minLat, "to", maxLat, "Lng:", minLng, "to", maxLng)
        
        // Calculate zone dimensions
        var centerCoord = QtPositioning.coordinate(centerLat, centerLng)
        var topLeftCoord = QtPositioning.coordinate(maxLat, minLng)
        var bottomRightCoord = QtPositioning.coordinate(minLat, maxLng)
        
        // Calculate actual width and height in meters
        var widthMeters = centerCoord.distanceTo(QtPositioning.coordinate(centerLat, maxLng)) * 2
        var heightMeters = centerCoord.distanceTo(QtPositioning.coordinate(maxLat, centerLng)) * 2
        
        console.log("Calculated zone dimensions:", widthMeters, "x", heightMeters, "meters")
        
        // Create perfect square zone
        var halfWidth = Math.max(widthMeters, heightMeters) / 2
        var topLeft = centerCoord.atDistanceAndAzimuth(halfWidth, 315) // Northwest
        var topRight = centerCoord.atDistanceAndAzimuth(halfWidth, 45)  // Northeast
        var bottomLeft = centerCoord.atDistanceAndAzimuth(halfWidth, 225) // Southwest
        var bottomRight = centerCoord.atDistanceAndAzimuth(halfWidth, 135) // Southeast
        
        // Create polygon for demining zone
        var polygonPath = [
            topLeft,
            topRight,
            bottomRight,
            bottomLeft
        ]
        
        console.log("Demining zone polygon created with", polygonPath.length, "corners")
        
        // Remove existing polygon if any
        if (_deminingZonePolygon) {
            _deminingZonePolygon.destroy()
        }
        
        // Create new polygon
        var component = Qt.createComponent("qrc:/qml/QGCMapPolygonVisuals.qml")
        if (component.status === Component.Ready) {
            _deminingZonePolygon = component.createObject(mapControl, {
                "mapControl": mapControl,
                "interiorColor": zoneColor,
                "interiorOpacity": zoneOpacity,
                "borderWidth": 2,
                "borderColor": zoneColor,
                "interactive": false
            })
            
            if (_deminingZonePolygon) {
                // Set the polygon path
                _deminingZonePolygon.mapPolygon = Qt.createQmlObject('
                    import QtQuick
                    import QtPositioning
                    import QGroundControl
                    
                    QGCMapPolygon {
                        id: deminingZonePolygon
                        path: ' + JSON.stringify(polygonPath.map(function(coord) {
                            return [coord.latitude, coord.longitude]
                        })) + '
                    }
                ', _deminingZonePolygon)
                
                console.log("Demining zone polygon created and added to map")
            }
        } else {
            console.log("Failed to create demining zone polygon component:", component.errorString())
        }
    }

    function clearDeminingZone() {
        console.log("Clearing demining zone")
        if (_deminingZonePolygon) {
            _deminingZonePolygon.destroy()
            _deminingZonePolygon = null
        }
        waypoints = []
    }

    onVisibleChanged: {
        if (_deminingZonePolygon) {
            _deminingZonePolygon.visible = visible
        }
    }
}
