# Mission Points Drawing Tool - Integration Guide

## Overview

This document provides a comprehensive guide for extracting and integrating the Mission Points Drawing tool from QGroundControl's AreaPlanEditor. The tool enables interactive area definition through map clicks and provides real-time mission waypoint generation.

## Core Components

### 1. C++ Backend (AreaPlanEditor.h/.cc)

#### Key Properties for Drawing Mode
```cpp
// Drawing mode control
Q_PROPERTY(bool isDrawingMode READ isDrawingMode WRITE setIsDrawingMode NOTIFY isDrawingModeChanged)

// Area definition properties
Q_PROPERTY(qreal areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
Q_PROPERTY(qreal areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
Q_PROPERTY(qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
Q_PROPERTY(int numPoints READ numPoints WRITE setNumPoints NOTIFY numPointsChanged)
Q_PROPERTY(qreal missionAltitude READ missionAltitude WRITE setMissionAltitude NOTIFY missionAltitudeChanged)
Q_PROPERTY(QGeoCoordinate areaCenter READ areaCenter WRITE setAreaCenter NOTIFY areaCenterChanged)
Q_PROPERTY(qreal areaRotation READ areaRotation WRITE setAreaRotation NOTIFY areaRotationChanged)

// Multi-drone planning properties
Q_PROPERTY(int droneCount READ droneCount WRITE setDroneCount NOTIFY droneCountChanged)
Q_PROPERTY(qreal altitudeBandStart READ altitudeBandStart WRITE setAltitudeBandStart NOTIFY altitudeBandStartChanged)
Q_PROPERTY(qreal altitudeBandStep READ altitudeBandStep WRITE setAltitudeBandStep NOTIFY altitudeBandStepChanged)
```

#### Essential Methods
```cpp
// Drawing mode control
Q_INVOKABLE void setIsDrawingMode(bool drawingMode);
Q_INVOKABLE void setAreaCenter(const QGeoCoordinate& center);

// Area manipulation
Q_INVOKABLE void moveAreaNorth();
Q_INVOKABLE void moveAreaSouth();
Q_INVOKABLE void moveAreaEast();
Q_INVOKABLE void moveAreaWest();
Q_INVOKABLE void rotateAreaClockwise();
Q_INVOKABLE void rotateAreaCounterClockwise();
Q_INVOKABLE void centerArea();

// Mission generation
Q_INVOKABLE QVariantList generateWaypoints();
Q_INVOKABLE QVariantList computePerDroneWaypointPreview() const;
Q_INVOKABLE void addWaypointsToMission();
Q_INVOKABLE void saveMissionFile();
```

### 2. QML Frontend Components

#### Drawing Mode Button
```qml
QGCButton {
    id: drawingModeButton
    text: {
        if (!areaPlanEditor) return qsTr("Activate Area Definition Mode")
        return areaPlanEditor.isDrawingMode ? 
            qsTr("Deactivate Area Definition Mode") : 
            qsTr("Activate Area Definition Mode")
    }
    width: parent.width
    height: _h * 2.2
    onClicked: {
        if (areaPlanEditor) {
            var newMode = !areaPlanEditor.isDrawingMode
            areaPlanEditor.setIsDrawingMode(newMode)
        }
    }
}
```

#### Interactive Map Area (AreaPlanMapVisuals.qml)
```qml
MouseArea {
    id: mapAreaMouseArea
    anchors.fill: parent
    enabled: interactive && isDrawingMode
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
```

#### Area Control Buttons
```qml
// Area movement controls
Grid {
    anchors.centerIn: parent
    columns: 3
    rowSpacing: _h * 0.4
    columnSpacing: _w * 0.5
    
    Item { width: _w * 6; height: _h * 2 }
    QGCButton {
        text: qsTr("↑")
        width: _w * 6
        height: _h * 2
        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaNorth()
    }
    Item { width: _w * 6; height: _h * 2 }
    
    QGCButton {
        text: qsTr("←")
        width: _w * 6
        height: _h * 2
        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaWest()
    }
    QGCButton {
        text: qsTr("Center Area")
        width: _w * 12
        height: _h * 2
        onClicked: if (areaPlanEditor) areaPlanEditor.centerArea()
    }
    QGCButton {
        text: qsTr("→")
        width: _w * 6
        height: _h * 2
        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaEast()
    }
    
    Item { width: _w * 6; height: _h * 2 }
    QGCButton {
        text: qsTr("↓")
        width: _w * 6
        height: _h * 2
        onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaSouth()
    }
    Item { width: _w * 6; height: _h * 2 }
}

// Area rotation controls
Row {
    spacing: _w * 0.2
    
    QGCButton {
        text: qsTr("Rotate Counterclockwise (-15°)")
        width: parent.width * 0.3
        height: parent.height
        onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaCounterClockwise()
    }
    
    QGCButton {
        text: qsTr("Reset Rotation to 0°")
        width: parent.width * 0.4
        height: parent.height
        onClicked: if (areaPlanEditor) areaPlanEditor.setAreaRotation(0.0)
    }
    
    QGCButton {
        text: qsTr("Rotate Clockwise (+15°)")
        width: parent.width * 0.3
        height: parent.height
        onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaClockwise()
    }
}
```

