# Mission Area Planner for QGroundControl – Product Requirements Document (PRD)

## Overview

The Mission Area Planner is a custom UI module for QGroundControl (QGC) that enables users to visually define a rectangular area, adjust its dimensions, spacing, and generate a grid of mission points for drone operations. This tool is designed for users who need to plan precise, repeatable missions over a defined area, such as for surveying, mapping, or agricultural applications. The planner will be integrated into the Plan Flight view of QGC as a custom panel, leveraging QML and C++ for seamless integration and maintainability.

## Core Features

-   **Interactive Area Definition**
    -   Users can set the center, width, and height of a rectangular area on the map.
    -   Area can be moved interactively (via UI controls or drag-and-drop).
-   **Grid and Point Generation**
    -   Adjustable line spacing and number of points per line.
    -   Visual overlay of grid lines and mission points within the area.
-   **Mission Generation**
    -   Generate mission waypoints based on the defined grid.
    -   Integrate with QGC’s mission upload system (no manual .waypoints file writing).
-   **Parameter Controls**
    -   QML-based controls for width, height, line spacing, number of points, and area movement.
-   **Precise Home Location**
    -   Option to set home/launch location using device geolocation or map interaction.
-   **Custom Build Friendly**
    -   Implemented as a resource override or plugin, following QGC’s custom build architecture.

## User Experience

-   **User Personas**
    -   Surveyors, mappers, agricultural professionals, and advanced drone operators.
-   **Key User Flows**
    1. Open Plan Flight view and access the Mission Area Planner panel.
    2. Adjust area parameters (center, width, height, spacing, points).
    3. Visualize the area, grid, and points on the map.
    4. Generate and review the mission.
    5. Upload the mission to the connected vehicle.
-   **UI/UX Considerations**
    -   Controls should be intuitive and responsive.
    -   Map overlays must update in real time as parameters change.
    -   Integration should feel native to QGC, matching its look and feel.

## Technical Architecture

-   **System Components**
    -   QML UI component for controls and map overlays.
    -   C++ backend (if needed) for geodesic calculations and mission logic.
    -   Integration with QGC’s mission planning/upload APIs.
-   **Data Models**
    -   Area definition: center (lat/lon), width, height.
    -   Grid parameters: line spacing, number of points.
    -   Waypoint list: generated from grid.
-   **APIs and Integrations**
    -   QGC’s QML Map and overlay types (`MapPolygon`, `MapPolyline`, etc.).
    -   QGC mission interface for uploading missions.
    -   Qt geospatial classes (`QGeoCoordinate`).
-   **Infrastructure Requirements**
    -   No external dependencies beyond Qt/QGC.
    -   Follows QGC custom build and resource override conventions.

## Development Roadmap

-   **MVP Requirements**
    1. QML UI for area/grid controls and map overlays.
    2. Geodesic/grid calculation logic in QML JS or C++.
    3. Integration with QGC mission upload (generate/upload waypoints).
    4. Resource override or plugin integration for Plan Flight view.
-   **Future Enhancements**
    -   Support for arbitrary polygonal areas.
    -   Advanced mission types (e.g., lawnmower, spiral).
    -   Save/load area templates.
    -   More advanced point/line spacing logic.
-   **Scope Pacing**
    -   Start with rectangle/grid only, then iterate.

## Logical Dependency Chain

1. Implement QML UI and controls for area/grid parameters.
2. Implement map overlays for area, grid lines, and points.
3. Implement geodesic/grid calculation logic.
4. Integrate with QGC mission upload system.
5. Polish UI/UX and test in custom build.
6. (Optional) Add advanced features and templates.

## Risks and Mitigations

-   **Technical Challenges**
    -   Porting geodesic math from Python/geopy to Qt/QML: Use Qt’s `QGeoCoordinate` and test thoroughly.
    -   Ensuring overlays and controls are performant: Profile and optimize QML as needed.
-   **MVP Definition**
    -   Focus on rectangle/grid only for first release.
-   **Resource Constraints**
    -   Leverage QGC’s plugin/resource override system to minimize upstream changes.

## Appendix

-   **Research Findings**
    -   QGC supports custom QML panels and resource overrides for UI extension.
    -   QtLocation and QGeoCoordinate provide all needed geospatial functionality.
-   **Technical Specifications**
    -   QML for UI, C++ for heavy logic if needed.
    -   Follows QGC custom build and resource override best practices.
