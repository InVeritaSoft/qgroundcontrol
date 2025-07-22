# Mission Area Planner for QGroundControl Custom Build - Product Requirements Document (PRD)

## Overview

The Mission Area Planner is a custom UI module for QGroundControl (QGC) that enables users to visually define a rectangular area, adjust its dimensions, spacing, and generate a grid of mission points for drone operations. This tool is designed for users who need to plan precise, repeatable missions over a defined area, such as for surveying, mapping, or agricultural applications. The planner will be integrated into the Plan Flight view of QGC as a custom panel, leveraging QML and C++ for seamless integration and maintainability.

**Primary Objective**: Convert the mission GUI functionality from `mission_gui.py` into native QGC QML controls that integrate seamlessly with the existing Plan Flight view, providing interactive area definition and manipulation, real-time grid generation and visualization, direct mission upload to connected vehicles, and no external dependencies or manual file operations.

## Core Features

### Interactive Area Definition

-   Users can set the center, width, and height of a rectangular area on the map
-   Area can be moved interactively (via UI controls or drag-and-drop)
-   Directional movement buttons (N/S/E/W) for fine positioning
-   Center coordinate input (latitude/longitude)

### Grid and Point Generation

-   Adjustable line spacing and number of points per line
-   Visual overlay of grid lines and mission points within the area
-   Real-time grid calculation and visualization
-   Grid generation parameters with immediate feedback

### Mission Generation

-   Generate mission waypoints based on the defined grid
-   Integrate with QGC's mission upload system (no manual .waypoints file writing)
-   Automatic waypoint generation from grid
-   Direct upload to connected vehicle
-   Mission file generation (optional backup)

### Parameter Controls

-   QML-based controls for width, height, line spacing, number of points, and area movement
-   Width and height input fields (meters)
-   Line spacing control (meters between parallel lines)
-   Number of points per line

### Precise Home Location

-   Option to set home/launch location using device geolocation or map interaction
-   Precise home location setting via device GPS
-   Map-based location selection
-   Current vehicle position integration

### Custom Build Friendly

-   Implemented as a resource override or plugin, following QGC's custom build architecture
-   No external dependencies beyond Qt/QGC
-   Follows QGC custom build and resource override conventions

## User Experience

### User Personas

-   Surveyors, mappers, agricultural professionals, and advanced drone operators

### Key User Flows

1. Open Plan Flight view and access the Mission Area Planner panel
2. Adjust area parameters (center, width, height, spacing, points)
3. Visualize the area, grid, and points on the map
4. Generate and review the mission
5. Upload the mission to the connected vehicle

### UI/UX Considerations

-   Controls should be intuitive and responsive
-   Map overlays must update in real time as parameters change
-   Integration should feel native to QGC, matching its look and feel
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

## Technical Architecture

### Integration Strategy

-   **Resource Override Approach**: Use QGC's resource override system to extend the Plan Flight view
-   **QML Component Structure**: Create modular QML components following QGC patterns
-   **C++ Backend**: Implement geodesic calculations and mission logic in C++ for performance
-   **No External Dependencies**: All functionality must be self-contained within QGC

### System Components

-   QML UI component for controls and map overlays
-   C++ backend (if needed) for geodesic calculations and mission logic
-   Integration with QGC's mission planning/upload APIs

### Component Structure

#### 1. Mission Area Planner Panel (`MissionAreaPlannerPanel.qml`)

-   Main control panel for area definition
-   Input fields for width, height, line spacing, points per line
-   Directional movement controls
-   Status display and feedback
-   QGCTextField for width, height, line spacing, points per line
-   QGCButton for directional movement (N/S/E/W)
-   QGCButton for "Set Current Location"
-   QGCButton for "Generate Mission"
-   QGCLabel for status and feedback
-   Integration with MissionAreaPlanner backend

#### 2. Mission Area Map Overlay (`MissionAreaMapOverlay.qml`)

-   Visual representation of defined area on map
-   Grid lines and waypoint markers
-   Interactive area manipulation
-   Real-time updates as parameters change
-   MapPolygon for area visualization
-   MapPolyline for grid lines
-   MapQuickItem for waypoint markers
-   Interactive drag/drop for area positioning
-   Real-time updates from backend

#### 3. Mission Area Planner Backend (`MissionAreaPlanner.h/.cc`)