### 3. Mission Generation Logic

#### Core Waypoint Generation Algorithm
```cpp
QList<QVariant> AreaPlanEditor::generateWaypoints()
{
    QList<QVariant> waypoints;
    
    // Basic validation
    if (_areaCenter.isValid() == false || _areaWidth <= 0 || _areaHeight <= 0 || 
        _numPoints <= 0 || _lineSpacing <= 0) {
        return waypoints;
    }
    
    // Compute number of grid lines along height (north-south axis before rotation)
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    
    // Calculate half dimensions for centering
    const qreal halfWidth = _areaWidth * 0.5;
    const qreal halfHeight = _areaHeight * 0.5;
    
    // Rotation transformation
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosTheta = qCos(theta);
    const qreal sinTheta = qSin(theta);
    
    // Generate waypoints for each line
    for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
        // Calculate line position (north-south offset from center)
        const qreal lineOffset = (lineIndex * _lineSpacing) - halfHeight;
        
        // Generate points along this line
        for (int pointIndex = 0; pointIndex < _numPoints; ++pointIndex) {
            // Calculate point position along line (east-west offset from center)
            const qreal pointOffset = (pointIndex * (_areaWidth / qMax(1, _numPoints - 1))) - halfWidth;
            
            // Apply rotation transformation
            const qreal rotatedX = pointOffset * cosTheta - lineOffset * sinTheta;
            const qreal rotatedY = pointOffset * sinTheta + lineOffset * cosTheta;
            
            // Convert to geographic coordinate
            QGeoCoordinate waypoint = calculateOffsetCoordinate(
                _areaCenter, 
                qAbs(rotatedY), 
                rotatedY >= 0 ? 0.0 : 180.0
            );
            waypoint = calculateOffsetCoordinate(
                waypoint, 
                qAbs(rotatedX), 
                rotatedX >= 0 ? 90.0 : 270.0
            );
            waypoint.setAltitude(_missionAltitude);
            
            waypoints.append(QVariant::fromValue(waypoint));
        }
    }
    
    return waypoints;
}
```

#### Multi-Drone Preview Generation
```cpp
QList<QVariant> AreaPlanEditor::computePerDroneWaypointPreview() const
{
    QList<QVariant> preview;
    
    // Guard conditions
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) {
        return preview;
    }
    
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto roundRobin = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    
    // Precompute rotated coordinates for each line and point
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    
    auto rotateXY = [&](qreal x, qreal y) { 
        return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); 
    };
    
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };
    
    // Generate preview for each drone
    for (int droneIndex = 0; droneIndex < _droneCount; ++droneIndex) {
        QVariantMap droneData;
        droneData["droneIndex"] = droneIndex;
        droneData["altitudeOffsetM"] = _altitudeBandStart + (droneIndex * _altitudeBandStep);
        droneData["timeOffsetS"] = droneIndex * _timeOffsetPerDrone;
        
        QVariantList waypoints;
        const auto& assignedLines = roundRobin[droneIndex];
        
        for (int lineIdx : assignedLines) {
            const qreal lineOffset = (lineIdx * _lineSpacing) - halfH;
            
            for (int pointIdx = 0; pointIdx < _numPoints; ++pointIdx) {
                const qreal pointOffset = (pointIdx * (_areaWidth / qMax(1, _numPoints - 1))) - halfW;
                const auto rotated = rotateXY(pointOffset, lineOffset);
                
                QGeoCoordinate wp = offsetByXY(_areaCenter, rotated.x(), rotated.y());
                wp.setAltitude(_missionAltitude + droneData["altitudeOffsetM"].toReal());
                waypoints.append(QVariant::fromValue(wp));
            }
        }
        
        droneData["waypoints"] = waypoints;
        preview.append(droneData);
    }
    
    return preview;
}
```

## Integration Instructions

### Step 1: Extract Core Files

Copy these files to your new branch:
- `src/QmlControls/AreaPlanEditor.h`
- `src/QmlControls/AreaPlanEditor.cc`
- `src/QmlControls/AreaPlanEditor.qml`
- `src/QmlControls/AreaPlanMapVisuals.qml`

### Step 2: Update CMakeLists.txt

Add the new files to `src/QmlControls/CMakeLists.txt`:

```cmake
# Area Plan Editor
AreaPlanEditor.h
AreaPlanEditor.cc
AreaPlanEditor.qml
AreaPlanMapVisuals.qml
```

### Step 3: Register QML Types

In your main QML file or `QGroundControlQmlGlobal.h`, register the new types:

```cpp
qmlRegisterType<AreaPlanEditor>("QGroundControl", 1, 0, "AreaPlanEditor");
```

### Step 4: Dependencies

Ensure these QGroundControl components are available:
- `MissionController`
- `MissionManager`
- `Vehicle`
- `MultiVehicleManager`
- `SimpleMissionItem`
- `VisualMissionItem`

### Step 5: QML Integration

In your main QML file, add the drawing tool:

```qml
import QGroundControl 1.0

Item {
    // Your existing content
    
    AreaPlanEditor {
        id: areaPlanEditor
        planMasterController: missionController
    }
    
    // Map integration
    Map {
        id: mapControl
        // Your map configuration
        
        AreaPlanMapVisuals {
            mapControl: mapControl
            areaPlanEditor: areaPlanEditor
            interactive: true
        }
    }
    
    // Drawing controls UI
    Rectangle {
        // Your UI container
        
        Column {
            // Drawing mode button
            QGCButton {
                text: areaPlanEditor.isDrawingMode ? 
                    qsTr("Deactivate Area Definition Mode") : 
                    qsTr("Activate Area Definition Mode")
                onClicked: areaPlanEditor.setIsDrawingMode(!areaPlanEditor.isDrawingMode)
            }
            
            // Area parameters
            QGCTextField {
                text: areaPlanEditor.areaWidth
                onTextChanged: areaPlanEditor.setAreaWidth(parseFloat(text))
            }
            
            QGCTextField {
                text: areaPlanEditor.areaHeight
                onTextChanged: areaPlanEditor.setAreaHeight(parseFloat(text))
            }
            
            // Mission generation
            QGCButton {
                text: qsTr("Generate Mission")
                onClicked: areaPlanEditor.addWaypointsToMission()
            }
        }
    }
}
```

## Key Features

### Interactive Drawing
- **Click to Set Center**: First click sets the area center
- **Drag to Move**: Drag to reposition the area center
- **Real-time Preview**: Visual feedback of area boundaries and waypoints
- **Drawing Mode Toggle**: Enable/disable interactive drawing

### Area Manipulation
- **Movement Controls**: North/South/East/West movement buttons
- **Rotation Controls**: Clockwise/counter-clockwise rotation with reset
- **Centering**: Center area on current vehicle position
- **Parameter Input**: Width, height, line spacing, point count

### Mission Generation
- **Single Drone**: Basic waypoint generation for single vehicle
- **Multi-Drone**: Coordinated missions with altitude banding
- **Time Staggering**: Offset missions for coordinated execution
- **Mission Integration**: Direct integration with QGroundControl mission system

### Visualization
- **Area Boundaries**: Visual representation of defined area
- **Waypoint Preview**: Real-time waypoint visualization
- **Multi-Drone Colors**: Color-coded waypoints per drone
- **Altitude Bands**: Visual altitude differentiation

## Customization Options

### Visual Styling
```qml
AreaPlanMapVisuals {
    interiorColor: "#303030"        // Area fill color
    borderColor: "#2196F3"          // Area border color
    borderWidth: 2                  // Border thickness
    interiorOpacity: 0.7            // Fill opacity
    showGridLines: true             // Show grid lines
    showWaypoints: true             // Show waypoint markers
}
```

### Drawing Behavior
```qml
MouseArea {
    enabled: interactive && isDrawingMode
    acceptedButtons: Qt.LeftButton  // Only left mouse button
    // Customize interaction behavior
}
```

### Mission Parameters
```cpp
// In C++ backend
areaPlanEditor->setAreaWidth(100.0);      // 100 meters
areaPlanEditor->setAreaHeight(200.0);     // 200 meters
areaPlanEditor->setLineSpacing(10.0);     // 10 meter spacing
areaPlanEditor->setNumPoints(5);          // 5 points per line
areaPlanEditor->setMissionAltitude(50.0); // 50 meter altitude
areaPlanEditor->setDroneCount(3);         // 3 drones
```

## Troubleshooting

### Common Issues

1. **Drawing Mode Not Activating**
   - Ensure `areaPlanEditor` object is properly initialized
   - Check that `isDrawingMode` property is being set correctly
   - Verify MouseArea is enabled and interactive

2. **Waypoints Not Generating**
   - Validate area parameters (width, height, spacing > 0)
   - Ensure area center is valid coordinate
   - Check mission controller is properly connected

3. **Map Integration Issues**
   - Verify mapControl reference is valid
   - Ensure coordinate conversion is working
   - Check z-order for proper layering

4. **Multi-Drone Preview Not Showing**
   - Verify drone count > 1
   - Check altitude band parameters
   - Ensure preview data is being generated

### Debug Console Output

The tool provides extensive console logging for debugging:
```javascript
console.log("Drawing mode button clicked")
console.log("Map area pressed")
console.log("Area center set to:", latitude, longitude)
console.log("Generated waypoints:", waypointCount)
```

## Performance Considerations

- **Caching**: Waypoint generation results are cached for performance
- **Lazy Loading**: Preview data is only generated when needed
- **Optimization**: Use `enableOptimizations()` for large areas
- **Memory Management**: Clear cache when parameters change significantly

## Future Enhancements

- **Polygon Drawing**: Support for custom polygon areas
- **Path Following**: Waypoint generation along custom paths
- **Advanced Patterns**: Spiral, zigzag, and other coverage patterns
- **Real-time Editing**: Live parameter adjustment with immediate preview
- **Mission Templates**: Save and load common mission configurations

---

This tool provides a complete solution for interactive mission area definition and waypoint generation, making it easy to create complex multi-drone missions through intuitive map interaction.