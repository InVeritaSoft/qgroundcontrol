# Mission GUI Controls for QGroundControl Custom Build - Product Requirements Document (PRD)

## Overview

This PRD defines the implementation of custom mission planning controls for QGroundControl (QGC) custom build, converting the functionality from `mission_gui.py` into native QGC QML components. The goal is to provide intuitive flight planning controls directly in the Plan Flight view without requiring external Python applications or manual waypoint file generation.

## Core Requirements

### Primary Objective

Convert the mission GUI functionality from `mission_gui.py` into native QGC QML controls that integrate seamlessly with the existing Plan Flight view, providing:

-   Interactive area definition and manipulation
-   Real-time grid generation and visualization
-   Direct mission upload to connected vehicles
-   No external dependencies or manual file operations

### Key Features from mission_gui.py to Implement

1. **Area Definition Controls**

    - Width and height input fields (meters)
    - Center coordinate input (latitude/longitude)
    - Directional movement buttons (N/S/E/W) for fine positioning

2. **Grid Generation Parameters**

    - Line spacing control (meters between parallel lines)
    - Number of points per line
    - Real-time grid calculation and visualization

3. **Location Management**

    - Precise home location setting via device GPS
    - Map-based location selection
    - Current vehicle position integration

4. **Mission Generation**
    - Automatic waypoint generation from grid
    - Direct upload to connected vehicle
    - Mission file generation (optional backup)

## Technical Architecture

### Integration Strategy

-   **Resource Override Approach**: Use QGC's resource override system to extend the Plan Flight view
-   **QML Component Structure**: Create modular QML components following QGC patterns
-   **C++ Backend**: Implement geodesic calculations and mission logic in C++ for performance
-   **No External Dependencies**: All functionality must be self-contained within QGC

### Component Structure

#### 1. Mission Area Planner Panel (`MissionAreaPlannerPanel.qml`)

-   Main control panel for area definition
-   Input fields for width, height, line spacing, points per line
-   Directional movement controls
-   Status display and feedback

#### 2. Mission Area Map Overlay (`MissionAreaMapOverlay.qml`)

-   Visual representation of defined area on map
-   Grid lines and waypoint markers
-   Interactive area manipulation
-   Real-time updates as parameters change

#### 3. Mission Area Planner Backend (`MissionAreaPlanner.h/.cc`)

-   C++ class for geodesic calculations
-   Grid generation algorithms
-   Mission waypoint creation
-   Integration with QGC mission controller

#### 4. Plan View Integration (`PlanView.qml` override)

-   Resource override to add custom panel to right side
-   Proper integration with existing mission controls
-   Maintain existing QGC functionality

### Data Flow

1. User adjusts parameters in MissionAreaPlannerPanel
2. Changes trigger updates in MissionAreaPlanner backend
3. Backend recalculates grid and waypoints
4. MissionAreaMapOverlay updates visual representation
5. User can generate and upload mission directly

## Implementation Requirements

### QML Components

#### MissionAreaPlannerPanel.qml

```qml
// Must include:
- QGCTextField for width, height, line spacing, points per line
- QGCButton for directional movement (N/S/E/W)
- QGCButton for "Set Current Location"
- QGCButton for "Generate Mission"
- QGCLabel for status and feedback
- Integration with MissionAreaPlanner backend
```

#### MissionAreaMapOverlay.qml

```qml
// Must include:
- MapPolygon for area visualization
- MapPolyline for grid lines
- MapQuickItem for waypoint markers
- Interactive drag/drop for area positioning
- Real-time updates from backend
```

### C++ Backend Requirements

#### MissionAreaPlanner.h

```cpp
// Must include:
- Q_PROPERTY for center, width, height, lineSpacing, pointsPerLine
- Q_INVOKABLE methods for parameter updates
- Q_INVOKABLE methods for grid generation
- Q_INVOKABLE methods for mission creation
- Signal/slot connections for QML integration
```

#### MissionAreaPlanner.cc

```cpp
// Must include:
- Geodesic calculations using Qt positioning
- Grid generation algorithms
- Waypoint creation for QGC mission format
- Integration with QGC mission controller
- Error handling and validation
```

### Resource Override Configuration

#### custom.qrc

```xml
<!-- Must include resource mappings for: -->
<qresource prefix="/Custom/qml">
    <file alias="QGroundControl/QmlControls/PlanView.qml">res/Custom/PlanView.qml</file>
    <file alias="QGroundControl/PlanView/MissionAreaPlannerPanel.qml">res/Custom/MissionAreaPlannerPanel.qml</file>
    <file alias="QGroundControl/PlanView/MissionAreaMapOverlay.qml">res/Custom/MissionAreaMapOverlay.qml</file>
</qresource>
```

