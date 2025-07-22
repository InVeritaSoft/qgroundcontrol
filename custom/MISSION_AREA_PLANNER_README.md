# Mission Area Planner for QGroundControl

## Overview

The Mission Area Planner is a custom QGroundControl plugin that provides an intuitive interface for creating systematic grid-based missions. It allows users to define rectangular areas and automatically generates waypoints in a grid pattern, perfect for mapping, surveying, or systematic coverage missions.

## Features

### Core Functionality

-   **Area Definition**: Define rectangular mission areas with customizable width and height
-   **Grid Generation**: Automatically generate waypoints in a systematic grid pattern
-   **Geodesic Calculations**: Accurate distance and bearing calculations using spherical trigonometry
-   **Interactive Map**: Visual representation of the area, grid lines, and waypoints
-   **Real-time Updates**: Dynamic updates as parameters change

### User Interface

-   **Mission Area Planner Panel**: Integrated into QGC's Plan View right panel
-   **Parameter Controls**: Input fields for area dimensions, line spacing, and points per line
-   **Coordinate Input**: Direct latitude/longitude input for area center
-   **Directional Controls**: N/S/E/W buttons for fine-tuning area position
-   **Status Display**: Real-time feedback on operations and calculations

### Map Integration

-   **Area Visualization**: Semi-transparent blue polygon showing the mission area
-   **Grid Lines**: Yellow lines showing the generated grid pattern
-   **Waypoint Markers**: Visual indicators for each generated waypoint
-   **Center Marker**: Red marker showing the area center point
-   **Interactive Manipulation**: Click and drag to move the area

## Architecture

### Components

1. **MissionAreaPlannerPanel.qml** - Main UI component with controls and parameter inputs
2. **MissionAreaPlanner.qml** - QML backend with geodesic calculations and grid generation
3. **MissionAreaMapOverlay.qml** - Map visualization component for area and grid display
4. **MissionAreaPlanner.h/.cc** - C++ backend class for complex calculations
5. **PlanView.qml** - Custom override integrating the planner into QGC's Plan View

### Integration Points

-   **Resource Override System**: Uses QGC's resource override mechanism for seamless integration
-   **PlanMasterController**: Integrates with QGC's mission management system
-   **Map Integration**: Connects to QGC's map system for visualization
-   **Custom Build Architecture**: Follows QGC's custom build patterns

## Installation

### Prerequisites

-   QGroundControl source code
-   Qt 6.x development environment
-   CMake build system

### Build Instructions

1. **Clone QGroundControl**:

    ```bash
    git clone https://github.com/mavlink/qgroundcontrol.git
    cd qgroundcontrol
    ```

2. **Rename Custom Directory**:

    ```bash
    mv custom-example custom
    ```

3. **Build QGroundControl**:
    ```bash
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release
    ```

### Custom Build Features

The Mission Area Planner is implemented as part of a custom QGroundControl build, which provides:

-   Custom branding and color scheme
-   Simplified UI for commercial applications
-   Advanced mode toggle for power users
-   Integrated mission planning tools

## Usage

### Basic Workflow

1. **Open Plan View**: Navigate to the Plan View in QGroundControl
2. **Access Mission Area Planner**: The planner panel appears in the right panel when in Mission mode
3. **Set Area Parameters**:

    - **Width**: Set the width of the mission area in meters
    - **Height**: Set the height of the mission area in meters
    - **Line Spacing**: Set the distance between grid lines in meters
    - **Points per Line**: Set the number of waypoints per grid line

4. **Position the Area**:

    - **Manual Input**: Enter latitude and longitude coordinates
    - **Current Location**: Click "Set Current Location" to use map center
    - **Directional Controls**: Use N/S/E/W buttons for fine adjustments
    - **Interactive Map**: Click and drag on the map to move the area

5. **Generate Mission**: Click "Generate Mission" to create waypoints in QGC's mission system

### Advanced Features

#### Geodesic Calculations

The planner uses accurate geodesic calculations for:

-   Distance measurements between coordinates
-   Bearing calculations for navigation
-   Coordinate interpolation for grid generation
-   Area corner calculations

#### Grid Generation Algorithm

1. **Area Corners**: Calculate four corners of the rectangular area
2. **Grid Lines**: Generate parallel lines across the area
3. **Waypoint Distribution**: Distribute waypoints evenly along each line
4. **Mission Integration**: Convert waypoints to QGC mission format

#### Interactive Map Features

-   **Area Polygon**: Visual representation of the mission area
-   **Grid Lines**: Yellow lines showing the generated pattern
-   **Waypoint Markers**: Individual markers for each waypoint
-   **Center Marker**: Red marker for area center
-   **Drag and Drop**: Click and drag to reposition the area

