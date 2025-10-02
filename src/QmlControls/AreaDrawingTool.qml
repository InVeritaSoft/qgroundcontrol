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
    property bool isDrawingMode: false
    property var onAreaDrawn: null // Callback function when area is drawn
    
    // Drawing state
    property var areaCenter: QtPositioning.coordinate()
    property real areaWidth: 100.0
    property real areaHeight: 100.0
    property real areaRotation: 0.0
    
    // Visual properties
    property color areaColor: "#2196F3"
    property real areaOpacity: 0.3
    property real borderWidth: 2
    
    // Mouse area for drawing
    MouseArea {
        id: drawingMouseArea
        anchors.fill: parent
        enabled: isDrawingMode
        acceptedButtons: Qt.LeftButton
        
        property point startPos
        property bool hasMoved: false
        
        onPressed: function(mouse) {
            if (mapControl) {
                // Get click coordinate
                var p = drawingMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(p.x, p.y), false)
                
                if (clickCoordinate.isValid) {
                    areaCenter = clickCoordinate
                    console.log("Area center set to:", clickCoordinate.latitude, clickCoordinate.longitude)
                }
                
                startPos = Qt.point(mouse.x, mouse.y)
                hasMoved = false
            }
        }
        
        onPositionChanged: function(mouse) {
            if (pressed && mapControl) {
                hasMoved = true
                // Get current mouse position coordinate for area sizing
                var mapped = drawingMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var currentCoordinate = mapControl.toCoordinate(Qt.point(mapped.x, mapped.y), false)
                
                if (currentCoordinate.isValid && areaCenter.isValid) {
                    // Calculate area size based on distance from center
                    var distance = areaCenter.distanceTo(currentCoordinate)
                    areaWidth = distance * 2
                    areaHeight = distance * 2
                }
            }
        }
        
        onReleased: function(mouse) {
            if (!hasMoved && mapControl) {
                // Single click - set center with default size
                var mappedClick = drawingMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(mappedClick.x, mappedClick.y), false)
                
                if (clickCoordinate.isValid) {
                    areaCenter = clickCoordinate
                    areaWidth = 100.0  // Default size
                    areaHeight = 100.0
                    console.log("Area set at:", clickCoordinate.latitude, clickCoordinate.longitude)
                }
            }
            
            // Notify that area is drawn
            if (areaCenter.isValid && onAreaDrawn) {
                onAreaDrawn(areaCenter, areaWidth, areaHeight, areaRotation)
            }
        }
    }
    
    // Area visualization
    Repeater {
        model: isDrawingMode && areaCenter.isValid ? 1 : 0
        
        MapPolygon {
            id: areaPolygon
            color: areaColor
            border.color: areaColor
            border.width: borderWidth
            opacity: areaOpacity
            
            // Calculate polygon vertices
            function updateVertices() {
                if (!areaCenter.isValid) return
                
                var center = areaCenter
                var width = areaWidth
                var height = areaHeight
                var rotation = areaRotation
                
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
                    var lonOffset = rotatedX / (111000.0 * Math.cos(center.latitude * Math.PI / 180.0))
                    
                    var vertex = QtPositioning.coordinate(
                        center.latitude + latOffset,
                        center.longitude + lonOffset,
                        center.altitude
                    )
                    vertices.push(vertex)
                }
                
                areaPolygon.path = vertices
            }
            
            Component.onCompleted: updateVertices()
            
            Connections {
                target: root
                function onAreaCenterChanged() { updateVertices() }
                function onAreaWidthChanged() { updateVertices() }
                function onAreaHeightChanged() { updateVertices() }
                function onAreaRotationChanged() { updateVertices() }
            }
        }
    }
    
    // Simple controls for area adjustment
    Rectangle {
        id: controlsPanel
        width: ScreenTools.defaultFontPixelWidth * 20
        height: childrenRect.height + ScreenTools.defaultFontPixelHeight
        color: Qt.rgba(0, 0, 0, 0.8)
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        visible: isDrawingMode && areaCenter.isValid
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: ScreenTools.defaultFontPixelHeight
        
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.5
            spacing: ScreenTools.defaultFontPixelHeight * 0.25
            
            QGCLabel {
                text: qsTr("Drawing Area")
                font.pointSize: ScreenTools.defaultFontPointSize
                font.bold: true
                color: "white"
            }
            
            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                
                QGCLabel {
                    text: qsTr("W:")
                    color: "white"
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaWidth.toFixed(0)
                    onTextChanged: areaWidth = parseFloat(text) || 0
                    Layout.fillWidth: true
                }
                
                QGCLabel {
                    text: qsTr("H:")
                    color: "white"
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaHeight.toFixed(0)
                    onTextChanged: areaHeight = parseFloat(text) || 0
                    Layout.fillWidth: true
                }
            }
            
            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                
                QGCLabel {
                    text: qsTr("Rot:")
                    color: "white"
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 3
                }
                QGCTextField {
                    text: areaRotation.toFixed(0)
                    onTextChanged: areaRotation = parseFloat(text) || 0
                    Layout.fillWidth: true
                }
            }
            
            QGCButton {
                text: qsTr("Use This Area")
                width: parent.width
                height: ScreenTools.defaultFontPixelHeight * 2
                primary: true
                onClicked: {
                    if (areaCenter.isValid && onAreaDrawn) {
                        onAreaDrawn(areaCenter, areaWidth, areaHeight, areaRotation)
                    }
                }
            }
        }
    }
}