#### CustomPlugin.cc

```cpp
// Must include:
- Resource override registration for PlanView.qml
- MissionAreaPlanner backend registration
- Proper QML context setup
```

## User Experience Requirements

### Plan Flight View Integration

-   Custom panel appears in right side of Plan Flight view
-   Panel is collapsible and doesn't interfere with existing controls
-   Maintains existing QGC look and feel
-   Responsive design for different screen sizes

### Control Layout

-   Logical grouping of related controls
-   Clear labeling and units (meters, degrees)
-   Real-time feedback for parameter changes
-   Error handling with user-friendly messages

### Map Interaction

-   Visual area representation updates in real-time
-   Click-to-position functionality for area center
-   Drag-and-drop area manipulation
-   Grid lines and waypoints clearly visible

### Mission Generation

-   One-click mission generation from current parameters
-   Direct upload to connected vehicle
-   Progress indication during upload
-   Success/error feedback

## Technical Constraints

### QGC Compatibility

-   Must work with current QGC version
-   Follow QGC coding standards and patterns
-   Use existing QGC controls and styling
-   Maintain backward compatibility

### Performance Requirements

-   Real-time updates without lag
-   Efficient geodesic calculations
-   Minimal memory footprint
-   Responsive UI interactions

### Build System Integration

-   Must compile with existing QGC build system
-   No external dependencies
-   Proper CMake integration
-   Resource file management

## Development Phases

### Phase 1: Core Infrastructure

1. Create MissionAreaPlanner C++ backend
2. Implement basic geodesic calculations
3. Create QML property bindings
4. Set up resource override system

### Phase 2: UI Components

1. Implement MissionAreaPlannerPanel.qml
2. Create input controls and validation
3. Add directional movement buttons
4. Implement status display

### Phase 3: Map Integration

1. Create MissionAreaMapOverlay.qml
2. Implement area visualization
3. Add grid line rendering
4. Create interactive positioning

### Phase 4: Mission Generation

1. Implement waypoint generation
2. Add mission upload functionality
3. Create error handling
4. Add progress indicators

### Phase 5: Integration & Testing

1. Integrate all components
2. Test with real vehicles
3. Performance optimization
4. User experience refinement

## Success Criteria

### Functional Requirements

-   All mission_gui.py functionality replicated in QGC
-   Real-time parameter updates and visualization
-   Successful mission generation and upload
-   Proper error handling and user feedback

### Technical Requirements

-   Clean integration with existing QGC codebase
-   No external dependencies or manual file operations
-   Responsive UI with real-time updates
-   Proper resource management and memory usage

### User Experience Requirements

-   Intuitive controls following QGC patterns
-   Clear visual feedback for all operations
-   Seamless integration with Plan Flight view
-   Professional appearance matching QGC standards

## Risk Mitigation

### Technical Risks

-   **Complex geodesic calculations**: Use Qt's built-in positioning classes
-   **QML performance**: Implement efficient property bindings and avoid unnecessary updates
-   **Resource override conflicts**: Follow QGC's resource override patterns exactly

### Integration Risks

-   **Mission controller compatibility**: Use QGC's official mission APIs
-   **Map overlay performance**: Implement efficient rendering and update strategies
-   **Build system issues**: Follow existing custom build patterns

### User Experience Risks

-   **UI complexity**: Start with essential controls, add advanced features incrementally
-   **Learning curve**: Provide clear labels and tooltips
-   **Error handling**: Implement comprehensive validation and user feedback

## Dependencies

### QGC Components Required

-   QGroundControl.PlanView module
-   QGroundControl.FlightMap module
-   QGroundControl.Controllers module
-   QGroundControl.FactSystem module

### Qt Components Required

-   QtLocation for map integration
-   QtPositioning for geodesic calculations
-   QtQuick for QML components
-   QtCore for C++ backend

### Custom Build Requirements

-   Existing custom build infrastructure
-   Resource override system
-   QML module registration
-   C++ plugin architecture

## Conclusion

This PRD defines a comprehensive approach to implementing mission GUI controls in QGC custom build, converting the functionality from `mission_gui.py` into native QGC components. The implementation will provide users with intuitive flight planning capabilities directly within QGC, eliminating the need for external applications while maintaining the professional quality and reliability expected from QGC.

The modular architecture ensures maintainability and extensibility, while the resource override approach ensures compatibility with future QGC updates. The implementation follows QGC's established patterns and coding standards, ensuring seamless integration with the existing codebase.