## Technical Details

### Geodesic Calculations

The planner implements the Haversine formula for accurate distance calculations:

```cpp
double geodesicDistance(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2) {
    double lat1 = coord1.latitude() * PI / 180.0;
    double lon1 = coord1.longitude() * PI / 180.0;
    double lat2 = coord2.latitude() * PI / 180.0;
    double lon2 = coord2.longitude() * PI / 180.0;

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return EARTH_RADIUS * c;
}
```

### Coordinate Calculations

For calculating coordinates at distance and bearing:

```cpp
QGeoCoordinate coordinateAtDistance(const QGeoCoordinate& referenceCoord,
                                   double distance, double bearing) {
    double lat1 = referenceCoord.latitude() * PI / 180.0;
    double lon1 = referenceCoord.longitude() * PI / 180.0;
    double brng = bearing * PI / 180.0;

    double angularDistance = distance / EARTH_RADIUS;

    double lat2 = asin(sin(lat1) * cos(angularDistance) +
                      cos(lat1) * sin(angularDistance) * cos(brng));

    double lon2 = lon1 + atan2(sin(brng) * sin(angularDistance) * cos(lat1),
                              cos(angularDistance) - sin(lat1) * sin(lat2));

    return QGeoCoordinate(lat2 * 180.0 / PI, lon2 * 180.0 / PI);
}
```

### QML Integration

The planner integrates with QGC's QML system through:

1. **Resource Overrides**: Custom QML files override default QGC components
2. **Property Binding**: Real-time updates through QML property bindings
3. **Signal Connections**: Event-driven updates for user interactions
4. **Map Integration**: Direct integration with QGC's map components

## Customization

### UI Customization

-   Modify `MissionAreaPlannerPanel.qml` for UI changes
-   Update `MissionAreaMapOverlay.qml` for map visualization changes
-   Customize colors and styling in the QML files

### Algorithm Customization

-   Modify `MissionAreaPlanner.cc` for calculation changes
-   Update grid generation logic in `updateGrid()` method
-   Customize geodesic calculations as needed

### Integration Customization

-   Modify `PlanView.qml` for different integration points
-   Update `CustomPlugin.cc` for additional QML registrations
-   Customize resource overrides in `custom.qrc`

## Troubleshooting

### Common Issues

1. **Build Errors**:

    - Ensure Qt 6.x is properly installed
    - Check that all dependencies are available
    - Verify CMake configuration

2. **UI Not Appearing**:

    - Check that the custom build is properly configured
    - Verify resource overrides are working
    - Ensure QML files are properly registered

3. **Calculations Not Working**:
    - Verify coordinate inputs are valid
    - Check that area parameters are positive numbers
    - Ensure map integration is working

### Debug Information

Enable debug logging by setting the `CustomLog` category:

```bash
export QT_LOGGING_RULES="gcs.custom.customplugin=true"
```

## Future Enhancements

### Planned Features

-   **Arbitrary Polygons**: Support for non-rectangular areas
-   **Advanced Patterns**: Spiral, zigzag, and other grid patterns
-   **Altitude Control**: Automatic altitude variation for 3D missions
-   **Mission Templates**: Save and load area configurations
-   **Batch Processing**: Generate multiple areas simultaneously

### Technical Improvements

-   **Performance Optimization**: Improve calculation speed for large areas
-   **Memory Management**: Optimize memory usage for complex grids
-   **Error Handling**: Enhanced error handling and user feedback
-   **Testing**: Comprehensive unit and integration tests

## Contributing

### Development Setup

1. Fork the QGroundControl repository
2. Create a feature branch for your changes
3. Implement your changes following QGC's coding standards
4. Test thoroughly with different area configurations
5. Submit a pull request with detailed description

### Code Standards

-   Follow QGC's existing code style and patterns
-   Use Qt 6.x APIs and best practices
-   Implement proper error handling and validation
-   Add appropriate documentation and comments
-   Include unit tests for new functionality

## License

This Mission Area Planner is part of the QGroundControl custom build and follows the same licensing terms as QGroundControl itself. See the main QGroundControl repository for license details.

## Support

For issues and questions:

1. Check the troubleshooting section above
2. Review QGroundControl documentation
3. Search existing GitHub issues
4. Create a new issue with detailed information

## Acknowledgments

-   QGroundControl development team for the excellent platform
-   Qt development team for the powerful QML framework
-   Open source community for geodesic calculation algorithms
-   Contributors and testers who provided feedback and improvements