-   C++ class for geodesic calculations
-   Grid generation algorithms
-   Mission waypoint creation
-   Integration with QGC mission controller
-   Q_PROPERTY for center, width, height, lineSpacing, pointsPerLine
-   Q_INVOKABLE methods for parameter updates
-   Q_INVOKABLE methods for grid generation
-   Q_INVOKABLE methods for mission creation
-   Signal/slot connections for QML integration
-   Geodesic calculations using Qt positioning
-   Grid generation algorithms
-   Waypoint creation for QGC mission format
-   Integration with QGC mission controller
-   Error handling and validation

#### 4. Plan View Integration (`PlanView.qml` override)

-   Resource override to add custom panel to right side
-   Proper integration with existing mission controls
-   Maintain existing QGC functionality

### Data Models

-   Area definition: center (lat/lon), width, height
-   Grid parameters: line spacing, number of points
-   Waypoint list: generated from grid

### APIs and Integrations

-   QGC's QML Map and overlay types (`MapPolygon`, `MapPolyline`, etc.)
-   QGC mission interface for uploading missions
-   Qt geospatial classes (`QGeoCoordinate`)

### Data Flow

1. User adjusts parameters in MissionAreaPlannerPanel
2. Changes trigger updates in MissionAreaPlanner backend
3. Backend recalculates grid and waypoints
4. MissionAreaMapOverlay updates visual representation
5. User can generate and upload mission directly

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

## Development Roadmap

### MVP Requirements

1. QML UI for area/grid controls and map overlays
2. Geodesic/grid calculation logic in QML JS or C++
3. Integration with QGC mission upload (generate/upload waypoints)
4. Resource override or plugin integration for Plan Flight view

### Development Phases

#### Phase 1: Core Infrastructure

1. Create MissionAreaPlanner C++ backend
2. Implement basic geodesic calculations
3. Create QML property bindings
4. Set up resource override system

#### Phase 2: UI Components

1. Implement MissionAreaPlannerPanel.qml
2. Create input controls and validation
3. Add directional movement buttons
4. Implement status display

#### Phase 3: Map Integration

1. Create MissionAreaMapOverlay.qml
2. Implement area visualization
3. Add grid line rendering
4. Create interactive positioning

#### Phase 4: Mission Generation

1. Implement waypoint generation
2. Add mission upload functionality
3. Create error handling
4. Add progress indicators

#### Phase 5: Integration & Testing

1. Integrate all components
2. Test with real vehicles
3. Performance optimization
4. User experience refinement

### Future Enhancements

-   Support for arbitrary polygonal areas
-   Advanced mission types (e.g., lawnmower, spiral)
-   Save/load area templates
-   More advanced point/line spacing logic

### Scope Pacing

-   Start with rectangle/grid only, then iterate

## Logical Dependency Chain

1. Implement QML UI and controls for area/grid parameters
2. Implement map overlays for area, grid lines, and points
3. Implement geodesic/grid calculation logic
4. Integrate with QGC mission upload system
5. Polish UI/UX and test in custom build
6. (Optional) Add advanced features and templates

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

### Infrastructure Requirements

-   No external dependencies beyond Qt/QGC
-   Follows QGC custom build and resource override conventions

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

## Risks and Mitigations

### Technical Challenges

-   **Porting geodesic math from Python/geopy to Qt/QML**: Use Qt's `QGeoCoordinate` and test thoroughly
-   **Ensuring overlays and controls are performant**: Profile and optimize QML as needed
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

### MVP Definition

-   Focus on rectangle/grid only for first release

### Resource Constraints

-   Leverage QGC's plugin/resource override system to minimize upstream changes

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

## Appendix

### Research Findings

-   QGC supports custom QML panels and resource overrides for UI extension
-   QtLocation and QGeoCoordinate provide all needed geospatial functionality

### Technical Specifications

-   QML for UI, C++ for heavy logic if needed
-   Follows QGC custom build and resource override best practices

## Conclusion

This PRD defines a comprehensive approach to implementing mission GUI controls in QGC custom build, converting the functionality from `mission_gui.py` into native QGC components. The implementation will provide users with intuitive flight planning capabilities directly within QGC, eliminating the need for external applications while maintaining the professional quality and reliability expected from QGC.

The modular architecture ensures maintainability and extensibility, while the resource override approach ensures compatibility with future QGC updates. The implementation follows QGC's established patterns and coding standards, ensuring seamless integration with the existing codebase.

The Mission Area Planner will enable users to visually define rectangular areas, adjust dimensions and spacing, and generate precise grid-based missions for drone operations, all within the familiar QGC interface.
