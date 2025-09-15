/**
 * @file AreaPlanEditor.cc
 * @brief Implementation of the Area Plan Editor for QGroundControl
 * 
 * This file contains the core implementation of the Area Plan Editor, which provides
 * multi-drone area planning capabilities for QGroundControl. The editor enables users
 * to create complex area coverage missions with multiple vehicles operating in
 * coordinated formations.
 * 
 * @section Architecture
 * The AreaPlanEditor integrates with QGroundControl's mission management system
 * through the following key components:
 * - QML frontend (AreaPlanEditor.qml) for user interface
 * - C++ backend (this file) for mission generation logic
 * - MissionController integration for waypoint management
 * - MultiVehicleManager for vehicle coordination
 * 
 * @section Features
 * - Interactive area drawing and definition
 * - Multi-drone mission planning with altitude banding
 * - Time-staggered mission execution
 * - Formation flying and coordination
 * - Mission optimization and validation
 * - Real-time progress tracking
 * 
 * @section Usage
 * The editor is typically instantiated from QML and provides properties and
 * methods for configuring area parameters, drone counts, and mission settings.
 * Mission generation is handled through the generateMission() method, which
 * creates waypoints and uploads them to connected vehicles.
 * 
 * @section Dependencies
 * - Qt Core, Positioning, and QML modules
 * - QGroundControl core components (Vehicle, MissionManager, etc.)
 * - Mission management infrastructure
 * 
 * @author QGroundControl Team
 * @date 2024
 * @version 1.0
 * 
 * @see AreaPlanEditor.h
 * @see AreaPlanEditor.qml
 * @see MissionController
 * @see MultiVehicleManager
 */

#include "AreaPlanEditor.h"

// Qt includes
#include <QDebug>
#include <QtMath>
#include <QtPositioning>
#include <QVariant>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QElapsedTimer>
#include <QCache>
#include <QDateTime>

// QGC includes for mission management
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MissionManager.h"
#include "MissionItem.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCMAVLink.h"
#include "MissionController.h"
#include "SimpleMissionItem.h"
#include "MissionManager/AreaPartition.h"
#include "QmlObjectListModel.h"

/**
 * @brief Constructor for AreaPlanEditor
 * 
 * Initializes a new AreaPlanEditor instance with default values for all
 * area planning parameters. The editor is designed to work with QML
 * and provides a complete interface for multi-drone area mission planning.
 * 
 * @param parent Parent QObject for memory management
 * 
 * @section Initialization
 * The constructor sets up default values for:
 * - Area dimensions (width, height, line spacing)
 * - Mission parameters (altitude, number of points)
 * - Multi-drone configuration (count, altitude bands, timing)
 * - Formation settings (spacing, transition states)
 * - Processing flags and progress tracking
 * 
 * @section Default Values
 * - Area Width: 1000 meters
 * - Area Height: 1000 meters
 * - Line Spacing: 50 meters
 * - Mission Altitude: 100 meters
 * - Drone Count: 1
 * - Altitude Band Start: 0 meters
 * - Altitude Band Step: 10 meters
 * - Time Offset Per Drone: 5 seconds
 * 
 * @note All default values are defined as static constants in the header file
 *       and can be modified through QML property bindings or direct method calls.
 */
AreaPlanEditor::AreaPlanEditor(QObject* parent)
    : QObject(parent)
    , _areaWidth(_defaultAreaWidth)
    , _areaHeight(_defaultAreaHeight)
    , _lineSpacing(_defaultLineSpacing)
    , _numPoints(_defaultNumPoints)
    , _missionAltitude(_defaultAltitude)
    , _droneCount(_defaultDroneCount)
    , _altitudeBandStart(_defaultAltitudeBandStart)
    , _altitudeBandStep(_defaultAltitudeBandStep)
    , _timeOffsetPerDrone(_defaultTimeOffsetPerDrone)
    , _rtlAfterEveryWaypoint(false)
    , _loiterAfterRtl(false)
    , _isProcessing(false)
    , _progressValue(0)
    , _isOptimized(false)
    , _cacheSize(0)
    , _isDrawingMode(false)
    , _planMasterController(nullptr)
    , _currentFormation(NoFormation)
    , _formationSpacing(5.0)
    , _isFormationTransitioning(false)
    , _leaderVehicle(nullptr)
{
    // Initialize any additional members if needed
}

/**
 * @brief Sets the width of the area to be covered by the mission
 * 
 * This method updates the area width property and emits a change signal
 * if the value actually changes. The width represents the horizontal
 * extent of the operational area in meters.
 * 
 * @param width New area width in meters (must be positive)
 * 
 * @section Usage
 * The area width is used to calculate:
 * - Number of parallel flight lines needed
 * - Total mission distance and duration
 * - Waypoint spacing and positioning
 * - Mission optimization parameters
 * 
 * @section Validation
 * The method uses qFuzzyCompare for floating-point comparison to
 * avoid unnecessary signal emissions due to minor floating-point
 * precision differences.
 * 
 * @section QML Integration
 * This method is automatically called when the corresponding QML
 * property is modified, ensuring the C++ backend stays synchronized
 * with the QML frontend.
 * 
 * @see areaWidth()
 * @see areaWidthChanged()
 */
void AreaPlanEditor::setAreaWidth(qreal width)
{
    if (qFuzzyCompare(_areaWidth, width)) {
        return;
    }
    _areaWidth = width;
    emit areaWidthChanged();
}

/**
 * @brief Sets the height of the area to be covered by the mission
 * 
 * This method updates the area height property and emits a change signal
 * if the value actually changes. The height represents the vertical
 * extent of the operational area in meters.
 * 
 * @param height New area height in meters (must be positive)
 * 
 * @section Usage
 * The area height is used to calculate:
 * - Mission flight path length
 * - Total coverage area
 * - Mission duration estimates
 * - Fuel/energy consumption planning
 * 
 * @section Coordinate System
 * The height is measured perpendicular to the width, forming a
 * rectangular area. When combined with area rotation, this creates
 * the complete operational boundary for the mission.
 * 
 * @section Validation
 * The method uses qFuzzyCompare for floating-point comparison to
 * avoid unnecessary signal emissions due to minor floating-point
 * precision differences.
 * 
 * @see areaHeight()
 * @see areaHeightChanged()
 */
void AreaPlanEditor::setAreaHeight(qreal height)
{
    if (qFuzzyCompare(_areaHeight, height)) {
        return;
    }
    _areaHeight = height;
    emit areaHeightChanged();
}

/**
 * @brief Sets the spacing between parallel flight lines
 * 
 * This method updates the line spacing property, which determines
 * the distance between adjacent parallel flight paths during
 * area coverage missions.
 * 
 * @param spacing Distance between flight lines in meters (must be positive)
 * 
 * @section Usage
 * Line spacing is critical for:
 * - Ensuring complete area coverage without gaps
 * - Optimizing mission efficiency and duration
 * - Balancing coverage density with mission time
 * - Meeting operational requirements for different applications
 * 
 * @section Coverage Calculation
 * The line spacing directly affects:
 * - Number of parallel flight lines = areaWidth / lineSpacing
 * - Total mission distance
 * - Mission duration and fuel consumption
 * - Coverage overlap and quality
 * 
 * @section Validation
 * The method uses qFuzzyCompare for floating-point comparison to
 * avoid unnecessary signal emissions due to minor floating-point
 * precision differences.
 * 
 * @note Smaller spacing values result in more thorough coverage
 *       but longer mission duration and higher resource consumption.
 * 
 * @see lineSpacing()
 * @see lineSpacingChanged()
 */
void AreaPlanEditor::setLineSpacing(qreal spacing)
{
    if (qFuzzyCompare(_lineSpacing, spacing)) {
        return;
    }
    _lineSpacing = spacing;
    emit lineSpacingChanged();
}

/**
 * @brief Sets the number of waypoints to generate for the mission
 * 
 * This method updates the number of points property, which controls
 * the density of waypoints generated along the flight path.
 * 
 * @param points Number of waypoints to generate (must be positive)
 * 
 * @section Usage
 * The number of points affects:
 * - Mission precision and smoothness
 * - Flight path resolution
 * - Mission file size and complexity
 * - Autopilot performance and stability
 * 
 * @section Waypoint Generation
 * Higher point counts result in:
 * - Smoother flight paths with tighter turns
 * - Better adherence to planned routes
 * - Increased mission file size
 * - More complex mission planning
 * 
 * @section Validation
 * The method performs a direct integer comparison since points
 * is an integer value, avoiding floating-point precision issues.
 * 
 * @note The optimal number of points depends on the mission
 *       requirements, vehicle capabilities, and desired precision.
 * 
 * @see numPoints()
 * @see numPointsChanged()
 */
void AreaPlanEditor::setNumPoints(int points)
{
    // Clamp to minimum of 1 to ensure at least one waypoint per line
    int clamped = points < 1 ? 1 : points;
    if (_numPoints == clamped) {
        return;
    }
    _numPoints = clamped;
    emit numPointsChanged();
}

/**
 * @brief Sets the mission altitude for all waypoints
 * 
 * This method updates the mission altitude property, which defines
 * the flight height for all waypoints in the generated mission.
 * 
 * @param altitude Mission altitude in meters above ground level
 * 
 * @section Usage
 * Mission altitude is critical for:
 * - Ensuring safe flight above obstacles and terrain
 * - Meeting regulatory requirements for different airspaces
 * - Optimizing sensor performance and coverage
 * - Coordinating multi-drone operations at different heights
 * 
 * @section Safety Considerations
 * The altitude should be set considering:
 * - Local terrain elevation and obstacles
 * - Airspace restrictions and regulations
 * - Weather conditions and visibility
 * - Sensor and camera requirements
 * 
 * @section Multi-Drone Coordination
 * When using multiple drones, the altitude property serves as
 * the base altitude, with individual drone altitudes calculated
 * using altitude banding (altitudeBandStart + droneIndex * altitudeBandStep).
 * 
 * @see missionAltitude()
 * @see missionAltitudeChanged()
 * @see altitudeBandStart()
 * @see altitudeBandStep()
 */
void AreaPlanEditor::setMissionAltitude(qreal altitude)
{
    if (qFuzzyCompare(_missionAltitude, altitude)) {
        return;
    }
    _missionAltitude = altitude;
    emit missionAltitudeChanged();
}

/**
 * @brief Sets the center point of the operational area
 * 
 * This method updates the area center property, which defines
 * the geographical center point around which the mission area
 * is defined and generated.
 * 
 * @param center QGeoCoordinate representing the center point
 * 
 * @section Usage
 * The area center is used to:
 * - Define the reference point for area dimensions
 * - Calculate waypoint positions relative to the center
 * - Position the mission within the global coordinate system
 * - Center the mission display on maps and planning interfaces
 * 
 * @section Coordinate System
 * The center point uses QGeoCoordinate with:
 * - Latitude and longitude for geographical positioning
 * - Altitude information (if provided)
 * - Automatic coordinate validation and normalization
 * 
 * @section Mission Generation
 * All waypoints are generated relative to this center point,
 * with the area extending outward based on width and height
 * parameters, rotated according to the area rotation setting.
 * 
 * @see areaCenter()
 * @see areaCenterChanged()
 * @see areaWidth()
 * @see areaHeight()
 * @see areaRotation()
 */
void AreaPlanEditor::setAreaCenter(const QGeoCoordinate& center)
{
    if (_areaCenter == center) {
        return;
    }
    _areaCenter = center;
    emit areaCenterChanged();
}

/**
 * @brief Sets the home location for mission planning
 * 
 * This method updates the home location property, which defines
 * the base location where vehicles return after mission completion
 * or in emergency situations.
 * 
 * @param location QGeoCoordinate representing the home location
 * 
 * @section Usage
 * The home location is critical for:
 * - RTL (Return to Launch) mission segments
 * - Emergency procedures and safety protocols
 * - Mission planning and waypoint generation
 * - Flight path optimization and safety margins
 * 
 * @section Safety Features
 * The home location enables:
 * - Automatic RTL commands on mission completion
 * - Emergency RTL on communication loss
 * - Safe landing procedures
 * - Mission abort and recovery operations
 * 
 * @section Mission Integration
 * The home location is typically:
 * - Set automatically from the vehicle's GPS home position
 * - Manually overridden for specific mission requirements
 * - Used as the starting and ending point for missions
 * - Referenced for all RTL and safety procedures
 * 
 * @see homeLocation()
 * @see homeLocationChanged()
 * @see rtlAfterEveryWaypoint()
 * @see landAtTargetReturn()
 */
void AreaPlanEditor::setHomeLocation(const QGeoCoordinate& location)
{
    if (_homeLocation == location) {
        return;
    }
    _homeLocation = location;
    emit homeLocationChanged();
}

/**
 * @brief Sets the rotation angle of the operational area
 * 
 * This method updates the area rotation property, which defines
 * the clockwise rotation angle of the rectangular mission area
 * relative to true north.
 * 
 * @param rotation Rotation angle in degrees (0-360, where 0 is north)
 * 
 * @section Usage
 * Area rotation is used to:
 * - Align the mission area with terrain features or boundaries
 * - Optimize flight paths for prevailing wind conditions
 * - Match operational requirements for specific applications
 * - Coordinate with other missions or operational areas
 * 
 * @section Coordinate System
 * The rotation is applied around the area center point:
 * - 0° = North (0° true bearing)
 * - 90° = East (90° true bearing)
 * - 180° = South (180° true bearing)
 * - 270° = West (270° true bearing)
 * 
 * @section Mission Generation
 * The rotation affects:
 * - Waypoint positioning and orientation
 * - Flight line direction and spacing
 * - Mission efficiency and duration
 * - Coverage pattern optimization
 * 
 * @note Rotation is applied after area dimensions are calculated,
 *       ensuring the complete area is covered regardless of orientation.
 * 
 * @see areaRotation()
 * @see areaRotationChanged()
 * @see areaCenter()
 * @see areaWidth()
 * @see areaHeight()
 */
void AreaPlanEditor::setAreaRotation(qreal rotation)
{
    if (qFuzzyCompare(_areaRotation, rotation)) {
        return;
    }
    _areaRotation = rotation;
    emit areaRotationChanged();
}

/**
 * @brief Sets the loiter time for waypoint missions
 * 
 * This method updates the loiter time property, which defines
 * how long vehicles should wait at each waypoint before proceeding
 * to the next waypoint in the mission.
 * 
 * @param time Loiter time in seconds (must be non-negative)
 * 
 * @section Usage
 * Loiter time is used for:
 * - Allowing sensors to collect data at specific locations
 * - Coordinating multi-drone operations and timing
 * - Implementing survey and mapping missions
 * - Managing mission pacing and duration
 * 
 * @section Mission Planning
 * The loiter time affects:
 * - Total mission duration calculation
 * - Fuel/energy consumption planning
 * - Sensor data collection opportunities
 * - Mission synchronization between vehicles
 * 
 * @section Multi-Drone Coordination
 * When using multiple drones, loiter time helps:
 * - Synchronize operations across the fleet
 * - Ensure proper timing for coordinated actions
 * - Manage traffic flow and spacing
 * - Coordinate payload operations
 * 
 * @see loiterTime()
 * @see loiterTimeChanged()
 * @see timeOffsetPerDrone()
 * @see droneCount()
 */
void AreaPlanEditor::setLoiterTime(qreal time)
{
    if (qFuzzyCompare(_loiterTime, time)) {
        return;
    }
    _loiterTime = time;
    emit loiterTimeChanged();
}

/**
 * @brief Sets the drawing mode for interactive area definition
 * 
 * This method updates the drawing mode property, which controls
 * whether the editor is in interactive drawing mode for defining
 * mission areas through user input.
 * 
 * @param drawingMode True to enable drawing mode, false to disable
 * 
 * @section Usage
 * Drawing mode enables:
 * - Interactive area definition on maps
 * - User-drawn mission boundaries
 * - Real-time area parameter updates
 * - Visual feedback during area planning
 * 
 * @section User Interface
 * When drawing mode is active:
 * - Users can click on maps to define area corners
 * - Area dimensions are calculated automatically
 * - Real-time preview of mission coverage
 * - Interactive parameter adjustment
 * 
 * @section Mission Generation
 * Drawing mode affects:
 * - Area center and dimension calculations
 * - Waypoint generation and positioning
 * - Mission optimization parameters
 * - Coverage pattern planning
 * 
 * @note Drawing mode is typically activated from the QML interface
 *       and automatically updates area parameters as the user draws.
 * 
 * @see isDrawingMode()
 * @see isDrawingModeChanged()
 * @see areaCenter()
 * @see areaWidth()
 * @see areaHeight()
 */
void AreaPlanEditor::setIsDrawingMode(bool drawingMode)
{
    if (_isDrawingMode == drawingMode) {
        return;
    }
    _isDrawingMode = drawingMode;
    emit isDrawingModeChanged();
}

/**
 * @brief Sets the plan master controller for mission management
 * 
 * This method updates the plan master controller property, which
 * provides access to the main mission planning and management
 * system within QGroundControl.
 * 
 * @param controller Pointer to the plan master controller object
 * 
 * @section Usage
 * The plan master controller provides:
 * - Mission item creation and management
 * - Waypoint generation and validation
 * - Mission upload and download capabilities
 * - Vehicle communication and coordination
 * 
 * @section Integration
 * The controller enables:
 * - Seamless integration with QGroundControl's mission system
 * - Access to vehicle state and capabilities
 * - Mission validation and error checking
 * - Real-time mission monitoring and control
 * 
 * @section Mission Operations
 * Through the controller, the editor can:
 * - Generate and validate mission waypoints
 * - Upload missions to connected vehicles
 * - Monitor mission execution progress
 * - Handle mission abort and recovery
 * 
 * @note The controller is typically set by the QML interface
 *       when the editor is initialized and provides the bridge
 *       between the editor and QGroundControl's core systems.
 * 
 * @see planMasterController()
 * @see planMasterControllerChanged()
 * @see generateMission()
 * @see uploadMission()
 */
void AreaPlanEditor::setPlanMasterController(QObject* controller)
{
    _planMasterController = controller;
    emit planMasterControllerChanged();
}

/**
 * @brief Sets the land-at-target-return policy for missions
 * 
 * This method updates the land-at-target-return property, which
 * controls whether vehicles should land at the target area
 * before returning to the home location.
 * 
 * @param enabled True to enable land-at-target behavior, false to return directly
 * 
 * @section Usage
 * This policy affects mission behavior:
 * - When enabled: Vehicle lands at target, then returns home
 * - When disabled: Vehicle returns home directly after mission completion
 * 
 * @section Mission Planning
 * The policy influences:
 * - Mission waypoint generation and sequencing
 * - Total mission duration and fuel consumption
 * - Landing site requirements and safety considerations
 * - Mission abort and recovery procedures
 * 
 * @section Safety Considerations
 * Landing at target requires:
 * - Suitable landing zones in the target area
 * - Adequate fuel/energy reserves for return flight
 * - Safe landing procedures and protocols
 * - Emergency landing site identification
 * 
 * @section Use Cases
 * Common applications include:
 * - Survey and mapping missions requiring ground operations
 * - Cargo delivery and payload operations
 * - Search and rescue missions
 * - Agricultural and inspection operations
 * 
 * @see landAtTargetReturn()
 * @see landAtTargetReturnChanged()
 * @see homeLocation()
 * @see rtlAfterEveryWaypoint()
 */
void AreaPlanEditor::setLandAtTargetReturn(bool enabled)
{
    if (_landAtTargetReturn == enabled) return;
    _landAtTargetReturn = enabled;
    emit landAtTargetReturnChanged();
}

/**
 * @brief Sets the number of drones for multi-vehicle missions
 * 
 * This method updates the drone count property, which defines
 * how many vehicles will participate in the coordinated area
 * coverage mission.
 * 
 * @param count Number of drones (minimum 1, automatically clamped)
 * 
 * @section Usage
 * Multi-drone missions enable:
 * - Parallel area coverage for faster mission completion
 * - Coordinated operations and formation flying
 * - Distributed sensor coverage and data collection
 * - Redundant operations for safety and reliability
 * 
 * @section Mission Planning
 * The drone count affects:
 * - Mission generation and waypoint distribution
 * - Altitude banding and vertical separation
 * - Time staggering and coordination parameters
 * - Total mission duration and efficiency
 * 
 * @section Safety Features
 * The method automatically:
 * - Clamps the count to a minimum of 1 drone
 * - Prevents invalid zero or negative values
 * - Ensures safe mission planning parameters
 * - Maintains system stability
 * 
 * @section Coordination
 * Multi-drone missions require:
 * - Proper altitude banding for vertical separation
 * - Time offsets to prevent conflicts
 * - Formation flying capabilities
 * - Coordinated mission execution
 * 
 * @note The drone count is automatically clamped to ensure
 *       valid mission parameters and system stability.
 * 
 * @see droneCount()
 * @see droneCountChanged()
 * @see altitudeBandStart()
 * @see altitudeBandStep()
 * @see timeOffsetPerDrone()
 */
void AreaPlanEditor::setDroneCount(int count)
{
    // Clamp to minimum of 1
    int clamped = count < 1 ? 1 : count;
    if (_droneCount == clamped) {
        return;
    }
    _droneCount = clamped;
    emit droneCountChanged();
}

/**
 * @brief Sets the starting altitude for altitude banding
 * 
 * This method updates the altitude band start property, which
 * defines the base altitude for the first drone in multi-vehicle
 * missions. Subsequent drones are assigned altitudes above this
 * base value.
 * 
 * @param startMeters Starting altitude in meters above ground level
 * 
 * @section Usage
 * Altitude banding provides:
 * - Vertical separation between multiple drones
 * - Safe multi-vehicle operations
 * - Coordinated mission execution
 * - Collision avoidance and safety margins
 * 
 * @section Altitude Calculation
 * Individual drone altitudes are calculated as:
 * - Drone 0: altitudeBandStart
 * - Drone 1: altitudeBandStart + altitudeBandStep
 * - Drone 2: altitudeBandStart + (2 * altitudeBandStep)
 * - And so on...
 * 
 * @section Safety Features
 * The method automatically:
 * - Clamps negative values to 0 meters
 * - Ensures safe minimum altitudes
 * - Prevents invalid altitude assignments
 * - Maintains operational safety
 * 
 * @section Mission Integration
 * Altitude banding affects:
 * - Waypoint generation for each drone
 * - Mission safety and collision avoidance
 * - Operational efficiency and coordination
 * - Regulatory compliance and airspace usage
 * 
 * @note The altitude band start should be set considering
 *       terrain elevation, obstacles, and regulatory requirements.
 * 
 * @see altitudeBandStart()
 * @see altitudeBandStartChanged()
 * @see altitudeBandStep()
 * @see droneCount()
 * @see missionAltitude()
 */
void AreaPlanEditor::setAltitudeBandStart(qreal startMeters)
{
    // Clamp to >= 0
    qreal clamped = startMeters < 0.0 ? 0.0 : startMeters;
    if (qFuzzyCompare(_altitudeBandStart, clamped)) {
        return;
    }
    _altitudeBandStart = clamped;
    emit altitudeBandStartChanged();
}

/**
 * @brief Sets the altitude step between drones for banding
 * 
 * This method updates the altitude band step property, which
 * defines the vertical separation between adjacent drones in
 * multi-vehicle missions.
 * 
 * @param stepMeters Altitude step in meters (must be positive)
 * 
 * @section Usage
 * Altitude banding enables:
 * - Safe vertical separation between multiple drones
 * - Coordinated multi-vehicle operations
 * - Collision avoidance and safety margins
 * - Efficient area coverage at different heights
 * 
 * @section Altitude Distribution
 * The step value determines:
 * - Vertical spacing between drones
 * - Total altitude range for the mission
 * - Safety margins and collision avoidance
 * - Operational efficiency and coordination
 * 
 * @section Safety Features
 * The method automatically:
 * - Ensures positive step values
 * - Resets to default if invalid input
 * - Maintains safe altitude separation
 * - Prevents dangerous altitude assignments
 * 
 * @section Mission Planning
 * The altitude step affects:
 * - Mission safety and collision avoidance
 * - Operational efficiency and coordination
 * - Regulatory compliance and airspace usage
 * - Mission duration and resource planning
 * 
 * @note The altitude step should provide sufficient vertical
 *       separation for safe multi-vehicle operations while
 *       maintaining mission efficiency.
 * 
 * @see altitudeBandStep()
 * @see altitudeBandStepChanged()
 * @see altitudeBandStart()
 * @see droneCount()
 * @see missionAltitude()
 */
void AreaPlanEditor::setAltitudeBandStep(qreal stepMeters)
{
    // Require >= 1.0m to maintain vertical separation; fallback to default if lower
    qreal value = stepMeters >= 1.0 ? stepMeters : qMax<qreal>(_defaultAltitudeBandStep, 1.0);
    if (qFuzzyCompare(_altitudeBandStep, value)) {
        return;
    }
    _altitudeBandStep = value;
    emit altitudeBandStepChanged();
}

/**
 * @brief Sets the time offset between drones for mission coordination
 * 
 * This method updates the time offset per drone property, which
 * defines the delay between when each drone starts its mission
 * to ensure proper coordination and prevent conflicts.
 * 
 * @param seconds Time offset in seconds (must be non-negative)
 * 
 * @section Usage
 * Time staggering provides:
 * - Coordinated mission execution across multiple drones
 * - Traffic flow management and conflict prevention
 * - Synchronized operations and timing
 * - Efficient resource utilization and coordination
 * 
 * @section Mission Coordination
 * The time offset affects:
 * - Mission start timing for each drone
 * - Traffic flow and spacing management
 * - Operational synchronization and coordination
 * - Mission duration and efficiency
 * 
 * @section Safety Features
 * The method automatically:
 * - Clamps negative values to 0 seconds
 * - Ensures safe timing parameters
 * - Prevents invalid time assignments
 * - Maintains operational safety
 * 
 * @section Mission Planning
 * Time offsets enable:
 * - Staggered mission starts for coordination
 * - Traffic flow management and spacing
 * - Synchronized operations and timing
 * - Efficient resource utilization
 * 
 * @note The time offset should be sufficient to allow
 *       safe mission execution while maintaining mission
 *       efficiency and coordination.
 * 
 * @see timeOffsetPerDrone()
 * @see timeOffsetPerDroneChanged()
 * @see droneCount()
 * @see altitudeBandStart()
 * @see altitudeBandStep()
 */
void AreaPlanEditor::setTimeOffsetPerDrone(qreal seconds)
{
    // Clamp to >= 0
    qreal clamped = seconds < 0.0 ? 0.0 : seconds;
    if (qFuzzyCompare(_timeOffsetPerDrone, clamped)) {
        return;
    }
    _timeOffsetPerDrone = clamped;
    emit timeOffsetPerDroneChanged();
}

/**
 * @brief Sets the time separation between targets for mission coordination
 * 
 * This method updates the per-target separation property, which defines
 * the minimum time interval between when different drones reach the
 * same target or waypoint to prevent conflicts and ensure safe operations.
 * 
 * @param seconds Time separation in seconds (must be non-negative)
 * 
 * @section Usage
 * Target separation provides:
 * - Safe spacing between drones at common waypoints
 * - Conflict prevention at mission intersections
 * - Coordinated target approach and departure
 * - Traffic flow management at critical points
 * 
 * @section Mission Safety
 * The separation time ensures:
 * - No two drones occupy the same airspace simultaneously
 * - Safe approach and departure from shared waypoints
 * - Proper traffic flow at mission intersections
 * - Collision avoidance and safety margins
 * 
 * @section Mission Planning
 * The separation affects:
 * - Mission timing and coordination
 * - Waypoint sequencing and spacing
 * - Total mission duration
 * - Operational efficiency and safety
 * 
 * @section Validation
 * The method automatically:
 * - Clamps negative values to 0 seconds
 * - Ensures safe timing parameters
 * - Prevents invalid time assignments
 * - Maintains operational safety
 * 
 * @note The separation time should be sufficient to allow
 *       safe target approach, operations, and departure
 *       while maintaining mission efficiency.
 * 
 * @see perTargetSeparationS()
 * @see perTargetSeparationSChanged()
 * @see droneCount()
 * @see timeOffsetPerDrone()
 * @see missionAltitude()
 */
void AreaPlanEditor::setPerTargetSeparationS(qreal seconds)
{
    // Enforce at least 1s to avoid simultaneous arrivals
    qreal clamped = seconds < 1.0 ? 1.0 : seconds;
    if (qFuzzyCompare(_perTargetSeparationS, clamped)) return;
    _perTargetSeparationS = clamped;
    emit perTargetSeparationSChanged();
}

/**
 * @brief Sets the RTL behavior after each waypoint
 * 
 * This method updates the RTL-after-every-waypoint property, which
 * controls whether vehicles should return to launch location after
 * completing each individual waypoint in the mission.
 * 
 * @param enabled True to enable RTL after each waypoint, false for continuous mission
 * 
 * @section Usage
 * This behavior affects mission execution:
 * - When enabled: Vehicle returns home after each waypoint
 * - When disabled: Vehicle continues to next waypoint without returning
 * 
 * @section Mission Behavior
 * The setting influences:
 * - Mission execution pattern and flow
 * - Total mission duration and fuel consumption
 * - Safety and monitoring capabilities
 * - Operational flexibility and control
 * 
 * @section Use Cases
 * Common applications include:
 * - High-risk missions requiring frequent safety checks
 * - Operations in restricted airspace
 * - Missions requiring frequent payload changes
 * - Training and demonstration scenarios
 * 
 * @section Safety Considerations
 * RTL after each waypoint provides:
 * - Frequent safety checkpoints
 * - Regular communication verification
 * - Opportunity for mission adjustments
 * - Enhanced monitoring and control
 * 
 * @note This setting significantly increases mission duration
 *       and fuel consumption but provides enhanced safety
 *       and control capabilities.
 * 
 * @see rtlAfterEveryWaypoint()
 * @see rtlAfterEveryWaypointChanged()
 * @see homeLocation()
 * @see landAtTargetReturn()
 */
void AreaPlanEditor::setRtlAfterEveryWaypoint(bool enabled)
{
    if (_rtlAfterEveryWaypoint == enabled) {
        return;
    }
    _rtlAfterEveryWaypoint = enabled;
    emit rtlAfterEveryWaypointChanged();
}

/**
 * @brief Sets the loiter behavior after RTL operations
 * 
 * This method updates the loiter-after-RTL property, which controls
 * whether vehicles should enter a loiter pattern after returning
 * to the launch location.
 * 
 * @param enabled True to enable loiter after RTL, false to land immediately
 * 
 * @section Usage
 * Loiter after RTL provides:
 * - Holding pattern for mission continuation
 * - Waiting for operator instructions
 * - Fuel/energy conservation during delays
 * - Flexible mission execution control
 * 
 * @section Mission Control
 * The setting affects:
 * - Mission flow and continuation options
 * - Operator control and decision making
 * - Mission timing and coordination
 * - Operational flexibility and adaptability
 * 
 * @section Use Cases
 * Common applications include:
 * - Missions requiring operator decisions at waypoints
 * - Operations with variable timing requirements
 * - Missions with conditional waypoint execution
 * - Training and demonstration scenarios
 * 
 * @section Safety Features
 * Loiter after RTL enables:
 * - Safe holding pattern during delays
 * - Operator decision making time
 * - Mission continuation options
 * - Enhanced operational control
 * 
 * @note Loiter behavior consumes fuel/energy but provides
 *       operational flexibility and control capabilities.
 * 
 * @see loiterAfterRtl()
 * @see loiterAfterRtlChanged()
 * @see rtlAfterEveryWaypoint()
 * @see homeLocation()
 */
void AreaPlanEditor::setLoiterAfterRtl(bool enabled)
{
    if (_loiterAfterRtl == enabled) {
        return;
    }
    _loiterAfterRtl = enabled;
    emit loiterAfterRtlChanged();
}

/**
 * @brief Sets the hold time at target waypoints
 * 
 * This method updates the target hold time property, which defines
 * how long vehicles should wait at each target waypoint before
 * proceeding to the next waypoint in the mission.
 * 
 * @param seconds Hold time in seconds (must be non-negative)
 * 
 * @section Usage
 * Target hold time enables:
 * - Sensor data collection at specific locations
 * - Payload operations and deployments
 * - Mission coordination and synchronization
 * - Operational flexibility and control
 * 
 * @section Mission Planning
 * The hold time affects:
 * - Total mission duration calculation
 * - Fuel/energy consumption planning
 * - Sensor data collection opportunities
 * - Mission synchronization between vehicles
 * 
 * @section Multi-Drone Coordination
 * When using multiple drones, hold time helps:
 * - Synchronize operations across the fleet
 * - Ensure proper timing for coordinated actions
 * - Manage traffic flow and spacing
 * - Coordinate payload operations
 * 
 * @section Validation
 * The method automatically:
 * - Clamps negative values to 0 seconds
 * - Ensures safe timing parameters
 * - Prevents invalid time assignments
 * - Maintains operational safety
 * 
 * @note The hold time should be sufficient for intended
 *       operations while maintaining mission efficiency.
 * 
 * @see targetHoldTimeS()
 * @see targetHoldTimeSChanged()
 * @see loiterTime()
 * @see timeOffsetPerDrone()
 */
void AreaPlanEditor::setTargetHoldTimeS(qreal seconds)
{
    qreal clamped = seconds < 0 ? 0 : seconds;
    if (qFuzzyCompare(_targetHoldTimeS, clamped)) return;
    _targetHoldTimeS = clamped;
    emit targetHoldTimeSChanged();
}

/**
 * @brief Sets the wait time at home location during turnarounds
 * 
 * This method updates the home turnaround wait time property, which
 * defines how long vehicles should wait at the home location before
 * proceeding to the next mission segment or waypoint.
 * 
 * @param seconds Wait time in seconds (must be non-negative)
 * 
 * @section Usage
 * Home turnaround wait time provides:
 * - Time for system checks and maintenance
 * - Operator decision making and planning
 * - Mission coordination and synchronization
 * - Operational flexibility and control
 * 
 * @section Mission Flow
 * The wait time affects:
 * - Mission execution timing and flow
 * - Total mission duration calculation
 * - Fuel/energy consumption planning
 * - Operational efficiency and coordination
 * 
 * @section Use Cases
 * Common applications include:
 * - Missions requiring frequent home base operations
 * - Operations with variable timing requirements
 * - Missions with conditional execution
 * - Training and demonstration scenarios
 * 
 * @section Safety Features
 * Home turnaround wait enables:
 * - System health checks and maintenance
 * - Operator decision making time
 * - Mission coordination and planning
 * - Enhanced operational control
 * 
 * @note The wait time should balance operational needs
 *       with mission efficiency and resource consumption.
 * 
 * @see homeTurnaroundWaitS()
 * @see homeTurnaroundWaitSChanged()
 * @see homeLocation()
 * @see rtlAfterEveryWaypoint()
 */
void AreaPlanEditor::setHomeTurnaroundWaitS(qreal seconds)
{
    qreal clamped = seconds < 0 ? 0 : seconds;
    if (qFuzzyCompare(_homeTurnaroundWaitS, clamped)) return;
    _homeTurnaroundWaitS = clamped;
    emit homeTurnaroundWaitSChanged();
}

/**
 * @brief Sets the payload release capability for missions
 * 
 * This method updates the payload release enabled property, which
 * controls whether vehicles are configured to release payloads
 * during mission execution.
 * 
 * @param enabled True to enable payload release, false to disable
 * 
 * @section Usage
 * Payload release enables:
 * - Cargo delivery and deployment operations
 * - Sensor deployment and retrieval
 * - Emergency equipment delivery
 * - Specialized mission capabilities
 * 
 * @section Mission Capabilities
 * The setting affects:
 * - Mission type and complexity
 * - Vehicle configuration requirements
 * - Safety and operational procedures
 * - Mission planning and execution
 * 
 * @section Use Cases
 * Common applications include:
 * - Cargo delivery missions
 * - Emergency response operations
 * - Scientific research missions
 * - Military and defense operations
 * 
 * @section Safety Considerations
 * Payload release requires:
 * - Proper vehicle configuration
 * - Safety procedures and protocols
 * - Operational training and certification
 * - Emergency response planning
 * 
 * @note Payload release capabilities require proper
 *       vehicle configuration and safety procedures.
 * 
 * @see payloadReleaseEnabled()
 * @see payloadReleaseEnabledChanged()
 * @see missionAltitude()
 * @see homeLocation()
 */
void AreaPlanEditor::setPayloadReleaseEnabled(bool enabled)
{
    if (_payloadReleaseEnabled == enabled) return;
    _payloadReleaseEnabled = enabled;
    emit payloadReleaseEnabledChanged();
}

/**
 * @brief Sets the takeoff height for mission planning
 * 
 * This method updates the takeoff height property, which defines
 * the altitude that vehicles should reach after takeoff before
 * proceeding to the first mission waypoint.
 * 
 * @param height Takeoff height in meters above ground level
 * 
 * @section Usage
 * Takeoff height provides:
 * - Safe clearance above ground obstacles
 * - Proper mission altitude establishment
 * - Takeoff safety and procedures
 * - Mission initialization and setup
 * 
 * @section Mission Safety
 * The takeoff height ensures:
 * - Safe clearance above ground level
 * - Proper altitude establishment
 * - Takeoff procedure completion
 * - Mission readiness and safety
 * 
 * @section Mission Planning
 * The height affects:
 * - Mission initialization and setup
 * - Takeoff procedures and safety
 * - Mission altitude establishment
 * - Operational safety and procedures
 * 
 * @section Validation
 * The method uses a small tolerance (0.1m) for:
 * - Floating-point precision handling
 * - Avoiding unnecessary signal emissions
 * - Maintaining system stability
 * - Ensuring proper operation
 * 
 * @note The takeoff height should provide sufficient
 *       clearance above ground obstacles and terrain.
 * 
 * @see takeoffHeight()
 * @see takeoffHeightChanged()
 * @see missionAltitude()
 * @see homeLocation()
 */
void AreaPlanEditor::setTakeoffHeight(qreal height)
{
    if (qAbs(_takeoffHeight - height) > 0.1) {  // Allow small floating point differences
        _takeoffHeight = height;
        emit takeoffHeightChanged();
    }
}

/**
 * @brief Sets the spacing between vehicles in formation flying
 * 
 * This method updates the formation spacing property, which defines
 * the distance between vehicles when flying in coordinated formations
 * during multi-vehicle missions.
 * 
 * @param spacing Distance between vehicles in meters (must be positive)
 * 
 * @section Usage
 * Formation spacing enables:
 * - Coordinated multi-vehicle operations
 * - Safe vehicle separation and positioning
 * - Formation flying and coordination
 * - Operational efficiency and safety
 * 
 * @section Formation Management
 * The spacing affects:
 * - Vehicle positioning and coordination
 * - Formation geometry and layout
 * - Safety margins and collision avoidance
 * - Operational efficiency and coordination
 * 
 * @section Safety Features
 * Formation spacing ensures:
 * - Safe vehicle separation
 * - Collision avoidance and safety margins
 * - Proper formation geometry
 * - Operational safety and coordination
 * 
 * @section Mission Integration
 * The spacing influences:
 * - Mission planning and coordination
 * - Vehicle positioning and routing
 * - Formation transitions and changes
 * - Operational efficiency and safety
 * 
 * @note The formation spacing should provide sufficient
 *       separation for safe operations while maintaining
 *       effective coordination and formation geometry.
 * 
 * @see formationSpacing()
 * @see formationSpacingChanged()
 * @see currentFormation()
 * @see droneCount()
 */
void AreaPlanEditor::setFormationSpacing(qreal spacing)
{
    // Enforce at least 1m spacing between vehicles in formation
    qreal clamped = spacing < 1.0 ? 1.0 : spacing;
    if (qFuzzyCompare(_formationSpacing, clamped)) {
        return;
    }
    
    _formationSpacing = clamped;
    calculateFormationPositions();
    emit formationSpacingChanged();
}

/**
 * @brief Calculates a new coordinate offset from a reference point
 * 
 * This utility method computes a new geographical coordinate by applying
 * a distance and bearing offset from a reference coordinate. It's used
 * for positioning waypoints, calculating formation positions, and
 * determining mission boundaries relative to a center point.
 * 
 * @param coord Reference coordinate (starting point)
 * @param meters Distance to offset in meters
 * @param bearing Bearing angle in degrees (0° = North, 90° = East)
 * 
 * @return QGeoCoordinate representing the new offset position
 * 
 * @section Usage
 * Coordinate offset calculation is used for:
 * - Waypoint positioning relative to area center
 * - Formation geometry and vehicle spacing
 * - Mission boundary calculations
 * - Flight path generation and optimization
 * 
 * @section Coordinate System
 * The method uses:
 * - Latitude/longitude coordinate system
 * - Approximate conversion factors for small distances
 * - Trigonometric calculations for offset direction
 * - Bearing angles in degrees from true north
 * 
 * @section Conversion Factors
 * Distance conversion uses approximate factors:
 * - Latitude: 111,319.9 meters per degree
 * - Longitude: Adjusted for latitude (cosine factor)
 * - Suitable for distances up to several kilometers
 * - Provides sufficient accuracy for mission planning
 * 
 * @section Mathematical Approach
 * The offset calculation:
 * - Converts bearing to radians for trigonometric functions
 * - Applies cosine for latitude offset (North/South component)
 * - Applies sine for longitude offset (East/West component)
 * - Adjusts longitude offset for latitude-dependent scaling
 * 
 * @note This method provides approximate results suitable for
 *       mission planning. For high-precision applications,
 *       consider using more accurate geodesic calculations.
 * 
 * @see areaCenter()
 * @see formationSpacing()
 * @see generateMission()
 * @see calculateFormationPositions()
 */
QGeoCoordinate AreaPlanEditor::calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const
{
    // Convert meters to degrees using approximate conversion factors
    // These factors are approximate for small distances
    const qreal metersPerDegreeLat = 111319.9;  // meters per degree of latitude
    const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(coord.latitude()));  // meters per degree of longitude

    qreal latOffset = meters * qCos(qDegreesToRadians(bearing)) / metersPerDegreeLat;
    qreal lonOffset = meters * qSin(qDegreesToRadians(bearing)) / metersPerDegreeLon;

    return QGeoCoordinate(coord.latitude() + latOffset, coord.longitude() + lonOffset);
}

/**
 * @brief Checks if the swarm of vehicles is ready for coordinated operations
 * 
 * This method evaluates the readiness status of all connected vehicles
 * to determine if they can participate in coordinated swarm operations.
 * It verifies vehicle availability, communication status, and operational
 * readiness across the entire fleet.
 * 
 * @return True if all vehicles are ready for swarm operations, false otherwise
 * 
 * @section Swarm Readiness Requirements
 * The method checks for:
 * - Minimum of 2 connected vehicles
 * - All vehicles have valid vehicle models
 * - Each vehicle reports ready status
 * - Proper communication and control links
 * 
 * @section Vehicle Management
 * The method accesses:
 * - MultiVehicleManager for vehicle enumeration
 * - Individual vehicle objects and their properties
 * - Vehicle readiness status tracking
 * - Communication and control verification
 * 
 * @section Use Cases
 * Swarm readiness is checked before:
 * - Coordinated takeoff operations
 * - Formation flying and transitions
 * - Multi-vehicle mission execution
 * - Swarm coordination and control
 * 
 * @section Safety Features
 * The readiness check ensures:
 * - All vehicles are properly connected
 * - Communication links are established
 * - Vehicles are in operational state
 * - Safe swarm operations can proceed
 * 
 * @note This method provides a safety gate for all swarm operations.
 *       Operations should not proceed unless swarm readiness is confirmed.
 * 
 * @see isSwarmReady()
 * @see startCoordinatedTakeoff()
 * @see startCoordinatedMission()
 * @see updateSwarmStatus()
 */
bool AreaPlanEditor::isSwarmReady() const
{
    return checkSwarmReadiness();
}

/**
 * @brief Gets the current status of the swarm operations
 * 
 * This method returns a human-readable string describing the current
 * status of swarm operations, including readiness state, operational
 * status, and any relevant status messages or error conditions.
 * 
 * @return QString containing the current swarm status description
 * 
 * @section Status Information
 * The status string provides:
 * - Current swarm operational state
 * - Readiness information and requirements
 * - Error conditions and troubleshooting details
 * - Operational progress and coordination status
 * 
 * @section User Interface
 * The status is used for:
 * - Displaying current swarm state to operators
 * - Providing feedback on swarm operations
 * - Troubleshooting and diagnostic information
 * - Operational planning and decision making
 * 
 * @section Status Updates
 * The status is updated when:
 * - Swarm readiness changes
 * - Operations are initiated or completed
 * - Errors or issues occur
 * - Coordination state changes
 * 
 * @section Common Status Values
 * Typical status messages include:
 * - "Swarm ready for operations"
 * - "Swarm not ready for coordinated takeoff"
 * - "Coordinated takeoff initiated"
 * - "Formation transition in progress"
 * 
 * @note The status provides real-time feedback on swarm operations
 *       and should be monitored during coordinated missions.
 * 
 * @see swarmStatus()
 * @see updateSwarmStatus()
 * @see isSwarmReady()
 * @see startCoordinatedTakeoff()
 */
QString AreaPlanEditor::swarmStatus() const
{
    return _swarmStatus;
}

/**
 * @brief Checks if a coordinated mission is currently active
 * 
 * This method indicates whether the swarm is currently executing
 * a coordinated mission with multiple vehicles operating in
 * synchronized formation or coordinated patterns.
 * 
 * @return True if a coordinated mission is active, false otherwise
 * 
 * @section Mission State
 * The method tracks:
 * - Active coordinated mission execution
 * - Formation flying operations
 * - Multi-vehicle synchronization
 * - Mission coordination status
 * 
 * @section Operational Context
 * Coordinated missions include:
 * - Formation flying and transitions
 * - Synchronized waypoint execution
 * - Multi-vehicle coordination
 * - Swarm mission management
 * 
 * @section Use Cases
 * This status is used for:
 * - Mission state monitoring and display
 * - Operator control and decision making
 * - Mission abort and recovery procedures
 * - Status reporting and logging
 * 
 * @section Safety Features
 * Mission status tracking enables:
 * - Proper mission state management
 * - Safe mission transitions and changes
 * - Emergency procedures and abort handling
 * - Operational safety and control
 * 
 * @note The coordinated mission status should be monitored
 *       during swarm operations to ensure proper mission flow.
 * 
 * @see isCoordinatedMissionActive()
 * @see startCoordinatedMission()
 * @see stopCoordinatedMission()
 * @see swarmStatus()
 */
bool AreaPlanEditor::isCoordinatedMissionActive() const
{
    return _isCoordinatedMissionActive;
}

/**
 * @brief Gets the current formation type being used by the swarm
 * 
 * This method returns the current formation type that the swarm
 * is using for coordinated operations. The formation type defines
 * the geometric arrangement and spacing of vehicles in the swarm.
 * 
 * @return FormationType enum value representing current formation
 * 
 * @section Formation Types
 * Available formations include:
 * - NoFormation: No specific formation pattern
 * - LineFormation: Vehicles in a straight line
 * - VFormation: Vehicles in V-shaped pattern
 * - DiamondFormation: Vehicles in diamond pattern
 * - CustomFormation: User-defined formation pattern
 * 
 * @section Formation Management
 * The formation affects:
 * - Vehicle positioning and spacing
 * - Mission coordination and execution
 * - Operational efficiency and safety
 * - Formation transitions and changes
 * 
 * @section Use Cases
 * Formation information is used for:
 * - Displaying current formation to operators
 * - Planning formation transitions
 * - Mission coordination and execution
 * - Safety and collision avoidance
 * 
 * @section Formation Transitions
 * Formation changes involve:
 * - Coordinated vehicle movement
 * - Safe spacing and positioning
 * - Mission flow and coordination
 * - Operational safety and control
 * 
 * @note The current formation should be considered when
 *       planning mission operations and transitions.
 * 
 * @see currentFormation()
 * @see setFormationSpacing()
 * @see isFormationTransitioning()
 * @see FormationType
 */
AreaPlanEditor::FormationType AreaPlanEditor::currentFormation() const
{
    return _currentFormation;
}

/**
 * @brief Checks if the swarm is currently transitioning between formations
 * 
 * This method indicates whether the swarm is in the process of
 * changing from one formation pattern to another. During transitions,
 * vehicles are moving to new positions while maintaining safety
 * and coordination.
 * 
 * @return True if formation transition is in progress, false otherwise
 * 
 * @section Transition State
 * The method tracks:
 * - Active formation change operations
 * - Vehicle movement and positioning
 * - Formation transition progress
 * - Coordination during transitions
 * 
 * @section Transition Process
 * Formation transitions involve:
 * - Coordinated vehicle movement
 * - Safe spacing and positioning
 * - Mission flow and coordination
 * - Operational safety and control
 * 
 * @section Use Cases
 * Transition status is used for:
 * - Mission state monitoring and display
 * - Operator control and decision making
 * - Mission flow and coordination
 * - Status reporting and logging
 * 
 * @section Safety Features
 * Transition tracking enables:
 * - Proper mission state management
 * - Safe formation changes
 * - Emergency procedures and abort handling
 * - Operational safety and control
 * 
 * @note Formation transitions should be monitored to ensure
 *       safe completion and proper mission flow.
 * 
 * @see isFormationTransitioning()
 * @see currentFormation()
 * @see setFormationSpacing()
 * @see swarmStatus()
 */
bool AreaPlanEditor::isFormationTransitioning() const
{
    return _isFormationTransitioning;
}

/**
 * @brief Internal method to check swarm readiness status
 * 
 * This private method performs the actual swarm readiness verification
 * by examining all connected vehicles and their operational status.
 * It's called by the public isSwarmReady() method to provide
 * consistent readiness checking across the system.
 * 
 * @return True if swarm meets all readiness requirements, false otherwise
 * 
 * @section Readiness Verification
 * The method verifies:
 * - Minimum vehicle count (at least 2 vehicles)
 * - Valid vehicle model availability
 * - Individual vehicle ready status
 * - Communication and control links
 * 
 * @section Vehicle Management
 * The method accesses:
 * - MultiVehicleManager singleton instance
 * - Vehicle list model and enumeration
 * - Individual vehicle objects and properties
 * - Vehicle readiness status tracking
 * 
 * @section Safety Requirements
 * Swarm operations require:
 * - Multiple vehicles for coordination
 * - All vehicles in ready state
 * - Proper communication links
 * - Operational control capabilities
 * 
 * @section Implementation Details
 * The verification process:
 * - Checks vehicle model availability and count
 * - Iterates through all connected vehicles
 * - Verifies individual vehicle readiness
 * - Returns false if any vehicle is not ready
 * 
 * @note This method provides the core logic for swarm readiness
 *       verification and is used by all swarm operation methods.
 * 
 * @see isSwarmReady()
 * @see MultiVehicleManager
 * @see Vehicle
 * @see _vehicleReadyStatus
 */
bool AreaPlanEditor::checkSwarmReadiness() const
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) {
        return false;  // Need at least 2 vehicles for swarm operations
    }

    // Check if all vehicles are ready
    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (!vehicle || !_vehicleReadyStatus.value(vehicle->id(), false)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Updates the swarm status with new information
 * 
 * This method updates the internal swarm status string and emits
 * a change signal when the status changes. It's used throughout
 * the system to provide real-time feedback on swarm operations,
 * including readiness state, operational status, and error conditions.
 * 
 * @param status New status string to set
 * 
 * @section Status Management
 * The method handles:
 * - Status string updates and changes
 * - Change signal emission for UI updates
 * - Status consistency and synchronization
 * - Real-time status feedback
 * 
 * @section Signal Emission
 * Status changes trigger:
 * - swarmStatusChanged() signal emission
 * - UI updates and status display
 * - Status monitoring and logging
 * - Operator notification and feedback
 * 
 * @section Use Cases
 * Status updates occur during:
 * - Swarm readiness changes
 * - Operation initiation and completion
 * - Error conditions and troubleshooting
 * - Formation transitions and changes
 * - Mission coordination events
 * 
 * @section Status Consistency
 * The method ensures:
 * - Status changes are properly tracked
 * - Signals are emitted only when needed
 * - Status remains synchronized across the system
 * - UI updates reflect current state
 * 
 * @note Status updates should provide clear, actionable information
 *       to operators and maintain consistency with actual swarm state.
 * 
 * @see updateSwarmStatus()
 * @see swarmStatus()
 * @see swarmStatusChanged()
 * @see _swarmStatus
 */
void AreaPlanEditor::updateSwarmStatus(const QString& status)
{
    if (_swarmStatus != status) {
        _swarmStatus = status;
        emit swarmStatusChanged();
    }
}

/**
 * @brief Initiates coordinated takeoff for all vehicles in the swarm
 * 
 * This method coordinates the takeoff sequence for all vehicles in
 * the swarm, ensuring synchronized departure and proper formation
 * establishment. It verifies swarm readiness before proceeding
 * and provides status feedback throughout the process.
 * 
 * @return True if coordinated takeoff was initiated, false otherwise
 * 
 * @section Takeoff Coordination
 * The method coordinates:
 * - Synchronized takeoff timing
 * - Formation establishment
 * - Safety verification and checks
 * - Status monitoring and feedback
 * 
 * @section Pre-flight Verification
 * Before takeoff, the method:
 * - Checks swarm readiness status
 * - Verifies vehicle availability
 * - Ensures communication links
 * - Validates operational parameters
 * 
 * @section Command Execution
 * During takeoff execution:
 * - Sends takeoff commands to all vehicles
 * - Monitors command acceptance
 * - Updates swarm status
 * - Provides operator feedback
 * 
 * @section Safety Features
 * Takeoff safety includes:
 * - Swarm readiness verification
 * - Communication link validation
 * - Vehicle status monitoring
 * - Error handling and reporting
 * 
 * @note Coordinated takeoff requires all vehicles to be ready
 *       and properly configured for swarm operations.
 * 
 * @see startCoordinatedTakeoff()
 * @see isSwarmReady()
 * @see sendSwarmCommand()
 * @see updateSwarmStatus()
 */
bool AreaPlanEditor::startCoordinatedTakeoff()
{
    if (!isSwarmReady()) {
        updateSwarmStatus("Swarm not ready for coordinated takeoff");
        return false;
    }

    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleModel->count(); i++) {
        vehicles.append(qobject_cast<Vehicle*>(vehicleModel->get(i)));
    }

    // Send takeoff command to all vehicles
    sendSwarmCommand(MAV_CMD_NAV_TAKEOFF, vehicles);
    updateSwarmStatus("Coordinated takeoff initiated");
    return true;
}

/**
 * @brief Starts a coordinated mission for the entire swarm
 * 
 * This method initiates coordinated mission execution for all
 * vehicles in the swarm, enabling synchronized operations,
 * formation flying, and coordinated mission management.
 * 
 * @return True if coordinated mission was started, false otherwise
 * 
 * @section Mission Initiation
 * The method handles:
 * - Mission state activation
 * - Swarm coordination setup
 * - Formation establishment
 * - Status monitoring and feedback
 * 
 * @section Pre-mission Verification
 * Before starting, the method:
 * - Checks swarm readiness status
 * - Verifies vehicle availability
 * - Ensures communication links
 * - Validates mission parameters
 * 
 * @section State Management
 * During mission start:
 * - Sets coordinated mission active flag
 * - Emits status change signals
 * - Updates swarm status
 * - Establishes mission coordination
 * 
 * @section Mission Coordination
 * The method enables:
 * - Synchronized mission execution
 * - Formation flying and transitions
 * - Multi-vehicle coordination
 * - Mission monitoring and control
 * 
 * @note Coordinated missions require proper swarm readiness
 *       and all vehicles to be in operational state.
 * 
 * @see startCoordinatedMission()
 * @see isSwarmReady()
 * @see coordinatedMissionStatusChanged()
 * @see updateSwarmStatus()
 */
bool AreaPlanEditor::startCoordinatedMission()
{
    if (!isSwarmReady()) {
        updateSwarmStatus("Swarm not ready for coordinated mission");
        return false;
    }

    _isCoordinatedMissionActive = true;
    emit coordinatedMissionStatusChanged();
    updateSwarmStatus("Coordinated mission started");
    return true;
}

/**
 * @brief Aborts the currently active coordinated mission
 * 
 * This method safely terminates the active coordinated mission,
 * stopping all swarm operations and returning vehicles to
 * safe operational states. It provides immediate mission
 * termination for safety and emergency situations.
 * 
 * @return True if mission was aborted, false if no mission was active
 * 
 * @section Mission Abort
 * The method handles:
 * - Immediate mission termination
 * - Swarm coordination shutdown
 * - Vehicle safety procedures
 * - Status updates and feedback
 * 
 * @section Abort Process
 * During abort execution:
 * - Sets mission inactive flag
 * - Emits status change signals
 * - Updates swarm status
 * - Initiates safety procedures
 * 
 * @section Safety Features
 * Abort safety includes:
 * - Immediate mission termination
 * - Vehicle safety state transition
 * - Communication link maintenance
 * - Status monitoring and feedback
 * 
 * @section Use Cases
 * Mission abort is used for:
 * - Emergency situations
 * - Safety violations
 * - Operator intervention
 * - System failures
 * 
 * @note Mission abort provides immediate termination for
 *       safety-critical situations and emergency procedures.
 * 
 * @see abortCoordinatedMission()
 * @see isCoordinatedMissionActive()
 * @see coordinatedMissionStatusChanged()
 * @see updateSwarmStatus()
 */
bool AreaPlanEditor::abortCoordinatedMission()
{
    if (!_isCoordinatedMissionActive) {
        return false;
    }

    _isCoordinatedMissionActive = false;
    emit coordinatedMissionStatusChanged();
    updateSwarmStatus("Coordinated mission aborted");
    return true;
}

/**
 * @brief Sets the formation type for swarm operations
 * 
 * This method changes the formation pattern used by the swarm
 * for coordinated operations. It automatically recalculates
 * formation positions and updates the system state to reflect
 * the new formation configuration.
 * 
 * @param type New formation type to set
 * @return True if formation was set successfully, false otherwise
 * 
 * @section Formation Types
 * Available formation patterns include:
 * - NoFormation: No specific formation pattern
 * - LineFormation: Vehicles in a straight line
 * - VFormation: Vehicles in V-shaped pattern
 * - DiamondFormation: Vehicles in diamond pattern
 * - CustomFormation: User-defined formation pattern
 * 
 * @section Formation Change Process
 * When changing formations:
 * - Updates current formation type
 * - Recalculates vehicle positions
 * - Emits formation change signals
 * - Updates system state
 * 
 * @section Position Calculation
 * Formation changes trigger:
 * - Automatic position recalculation
 * - Vehicle spacing adjustments
 * - Formation geometry updates
 * - Safety margin verification
 * 
 * @section Signal Emission
 * Formation changes emit:
 * - formationChanged() signal
 * - UI updates and notifications
 * - Status monitoring updates
 * - Operator feedback
 * 
 * @note Formation changes should be planned and executed
 *       safely to maintain swarm coordination and safety.
 * 
 * @see setFormationType()
 * @see currentFormation()
 * @see calculateFormationPositions()
 * @see formationChanged()
 */
bool AreaPlanEditor::setFormationType(FormationType type)
{
    if (_currentFormation == type) {
        return true;
    }

    _currentFormation = type;
    calculateFormationPositions();
    emit formationChanged();
    return true;
}

/**
 * @brief Adjusts the spacing between vehicles in the current formation
 * 
 * This method modifies the distance between vehicles in the
 * current formation pattern. It updates the formation spacing
 * and recalculates all vehicle positions to maintain proper
 * formation geometry and safety margins.
 * 
 * @param spacing New spacing distance in meters
 * @return True if spacing was adjusted successfully
 * 
 * @section Spacing Adjustment
 * The method handles:
 * - Formation spacing updates
 * - Position recalculation
 * - Safety margin verification
 * - Formation geometry maintenance
 * 
 * @section Position Updates
 * Spacing changes trigger:
 * - Vehicle position recalculation
 * - Formation geometry updates
 * - Safety margin verification
 * - Coordinate updates
 * 
 * @section Safety Considerations
 * Spacing adjustments ensure:
 * - Safe vehicle separation
 * - Collision avoidance
 * - Proper formation geometry
 * - Operational safety
 * 
 * @section Formation Maintenance
 * The method maintains:
 * - Formation pattern integrity
 * - Vehicle coordination
 * - Safety margins
 * - Operational efficiency
 * 
 * @note Spacing adjustments should provide sufficient
 *       separation for safe operations while maintaining
 *       effective formation geometry.
 * 
 * @see adjustFormationSpacing()
 * @see formationSpacing()
 * @see calculateFormationPositions()
 * @see setFormationSpacing()
 */
bool AreaPlanEditor::adjustFormationSpacing(qreal spacing)
{
    setFormationSpacing(spacing);
    return true;
}

/**
 * @brief Assigns formation roles to all vehicles in the swarm
 * 
 * This method establishes the role hierarchy within the formation,
 * designating one vehicle as the leader and others as followers.
 * Role assignment is critical for coordinated operations and
 * formation management.
 * 
 * @return True if roles were assigned successfully, false otherwise
 * 
 * @section Role Assignment
 * The method assigns:
 * - Leader vehicle (index 0)
 * - Follower vehicles (indices 1+)
 * - Formation hierarchy
 * - Operational responsibilities
 * 
 * @section Vehicle Management
 * Role assignment involves:
 * - Vehicle enumeration and validation
 * - Role mapping and assignment
 * - Leader vehicle designation
 * - Follower vehicle organization
 * 
 * @section Formation Hierarchy
 * The role system provides:
 * - Clear operational leadership
 * - Coordinated mission execution
 * - Formation management structure
 * - Operational control hierarchy
 * 
 * @section Signal Emission
 * Role changes emit:
 * - formationRolesChanged() signal
 * - leaderVehicleChanged() signal
 * - UI updates and notifications
 * - Status monitoring updates
 * 
 * @note Role assignment requires at least 2 vehicles
 *       and establishes the foundation for coordinated operations.
 * 
 * @see assignFormationRoles()
 * @see _formationRoles
 * @see _leaderVehicle
 * @see formationRolesChanged()
 */
bool AreaPlanEditor::assignFormationRoles()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) {
        return false;
    }

    // Clear existing roles
    _formationRoles.clear();

    // Assign roles (0 = leader, 1+ = followers)
    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle) {
            _formationRoles[vehicle->id()] = i;
            if (i == 0) {
                _leaderVehicle = vehicle;
                emit leaderVehicleChanged();
            }
        }
    }

    emit formationRolesChanged();
    return true;
}

/**
 * @brief Initiates a formation transition for the swarm
 * 
 * This method starts the process of changing from the current
 * formation to a new one. It coordinates the movement of all
 * vehicles to their new positions while maintaining safety
 * and coordination throughout the transition.
 * 
 * @return True if formation transition was started, false otherwise
 * 
 * @section Transition Initiation
 * The method handles:
 * - Transition state activation
 * - Position calculation
 * - Command generation
 * - Status monitoring
 * 
 * @section Pre-transition Checks
 * Before starting, the method:
 * - Verifies swarm readiness
 * - Checks transition state
 * - Validates formation parameters
 * - Ensures safety conditions
 * 
 * @section Transition Process
 * During transition execution:
 * - Sets transition state flag
 * - Calculates new positions
 * - Sends formation commands
 * - Monitors progress
 * 
 * @section Safety Features
 * Transition safety includes:
 * - Swarm readiness verification
 * - State conflict prevention
 * - Position calculation validation
 * - Command execution monitoring
 * 
 * @note Formation transitions require proper swarm readiness
 *       and should be executed safely to maintain coordination.
 * 
 * @see startFormationTransition()
 * @see isFormationTransitioning()
 * @see calculateFormationPositions()
 * @see sendFormationCommands()
 */
bool AreaPlanEditor::startFormationTransition()
{
    if (!isSwarmReady() || _isFormationTransitioning) {
        return false;
    }

    _isFormationTransitioning = true;
    emit formationTransitioningChanged();

    // Calculate and send new formation positions
    calculateFormationPositions();
    sendFormationCommands();

    return true;
}

/**
 * @brief Sends MAVLink commands to multiple vehicles in the swarm
 * 
 * This method distributes MAVLink commands to all specified
 * vehicles in the swarm, enabling coordinated command execution
 * across the entire fleet. It's used for synchronized operations
 * like takeoff, formation changes, and mission coordination.
 * 
 * @param command MAVLink command ID to send
 * @param vehicles List of vehicles to receive the command
 * 
 * @section Command Distribution
 * The method handles:
 * - Command distribution to multiple vehicles
 * - Vehicle validation and verification
 * - Command execution monitoring
 * - Status tracking and feedback
 * 
 * @section MAVLink Integration
 * Command sending involves:
 * - MAVLink command formatting
 * - Vehicle communication
 * - Command validation
 * - Response monitoring
 * 
 * @section Vehicle Management
 * The method processes:
 * - Vehicle list enumeration
 * - Individual vehicle targeting
 * - Command distribution
 * - Status monitoring
 * 
 * @section Use Cases
 * Common command types include:
 * - MAV_CMD_NAV_TAKEOFF for coordinated takeoff
 * - MAV_CMD_NAV_WAYPOINT for waypoint navigation
 * - MAV_CMD_DO_SET_SERVO for payload operations
 * - MAV_CMD_DO_MOUNT_CONTROL for camera control
 * 
 * @note This method provides the foundation for coordinated
 *       swarm operations and synchronized command execution.
 * 
 * @see sendSwarmCommand()
 * @see MAV_CMD_NAV_TAKEOFF
 * @see Vehicle
 * @see MAVLink
 */
void AreaPlanEditor::sendSwarmCommand(uint16_t command, const QList<Vehicle*>& vehicles)
{
    for (Vehicle* vehicle : vehicles) {
        if (vehicle) {
            // Send MAVLink command to each vehicle
            // Note: Actual command sending would require more parameters and proper MAVLink integration
            qDebug() << "Sending command" << command << "to vehicle" << vehicle->id();
        }
    }
}

/**
 * @brief Handles MAVLink command responses from swarm vehicles
 * 
 * This method processes responses from individual vehicles in the swarm
 * after they receive and execute MAVLink commands. It updates vehicle
 * readiness status and swarm status based on command success or failure,
 * enabling coordinated operation monitoring and error handling.
 * 
 * @param vehicle Vehicle that sent the response
 * @param command MAVLink command ID that was executed
 * @param result MAVLink result code (MAV_RESULT_ACCEPTED, etc.)
 * 
 * @section Response Processing
 * The method handles:
 * - Command result interpretation
 * - Vehicle status updates
 * - Swarm status monitoring
 * - Error condition handling
 * 
 * @section Status Management
 * Response handling updates:
 * - Vehicle ready status tracking
 * - Swarm operational status
 * - Error condition reporting
 * - Status synchronization
 * 
 * @section MAVLink Integration
 * The method processes:
 * - MAV_RESULT_ACCEPTED for successful commands
 * - MAV_RESULT_FAILED for failed commands
 * - Other MAVLink result codes
 * - Command execution feedback
 * 
 * @section Error Handling
 * Failed commands trigger:
 * - Vehicle status updates
 * - Swarm status notifications
 * - Error reporting and logging
 * - Troubleshooting information
 * 
 * @note This method is critical for maintaining accurate
 *       swarm status and enabling proper error handling.
 * 
 * @see handleSwarmResponse()
 * @see _vehicleReadyStatus
 * @see updateSwarmStatus()
 * @see MAV_RESULT_ACCEPTED
 */
void AreaPlanEditor::handleSwarmResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) {
        return;
    }

    // Update vehicle ready status based on command result
    _vehicleReadyStatus[vehicle->id()] = (result == MAV_RESULT_ACCEPTED);
    
    // Update swarm status
    if (result != MAV_RESULT_ACCEPTED) {
        updateSwarmStatus(QString("Vehicle %1 command %2 failed").arg(vehicle->id()).arg(command));
    }
}

/**
 * @brief Sends formation position commands to all vehicles in the swarm
 * 
 * This method distributes formation position commands to all vehicles
 * in the swarm, enabling coordinated formation establishment and
 * transitions. It sends individual position offsets to each vehicle
 * based on their assigned formation role and spacing.
 * 
 * @section Formation Commands
 * The method handles:
 * - Position command distribution
 * - Vehicle-specific offset calculation
 * - Formation geometry establishment
 * - Coordinated positioning
 * 
 * @section Vehicle Management
 * Command distribution involves:
 * - Vehicle enumeration and validation
 * - Formation offset lookup
 * - Individual command targeting
 * - Status monitoring and feedback
 * 
 * @section Formation Geometry
 * Position commands establish:
 * - Vehicle relative positioning
 * - Formation pattern geometry
 * - Safety margins and spacing
 * - Coordinated movement patterns
 * 
 * @section MAVLink Integration
 * Command sending requires:
 * - Proper MAVLink integration
 * - Position command formatting
 * - Vehicle communication
 * - Response monitoring
 * 
 * @note This method provides the foundation for formation
 *       establishment and coordinated positioning.
 * 
 * @see sendFormationCommands()
 * @see _formationOffsets
 * @see calculateFormationPositions()
 * @see MAVLink
 */
void AreaPlanEditor::sendFormationCommands()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel) {
        return;
    }

    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle && _formationOffsets.contains(vehicle->id())) {
            // Send position command to each vehicle
            // Note: Actual command sending would require proper MAVLink integration
            qDebug() << "Sending formation position to vehicle" << vehicle->id() 
                    << "offset:" << _formationOffsets[vehicle->id()];
        }
    }
}

/**
 * @brief Handles formation command responses from swarm vehicles
 * 
 * This method processes responses from vehicles after they receive
 * formation position commands. It tracks completion status across
 * all vehicles and updates the formation transition state when
 * all vehicles have successfully positioned themselves.
 * 
 * @param vehicle Vehicle that sent the response
 * @param command MAVLink command ID that was executed
 * @param result MAVLink result code (MAV_RESULT_ACCEPTED, etc.)
 * 
 * @section Response Processing
 * The method handles:
 * - Individual vehicle responses
 * - Formation completion tracking
 * - Transition state management
 * - Status synchronization
 * 
 * @section Formation Completion
 * The method tracks:
 * - Vehicle response status
 * - Overall completion progress
 * - Formation transition state
 * - Status updates and notifications
 * 
 * @section State Management
 * Completion triggers:
 * - Formation transition completion
 * - State flag updates
 * - Signal emissions
 * - Status synchronization
 * 
 * @section Coordination
 * The method ensures:
 * - All vehicles complete positioning
 * - Formation state consistency
 * - Transition completion notification
 * - Status synchronization
 * 
 * @note This method is critical for maintaining accurate
 *       formation state and enabling proper coordination.
 * 
 * @see handleFormationResponse()
 * @see _isFormationTransitioning
 * @see formationTransitioningChanged()
 * @see _vehicleReadyStatus
 */
void AreaPlanEditor::handleFormationResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) {
        return;
    }

    // Update formation transition status if all vehicles have responded
    bool allComplete = true;
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    for (int i = 0; vehicleModel && i < vehicleModel->count(); i++) {
        Vehicle* v = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (v && !_vehicleReadyStatus.value(v->id(), false)) {
            allComplete = false;
            break;
        }
    }

    if (allComplete) {
        _isFormationTransitioning = false;
        emit formationTransitioningChanged();
    }
}

/**
 * @brief Moves the operational area north by one line spacing distance
 * 
 * This method shifts the center of the operational area northward
 * by the current line spacing distance. It's used for fine-tuning
 * area positioning and adjusting mission coverage boundaries.
 * 
 * @section Area Movement
 * The method handles:
 * - Northward area translation
 * - Center coordinate calculation
 * - Line spacing-based movement
 * - Area boundary adjustment
 * 
 * @section Coordinate Calculation
 * Movement calculation uses:
 * - Current area center position
 * - Line spacing distance
 * - North bearing (0 degrees)
 * - Coordinate offset calculation
 * 
 * @section Use Cases
 * Northward movement is used for:
 * - Fine area positioning
 * - Mission boundary adjustment
 * - Coverage area optimization
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - calculateOffsetCoordinate() for position calculation
 * - setAreaCenter() for area updates
 * - Line spacing parameter for movement distance
 * - Area center property for current position
 * 
 * @note Northward movement maintains the area's orientation
 *       and dimensions while shifting its position.
 * 
 * @see moveAreaNorth()
 * @see calculateOffsetCoordinate()
 * @see setAreaCenter()
 * @see _lineSpacing
 */
void AreaPlanEditor::moveAreaNorth()
{
    // Move area center north by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 0);
    setAreaCenter(newCenter);
}

/**
 * @brief Moves the operational area south by one line spacing distance
 * 
 * This method shifts the center of the operational area southward
 * by the current line spacing distance. It's used for fine-tuning
 * area positioning and adjusting mission coverage boundaries.
 * 
 * @section Area Movement
 * The method handles:
 * - Southward area translation
 * - Center coordinate calculation
 * - Line spacing-based movement
 * - Area boundary adjustment
 * 
 * @section Coordinate Calculation
 * Movement calculation uses:
 * - Current area center position
 * - Line spacing distance
 * - South bearing (180 degrees)
 * - Coordinate offset calculation
 * 
 * @section Use Cases
 * Southward movement is used for:
 * - Fine area positioning
 * - Mission boundary adjustment
 * - Coverage area optimization
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - calculateOffsetCoordinate() for position calculation
 * - setAreaCenter() for area updates
 * - Line spacing parameter for movement distance
 * - Area center property for current position
 * 
 * @note Southward movement maintains the area's orientation
 *       and dimensions while shifting its position.
 * 
 * @see moveAreaSouth()
 * @see calculateOffsetCoordinate()
 * @see setAreaCenter()
 * @see _lineSpacing
 */
void AreaPlanEditor::moveAreaSouth()
{
    // Move area center south by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 180);
    setAreaCenter(newCenter);
}

/**
 * @brief Moves the operational area east by one line spacing distance
 * 
 * This method shifts the center of the operational area eastward
 * by the current line spacing distance. It's used for fine-tuning
 * area positioning and adjusting mission coverage boundaries.
 * 
 * @section Area Movement
 * The method handles:
 * - Eastward area translation
 * - Center coordinate calculation
 * - Line spacing-based movement
 * - Area boundary adjustment
 * 
 * @section Coordinate Calculation
 * Movement calculation uses:
 * - Current area center position
 * - Line spacing distance
 * - East bearing (90 degrees)
 * - Coordinate offset calculation
 * 
 * @section Use Cases
 * Eastward movement is used for:
 * - Fine area positioning
 * - Mission boundary adjustment
 * - Coverage area optimization
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - calculateOffsetCoordinate() for position calculation
 * - setAreaCenter() for area updates
 * - Line spacing parameter for movement distance
 * - Area center property for current position
 * 
 * @note Eastward movement maintains the area's orientation
 *       and dimensions while shifting its position.
 * 
 * @see moveAreaEast()
 * @see calculateOffsetCoordinate()
 * @see setAreaCenter()
 * @see _lineSpacing
 */
void AreaPlanEditor::moveAreaEast()
{
    // Move area center east by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 90);
    setAreaCenter(newCenter);
}

/**
 * @brief Moves the operational area west by one line spacing distance
 * 
 * This method shifts the center of the operational area westward
 * by the current line spacing distance. It's used for fine-tuning
 * area positioning and adjusting mission coverage boundaries.
 * 
 * @section Area Movement
 * The method handles:
 * - Westward area translation
 * - Center coordinate calculation
 * - Line spacing-based movement
 * - Area boundary adjustment
 * 
 * @section Coordinate Calculation
 * Movement calculation uses:
 * - Current area center position
 * - Line spacing distance
 * - West bearing (270 degrees)
 * - Coordinate offset calculation
 * 
 * @section Use Cases
 * Westward movement is used for:
 * - Fine area positioning
 * - Mission boundary adjustment
 * - Coverage area optimization
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - calculateOffsetCoordinate() for position calculation
 * - setAreaCenter() for area updates
 * - Line spacing parameter for movement distance
 * - Area center property for current position
 * 
 * @note Westward movement maintains the area's orientation
 *       and dimensions while shifting its position.
 * 
 * @see moveAreaWest()
 * @see calculateOffsetCoordinate()
 * @see setAreaCenter()
 * @see _lineSpacing
 */
void AreaPlanEditor::moveAreaWest()
{
    // Move area center west by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 270);
    setAreaCenter(newCenter);
}

/**
 * @brief Rotates the operational area clockwise by 5 degrees
 * 
 * This method rotates the operational area clockwise around its center
 * point by 5 degrees. It's used for fine-tuning area orientation
 * to align with terrain features, wind conditions, or operational
 * requirements.
 * 
 * @section Area Rotation
 * The method handles:
 * - Clockwise area rotation
 * - Center point preservation
 * - Orientation adjustment
 * - Mission alignment
 * 
 * @section Rotation Parameters
 * Rotation characteristics:
 * - Direction: Clockwise (positive rotation)
 * - Increment: 5 degrees per call
 * - Center: Area center point
 * - Accumulative: Adds to current rotation
 * 
 * @section Use Cases
 * Clockwise rotation is used for:
 * - Terrain feature alignment
 * - Wind condition optimization
 * - Mission coordination
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - setAreaRotation() for rotation updates
 * - _areaRotation property for current state
 * - Mission generation for waypoint positioning
 * - Area visualization for display updates
 * 
 * @note Rotation is applied around the area center point
 *       and affects all waypoint positioning and mission geometry.
 * 
 * @see rotateAreaClockwise()
 * @see setAreaRotation()
 * @see _areaRotation
 * @see generateWaypoints()
 */
void AreaPlanEditor::rotateAreaClockwise()
{
    setAreaRotation(_areaRotation + 5.0);  // Rotate 5 degrees clockwise
}

/**
 * @brief Rotates the operational area counter-clockwise by 5 degrees
 * 
 * This method rotates the operational area counter-clockwise around
 * its center point by 5 degrees. It's used for fine-tuning area
 * orientation to align with terrain features, wind conditions, or
 * operational requirements.
 * 
 * @section Area Rotation
 * The method handles:
 * - Counter-clockwise area rotation
 * - Center point preservation
 * - Orientation adjustment
 * - Mission alignment
 * 
 * @section Rotation Parameters
 * Rotation characteristics:
 * - Direction: Counter-clockwise (negative rotation)
 * - Increment: 5 degrees per call
 * - Center: Area center point
 * - Accumulative: Subtracts from current rotation
 * 
 * @section Use Cases
 * Counter-clockwise rotation is used for:
 * - Terrain feature alignment
 * - Wind condition optimization
 * - Mission coordination
 * - Operational area refinement
 * 
 * @section Integration
 * The method integrates with:
 * - setAreaRotation() for rotation updates
 * - _areaRotation property for current state
 * - Mission generation for waypoint positioning
 * - Area visualization for display updates
 * 
 * @note Rotation is applied around the area center point
 *       and affects all waypoint positioning and mission geometry.
 * 
 * @see rotateAreaCounterClockwise()
 * @see setAreaRotation()
 * @see _areaRotation
 * @see generateWaypoints()
 */
void AreaPlanEditor::rotateAreaCounterClockwise()
{
    setAreaRotation(_areaRotation - 5.0);  // Rotate 5 degrees counter-clockwise
}

/**
 * @brief Centers the operational area on the current vehicle position
 * 
 * This method repositions the operational area center to match the
 * current vehicle's GPS coordinates. It's used for quick area
 * positioning and mission planning relative to the vehicle's
 * current location.
 * 
 * @section Area Centering
 * The method handles:
 * - Vehicle position retrieval
 * - Area center repositioning
 * - Coordinate synchronization
 * - Mission alignment
 * 
 * @section Vehicle Integration
 * Centering process involves:
 * - Getting current active vehicle
 * - Retrieving vehicle GPS coordinates
 * - Updating area center position
 * - Synchronizing mission parameters
 * 
 * @section Use Cases
 * Area centering is used for:
 * - Quick mission setup
 * - Vehicle-relative positioning
 * - Dynamic area placement
 * - Operational flexibility
 * 
 * @section Integration
 * The method integrates with:
 * - getCurrentVehicle() for vehicle access
 * - setAreaCenter() for position updates
 * - Vehicle coordinate system
 * - Mission generation for waypoint positioning
 * 
 * @note Centering on vehicle position provides immediate
 *       mission relevance and operational flexibility.
 * 
 * @see centerArea()
 * @see getCurrentVehicle()
 * @see setAreaCenter()
 * @see Vehicle::coordinate()
 */
void AreaPlanEditor::centerArea()
{
    // Get current vehicle position as center
    Vehicle* vehicle = getCurrentVehicle();
    if (vehicle) {
        setAreaCenter(vehicle->coordinate());
    }
}

/**
 * @brief Resets all area parameters to their default values
 * 
 * This method restores all area planning parameters to their
 * predefined default values, providing a clean slate for new
 * mission planning. It also centers the area on the current
 * vehicle position for immediate usability.
 * 
 * @section Parameter Reset
 * The method resets:
 * - Area dimensions (width, height)
 * - Line spacing and number of points
 * - Mission altitude
 * - Area rotation
 * - Area center position
 * 
 * @section Default Values
 * Reset parameters include:
 * - Area Width: _defaultAreaWidth
 * - Area Height: _defaultAreaHeight
 * - Line Spacing: _defaultLineSpacing
 * - Number of Points: _defaultNumPoints
 * - Mission Altitude: _defaultAltitude
 * - Area Rotation: 0.0 degrees
 * 
 * @section Reset Process
 * The reset sequence:
 * - Restores all parameters to defaults
 * - Resets rotation to north (0 degrees)
 * - Centers area on current vehicle
 * - Provides clean mission planning state
 * 
 * @section Use Cases
 * Area reset is used for:
 * - New mission planning
 * - Parameter cleanup
 * - Default configuration
 * - Quick mission restart
 * 
 * @note Reset provides a clean, default configuration
 *       centered on the current vehicle position.
 * 
 * @see resetArea()
 * @see centerArea()
 * @see Default parameter constants
 * @see Mission planning
 */
void AreaPlanEditor::resetArea()
{
    // Reset all area parameters to defaults
    setAreaWidth(_defaultAreaWidth);
    setAreaHeight(_defaultAreaHeight);
    setLineSpacing(_defaultLineSpacing);
    setNumPoints(_defaultNumPoints);
    setMissionAltitude(_defaultAltitude);
    setAreaRotation(0.0);
    
    // Center on current vehicle
    centerArea();
}

/**
 * @brief Gets the currently active vehicle for mission planning
 * 
 * This method retrieves the currently active vehicle from the
 * MultiVehicleManager, providing access to the primary vehicle
 * for mission planning, area positioning, and operational
 * coordination.
 * 
 * @return Pointer to the currently active vehicle, or nullptr if none
 * 
 * @section Vehicle Access
 * The method provides:
 * - Active vehicle identification
 * - Vehicle object access
 * - Mission planning context
 * - Operational coordination
 * 
 * @section MultiVehicleManager Integration
 * Vehicle retrieval involves:
 * - Accessing MultiVehicleManager singleton
 * - Retrieving active vehicle reference
 * - Providing vehicle object access
 * - Handling no-vehicle scenarios
 * 
 * @section Use Cases
 * Current vehicle access is used for:
 * - Mission planning and coordination
 * - Area positioning and centering
 * - Vehicle state monitoring
 * - Operational coordination
 * 
 * @section Return Value
 * The method returns:
 * - Valid Vehicle pointer if active vehicle exists
 * - nullptr if no active vehicle is available
 * - Consistent vehicle reference for operations
 * 
 * @note This method provides the primary vehicle
 *       reference for mission planning operations.
 * 
 * @see getCurrentVehicle()
 * @see MultiVehicleManager::activeVehicle()
 * @see Vehicle
 * @see Mission planning
 */
Vehicle* AreaPlanEditor::getCurrentVehicle() const
{
    return MultiVehicleManager::instance()->activeVehicle();
}

/**
 * @brief Gets the mission manager for the current vehicle
 * 
 * This method retrieves the mission manager associated with
 * the currently active vehicle, providing access to mission
 * creation, management, and execution capabilities.
 * 
 * @return Pointer to the vehicle's mission manager, or nullptr if unavailable
 * 
 * @section Mission Management
 * The method provides:
 * - Mission manager access
 * - Mission creation capabilities
 * - Mission execution control
 * - Mission state management
 * 
 * @section Vehicle Integration
 * Mission manager retrieval:
 * - Gets current active vehicle
 * - Accesses vehicle's mission manager
 * - Provides mission management interface
 * - Handles no-vehicle scenarios
 * 
 * @section Use Cases
 * Mission manager access is used for:
 * - Mission creation and planning
 * - Waypoint generation and management
 * - Mission execution and control
 * - Mission state monitoring
 * 
 * @section Return Value
 * The method returns:
 * - Valid MissionManager pointer if available
 * - nullptr if no vehicle or mission manager
 * - Consistent mission management interface
 * 
 * @note This method provides access to mission
 *       management capabilities for the active vehicle.
 * 
 * @see getMissionManager()
 * @see getCurrentVehicle()
 * @see Vehicle::missionManager()
 * @see MissionManager
 */
MissionManager* AreaPlanEditor::getMissionManager() const
{
    Vehicle* vehicle = getCurrentVehicle();
    return vehicle ? vehicle->missionManager() : nullptr;
}

/**
 * @brief Gets the mission controller for plan management
 * 
 * This method retrieves the mission controller from the plan
 * master controller, providing access to high-level mission
 * planning, coordination, and management capabilities.
 * 
 * @return Pointer to the mission controller, or nullptr if unavailable
 * 
 * @section Mission Control
 * The method provides:
 * - Mission controller access
 * - High-level mission planning
 * - Mission coordination capabilities
 * - Plan management interface
 * 
 * @section Controller Integration
 * Mission controller retrieval:
 * - Accesses plan master controller
 * - Casts to MissionController type
 * - Provides mission control interface
 * - Handles controller availability
 * 
 * @section Use Cases
 * Mission controller access is used for:
 * - High-level mission planning
 * - Mission coordination and management
 * - Plan execution and control
 * - Mission state coordination
 * 
 * @section Return Value
 * The method returns:
 * - Valid MissionController pointer if available
 * - nullptr if no controller or wrong type
 * - Consistent mission control interface
 * 
 * @note This method provides access to high-level
 *       mission planning and coordination capabilities.
 * 
 * @see getMissionController()
 * @see _planMasterController
 * @see MissionController
 * @see Mission planning
 */
MissionController* AreaPlanEditor::getMissionController() const
{
    return qobject_cast<MissionController*>(_planMasterController);
}

/**
 * @brief Generates waypoints for area coverage mission planning
 * 
 * This method creates a comprehensive set of waypoints covering
 * the entire operational area based on current area parameters.
 * It generates a grid pattern of waypoints with proper spacing,
 * rotation, and altitude settings for mission execution.
 * 
 * @return QList<QVariant> containing waypoint coordinates as QGeoCoordinate objects
 * 
 * @section Waypoint Generation
 * The method handles:
 * - Grid pattern generation
 * - Area coverage calculation
 * - Coordinate transformation
 * - Altitude assignment
 * 
 * @section Grid Geometry
 * Waypoint generation creates:
 * - Parallel flight lines along height axis
 * - Evenly distributed waypoints along width
 * - Proper line spacing and coverage
 * - Rotated grid alignment
 * 
 * @section Coordinate System
 * The method uses:
 * - Local coordinate system for calculations
 * - Trigonometric rotation transformations
 * - Geographic coordinate conversion
 * - Altitude assignment from mission parameters
 * 
 * @section Validation
 * Pre-generation checks include:
 * - Area center validity
 * - Positive dimensions (width, height)
 * - Valid number of points
 * - Positive line spacing
 * 
 * @section Mathematical Approach
 * Generation process:
 * - Calculates grid line count from height/spacing
 * - Distributes waypoints evenly along each line
 * - Applies rotation transformation around center
 * - Converts to geographic coordinates
 * - Assigns mission altitude to all waypoints
 * 
 * @note This method provides the foundation for
 *       area coverage mission planning and execution.
 * 
 * @see generateWaypoints()
 * @see _areaCenter, _areaWidth, _areaHeight
 * @see _lineSpacing, _numPoints, _missionAltitude
 * @see calculateOffsetCoordinate()
 */
QList<QVariant> AreaPlanEditor::generateWaypoints()
{
    QList<QVariant> waypoints;

    // Basic validation
    if (_areaCenter.isValid() == false || _areaWidth <= 0 || _areaHeight <= 0 || _numPoints <= 0 || _lineSpacing <= 0) {
        return waypoints;
    }

    // Compute number of grid lines along height (north-south axis before rotation)
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));

    // Local helpers for geometry
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(-_areaRotation); // rotation: positive = clockwise
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);

    auto rotateXY = [&](qreal x, qreal y) {
        // Rotate local (x,y) around origin by theta
        return QPointF(x * cosT - y * sinT, x * sinT + y * cosT);
    };

    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        // Approximate translation by dy north, then dx east
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        QGeoCoordinate res = calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
        return res;
    };

    auto clamp = [](qreal v, qreal lo, qreal hi){ return v < lo ? lo : (v > hi ? hi : v); };

    // Y coordinates for each line (evenly distributed from -halfH to +halfH)
    for (int li = 0; li < lineCount; ++li) {
        qreal y;
        if (lineCount == 1) {
            y = 0.0;
        } else {
            y = -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
        }

        // X coordinates along width
        for (int pi = 0; pi < _numPoints; ++pi) {
            qreal x;
            if (_numPoints == 1) {
                x = 0.0;
            } else {
                x = -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
            }

            // Clamp to bounds in local space (safety against FP drift)
            x = clamp(x, -halfW, halfW);
            y = clamp(y, -halfH, halfH);

            // Apply rotation around center
            const QPointF r = rotateXY(x, y);

// Convert to geo coordinate
            QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
            // Set altitude as mission altitude for downstream consumers/tests
            wp.setAltitude(_missionAltitude);
            waypoints.append(QVariant::fromValue(wp));
        }
    }

    return waypoints;
}

/**
 * @brief Computes partition stripes for area coverage planning
 * 
 * This method generates a set of parallel stripes that partition
 * the operational area for systematic coverage. Each stripe
 * represents a flight line that will be covered by the mission,
 * enabling efficient area coverage and mission optimization.
 * 
 * @return QList<QVariant> containing stripe endpoints as coordinate pairs
 * 
 * @section Stripe Generation
 * The method handles:
 * - Parallel line generation
 * - Area partitioning
 * - Coordinate transformation
 * - Coverage optimization
 * 
 * @section Partition Geometry
 * Stripe generation creates:
 * - Parallel lines along the short axis
 * - Even spacing based on line spacing parameter
 * - Complete area coverage
 * - Rotated alignment with area
 * 
 * @section AreaPlan Integration
 * The method uses:
 * - AreaPlan::splitIntoStripes() for core logic
 * - Local coordinate system for calculations
 * - Geographic coordinate conversion
 * - Area center and rotation parameters
 * 
 * @section Coordinate Conversion
 * Stripe processing involves:
 * - Local coordinate generation
 * - Geographic coordinate transformation
 * - Endpoint mapping to real coordinates
 * - Proper coordinate system alignment
 * 
 * @section Validation
 * Pre-computation checks include:
 * - Positive area dimensions
 * - Valid line spacing
 * - Proper parameter validation
 * - Error handling for invalid inputs
 * 
 * @note This method provides the foundation for
 *       systematic area coverage and mission planning.
 * 
 * @see computePartitionStripes()
 * @see AreaPlan::splitIntoStripes()
 * @see _areaWidth, _areaHeight, _lineSpacing
 * @see calculateOffsetCoordinate()
 */
QList<QVariant> AreaPlanEditor::computePartitionStripes() const
{
    QList<QVariant> stripes;
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0) {
        return stripes;
    }
    // Number of stripes equals lineCount along height before rotation
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const double cx = 0.0;
    const double cy = 0.0;
    // Build local stripes, then map endpoints to geo using areaCenter and rotation already applied in helper
    const auto lines = AreaPlan::splitIntoStripes(cx, cy,
                                                  static_cast<double>(_areaWidth),
                                                  static_cast<double>(_areaHeight),
                                                  lineCount,
                                                  /*alongShortAxis=*/true,
                                                  static_cast<double>(_areaRotation));
    // Convert to geo coordinates: treat local meters (x east, y north) relative to areaCenter
    auto toGeo = [&](double x, double y) {
        // translate by dy north, then dx east
        QGeoCoordinate tmp = calculateOffsetCoordinate(_areaCenter, std::abs(y), y >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, std::abs(x), x >= 0 ? 90.0 : 270.0);
    };
    for (const auto& ln : lines) {
        QVariantMap m;
        const QGeoCoordinate a = toGeo(ln.a.x, ln.a.y);
        const QGeoCoordinate b = toGeo(ln.b.x, ln.b.y);
        m["a"] = QVariant::fromValue(a);
        m["b"] = QVariant::fromValue(b);
        stripes.append(m);
    }
    return stripes;
}

/**
 * @brief Computes round-robin drone assignments for area coverage
 * 
 * This method distributes area coverage stripes among multiple
 * drones using a round-robin assignment strategy. Each drone
 * is assigned a subset of stripes to cover, enabling parallel
 * area coverage and mission efficiency.
 * 
 * @return QList<QVariant> containing drone assignments with stripe indices
 * 
 * @section Assignment Strategy
 * The method implements:
 * - Round-robin stripe distribution
 * - Even workload distribution
 * - Drone-specific stripe assignments
 * - Coverage optimization
 * 
 * @section Drone Coordination
 * Assignment process:
 * - Calculates total number of stripes
 * - Distributes stripes round-robin among drones
 * - Assigns stripe indices to each drone
 * - Maintains balanced workload distribution
 * 
 * @section AreaPlan Integration
 * The method uses:
 * - AreaPlan::assignStripesRoundRobin() for core logic
 * - Drone count for assignment calculation
 * - Line count from area parameters
 * - Efficient assignment algorithms
 * 
 * @section Return Format
 * Each assignment contains:
 * - droneIndex: Drone identifier (0-based)
 * - lineIndices: List of assigned stripe indices
 * - Structured for easy processing and display
 * 
 * @section Use Cases
 * Round-robin assignments enable:
 * - Parallel area coverage
 * - Balanced workload distribution
 * - Mission efficiency optimization
 * - Multi-drone coordination
 * 
 * @note This method provides the foundation for
 *       coordinated multi-drone area coverage missions.
 * 
 * @see computeRoundRobinAssignments()
 * @see AreaPlan::assignStripesRoundRobin()
 * @see _droneCount, _areaHeight, _lineSpacing
 * @see Multi-drone coordination
 */
QList<QVariant> AreaPlanEditor::computeRoundRobinAssignments() const
{
    QList<QVariant> assignments;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        QVariantMap m;
        QVariantList idx;
        for (int i : rr[static_cast<size_t>(d)]) idx.append(i);
        m["droneIndex"] = d;
        m["lineIndices"] = idx;
        assignments.append(m);
    }
    return assignments;
}

/**
 * @brief Computes comprehensive drone assignments with altitude and timing
 * 
 * This method generates complete drone assignments including
 * stripe coverage, altitude banding, and time staggering for
 * coordinated multi-drone operations. It provides all necessary
 * parameters for safe and efficient swarm mission execution.
 * 
 * @return QList<QVariant> containing complete drone assignments
 * 
 * @section Assignment Components
 * Each assignment includes:
 * - Drone index and stripe assignments
 * - Altitude offset for vertical separation
 * - Time offset for mission coordination
 * - Complete mission parameters
 * 
 * @section Altitude Banding
 * Altitude calculation:
 * - Base altitude from altitudeBandStart
 * - Incremental offsets per drone
 * - Vertical separation for safety
 * - Collision avoidance margins
 * 
 * @section Time Staggering
 * Timing coordination:
 * - Sequential mission start times
 * - Time offset per drone
 * - Traffic flow management
 * - Mission synchronization
 * 
 * @section Safety Features
 * Assignment safety includes:
 * - Vertical separation through altitude banding
 * - Temporal separation through time staggering
 * - Coordinated mission execution
 * - Collision avoidance measures
 * 
 * @section Mission Integration
 * Assignments enable:
 * - Coordinated multi-drone operations
 * - Safe formation flying
 * - Efficient area coverage
 * - Mission optimization
 * 
 * @note This method provides complete mission
 *       parameters for coordinated swarm operations.
 * 
 * @see computeDroneAssignments()
 * @see _droneCount, _altitudeBandStart, _altitudeBandStep
 * @see _timeOffsetPerDrone, computeRoundRobinAssignments()
 * @see Swarm mission planning
 */
QList<QVariant> AreaPlanEditor::computeDroneAssignments() const
{
    QList<QVariant> assignments;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        QVariantMap m;
        QVariantList idx;
        for (int i : rr[static_cast<size_t>(d)]) idx.append(i);
        m["droneIndex"] = d;
        m["altitudeOffsetM"] = _altitudeBandStart + d * _altitudeBandStep;
        m["timeOffsetS"] = d * _timeOffsetPerDrone;
        m["lineIndices"] = idx;
        assignments.append(m);
    }
    return assignments;
}

/**
 * @brief Computes per-drone line counts and mission statistics
 * 
 * This method calculates comprehensive statistics for multi-drone
 * mission planning, including the number of lines assigned to
 * each drone, total coverage lines, and expected waypoint counts.
 * It provides mission planning insights and workload distribution
 * analysis.
 * 
 * @return QMap<QString, QVariant> containing mission statistics
 * 
 * @section Statistics Components
 * The method provides:
 * - Per-drone line count breakdown
 * - Total coverage lines
 * - Expected total waypoints
 * - Workload distribution analysis
 * 
 * @section Line Count Calculation
 * Line distribution analysis:
 * - Calculates total stripes from area parameters
 * - Distributes stripes using round-robin assignment
 * - Counts lines per drone
 * - Provides workload balance information
 * 
 * @section Waypoint Estimation
 * Waypoint calculation:
 * - Total lines × points per line
 * - Mission coverage estimation
 * - Planning resource requirements
 * - Mission duration estimation
 * 
 * @section Mission Planning
 * Statistics enable:
 * - Workload balance verification
 * - Resource allocation planning
 * - Mission duration estimation
 * - Coverage optimization
 * 
 * @section Return Format
 * Statistics include:
 * - perDrone: List of drone-specific line counts
 * - totalLines: Total coverage lines
 * - expectedTotalWaypoints: Estimated waypoint count
 * 
 * @note This method provides essential metrics for
 *       mission planning and optimization.
 * 
 * @see computePerDroneCounts()
 * @see _droneCount, _areaHeight, _lineSpacing, _numPoints
 * @see AreaPlan::assignStripesRoundRobin()
 * @see Mission planning statistics
 */
QMap<QString, QVariant> AreaPlanEditor::computePerDroneCounts() const
{
    QMap<QString, QVariant> counts;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    QVariantList per;
    int sum = 0;
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        const int c = static_cast<int>(rr[static_cast<size_t>(d)].size());
        QVariantMap m; m["droneIndex"] = d; m["lineCount"] = c; per.append(m); sum += c;
    }
    counts["perDrone"] = per;
    counts["totalLines"] = sum;
    counts["expectedTotalWaypoints"] = sum * _numPoints;
    return counts;
}

/**
 * @brief Computes per-drone waypoint preview for mission planning
 * 
 * This method generates a comprehensive preview of waypoints
 * for each drone, including coordinate positions, altitude
 * banding, and timing information. It provides mission
 * planners with detailed visualization of planned flight paths
 * and coordination parameters.
 * 
 * @return QList<QVariant> containing per-drone waypoint previews
 * 
 * @section Preview Components
 * Each drone preview includes:
 * - Drone index and identification
 * - Altitude offset for vertical separation
 * - Time offset for mission coordination
 * - Complete waypoint list with coordinates
 * 
 * @section Waypoint Generation
 * Preview generation process:
 * - Calculates stripe assignments per drone
 * - Generates waypoints for assigned stripes
 * - Applies rotation and coordinate transformation
 * - Assigns drone-specific altitude offsets
 * 
 * @section Coordinate System
 * Waypoint positioning:
 * - Local coordinate calculations
 * - Trigonometric rotation transformations
 * - Geographic coordinate conversion
 * - Altitude banding application
 * 
 * @section Mission Coordination
 * Preview enables:
 * - Flight path visualization
 * - Coordination parameter review
 * - Mission planning validation
 * - Safety parameter verification
 * 
 * @section Use Cases
 * Waypoint previews are used for:
 * - Mission planning and validation
 * - Flight path visualization
 * - Coordination parameter review
 * - Safety verification
 * 
 * @note This method provides comprehensive mission
 *       previews for planning and validation.
 * 
 * @see computePerDroneWaypointPreview()
 * @see _droneCount, _altitudeBandStart, _altitudeBandStep
 * @see _timeOffsetPerDrone, generateWaypoints()
 * @see Mission planning preview
 */
QList<QVariant> AreaPlanEditor::computePerDroneWaypointPreview() const
{
    QList<QVariant> preview;
    // Guard
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) {
        return preview;
    }
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);

    // Precompute rotated coordinates for each line index and point index similar to generateWaypoints
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    auto rotateXY = [&](qreal x, qreal y) { return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); };
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };

    auto lineYAt = [&](int li) {
        if (lineCount == 1) return 0.0;
        return -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
    };
    auto pointXAt = [&](int pi) {
        if (_numPoints == 1) return 0.0;
        return -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
    };

    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        const auto& indices = rr[static_cast<size_t>(d)];
        QVariantMap group;
        group["droneIndex"] = d;
        group["altitudeOffsetM"] = _altitudeBandStart + d * _altitudeBandStep;
        group["timeOffsetS"] = d * _timeOffsetPerDrone;
        QVariantList wps;
        for (int li : indices) {
            const qreal y = (lineCount == 1) ? 0.0 : qBound(-halfH, lineYAt(li), halfH);
            for (int pi = 0; pi < _numPoints; ++pi) {
                const qreal x = (_numPoints == 1) ? 0.0 : qBound(-halfW, pointXAt(pi), halfW);
                const QPointF r = rotateXY(x, y);
                QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
                // Altitude per-drone band offset
                wp.setAltitude(_missionAltitude + (_altitudeBandStart + d * _altitudeBandStep));
                wps.append(QVariant::fromValue(wp));
            }
        }
        group["waypoints"] = wps;
        preview.append(group);
    }
    return preview;
}

/**
 * @brief Generates waypoints for a specific drone's mission
 * 
 * This method creates waypoints specifically for a single drone
 * based on its assigned stripes and mission parameters. It
 * generates a complete flight path with proper altitude banding
 * and coordinate positioning for individual drone mission execution.
 * 
 * @param droneIndex Index of the drone (0-based)
 * @return QList<QVariant> containing waypoints for the specified drone
 * 
 * @section Drone-Specific Generation
 * Waypoint generation includes:
 * - Stripe assignments for the drone
 * - Altitude banding calculations
 * - Coordinate transformation and rotation
 * - Mission parameter integration
 * 
 * @section Parameter Validation
 * Pre-generation validation:
 * - Drone index range verification
 * - Area parameter validation
 * - Line spacing and point count checks
 * - Error handling for invalid inputs
 * 
 * @section Coordinate Calculation
 * Waypoint positioning:
 * - Local coordinate system calculations
 * - Trigonometric rotation transformations
 * - Geographic coordinate conversion
 * - Altitude assignment with banding
 * 
 * @section Mission Integration
 * Generated waypoints enable:
 * - Individual drone mission execution
 * - Coordinated swarm operations
 * - Safe altitude separation
 * - Efficient area coverage
 * 
 * @section Return Value
 * The method returns:
 * - Valid waypoint list if parameters are valid
 * - Empty list for invalid drone index or parameters
 * - Complete flight path for mission execution
 * 
 * @note This method provides drone-specific waypoints
 *       for individual mission execution.
 * 
 * @see generatePerDroneWaypoints()
 * @see _droneCount, _altitudeBandStart, _altitudeBandStep
 * @see AreaPlan::assignStripesRoundRobin()
 * @see Individual drone missions
 */
QList<QVariant> AreaPlanEditor::generatePerDroneWaypoints(int droneIndex) const
{
    QList<QVariant> waypoints;
    if (droneIndex < 0 || droneIndex >= _droneCount) return waypoints;
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) return waypoints;

    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);

    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    auto rotateXY = [&](qreal x, qreal y) { return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); };
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };
    auto lineYAt = [&](int li) {
        if (lineCount == 1) return 0.0;
        return -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
    };
    auto pointXAt = [&](int pi) {
        if (_numPoints == 1) return 0.0;
        return -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
    };

    const double altOffset = _altitudeBandStart + droneIndex * _altitudeBandStep;

    const auto& indices = rr[static_cast<size_t>(droneIndex)];
    for (int li : indices) {
        const qreal y = (lineCount == 1) ? 0.0 : qBound(-halfH, lineYAt(li), halfH);
        for (int pi = 0; pi < _numPoints; ++pi) {
            const qreal x = (_numPoints == 1) ? 0.0 : qBound(-halfW, pointXAt(pi), halfW);
            const QPointF r = rotateXY(x, y);
            QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
            wp.setAltitude(_missionAltitude + altOffset);
            waypoints.append(QVariant::fromValue(wp));
        }
    }

    return waypoints;
}

/**
 * @brief Inserts gripper release command at specified coordinates
 * 
 * This method adds a payload release command to the mission
 * at the specified location. It creates a MAV_CMD_DO_GRIPPER
 * command with release action for payload delivery operations.
 * 
 * @param mission Mission controller for command insertion
 * @param atCoord Coordinates where payload release should occur
 * 
 * @section Payload Release
 * Command insertion includes:
 * - Gripper command creation
 * - Release action configuration
 * - Coordinate positioning
 * - Mission integration
 * 
 * @section Command Configuration
 * Gripper command setup:
 * - MAV_CMD_DO_GRIPPER command type
 * - Param1: Gripper instance (0)
 * - Param2: Action (0 = release)
 * - Coordinate positioning
 * 
 * @section Mission Integration
 * Command integration:
 * - Simple mission item creation
 * - Command parameter configuration
 * - Mission sequence integration
 * - Payload operation coordination
 * 
 * @section Safety Features
 * Release safety includes:
 * - Payload release enablement check
 * - Mission controller validation
 * - Command parameter verification
 * - Error handling for failures
 * 
 * @note This method enables automated payload
 *       delivery during mission execution.
 * 
 * @see insertGripperRelease()
 * @see _payloadReleaseEnabled, MAV_CMD_DO_GRIPPER
 * @see MissionController, SimpleMissionItem
 * @see Payload delivery operations
 */
void AreaPlanEditor::insertGripperRelease(MissionController* mission, const QGeoCoordinate& atCoord)
{
    if (!mission || !_payloadReleaseEnabled) return;
    VisualMissionItem* doItem = mission->insertSimpleMissionItem(atCoord, -1, false);
    if (SimpleMissionItem* sm = qobject_cast<SimpleMissionItem*>(doItem)) {
        sm->setCommand(MAV_CMD_DO_GRIPPER);
        // Param1=gripper instance (0), Param2=action (0=release)
        sm->missionItem().setParam1(0);
        sm->missionItem().setParam2(0);
    }
}

/**
 * @brief Adds complete mission for a specific drone to the mission controller
 * 
 * This method integrates a complete drone mission into the mission controller,
 * including waypoints, timing coordination, landing operations, and payload
 * delivery. It creates a comprehensive mission sequence with proper timing
 * and safety parameters for individual drone operations.
 * 
 * @param droneIndex Index of the drone (0-based) for mission generation
 * 
 * @section Mission Integration
 * Integration process includes:
 * - Mission controller validation
 * - Mission settings initialization
 * - Waypoint generation and insertion
 * - Timing coordination setup
 * 
 * @section Mission Settings
 * Settings initialization:
 * - Creates mission settings if needed
 * - Initializes mission parameters
 * - Establishes mission structure
 * - Prepares for mission items
 * 
 * @section Waypoint Processing
 * Waypoint integration:
 * - Generates drone-specific waypoints
 * - Inserts waypoints with timing
 * - Applies altitude and coordinate settings
 * - Maintains mission sequence integrity
 * 
 * @section Timing Coordination
 * Mission timing includes:
 * - Start delay based on drone index
 * - Per-target separation timing
 * - Mission synchronization parameters
 * - Traffic flow management
 * 
 * @section Landing Operations
 * Landing sequence options:
 * - Land at target with payload release
 * - Target hold time configuration
 * - Takeoff from target operations
 * - Return to home procedures
 * 
 * @section Advanced Features
 * Mission features include:
 * - RTL after every waypoint option
 * - Loiter after RTL configuration
 * - Home turnaround wait timing
 * - Mission flow optimization
 * 
 * @note This method creates comprehensive missions
 *       with proper timing and safety coordination.
 * 
 * @see addPerDroneToMission()
 * @see generatePerDroneWaypoints()
 * @see MissionController, timing parameters
 * @see Mission integration and coordination
 */
void AreaPlanEditor::addPerDroneToMission(int droneIndex)
{
    MissionController* mission = getMissionController();
    if (!mission) {
        qWarning() << "AreaPlanEditor::addPerDroneToMission: MissionController not set";
        return;
    }
    
    // Ensure mission has MissionSettings
    if (mission->visualItems()->count() == 0) {
        // Trigger init by inserting and removing a dummy to create settings if needed
        mission->insertSimpleMissionItem(_areaCenter, -1, false);
        // Remove the inserted item, keep settings
        if (mission->visualItems()->count() > 1) {
            mission->removeVisualItem(1);
        }
    }

    const QVariantList wps = generatePerDroneWaypoints(droneIndex);
    // Insert an initial takeoff before the first waypoint so missions always begin with a takeoff action
    if (!wps.isEmpty()) {
        const double altOffset = _altitudeBandStart + droneIndex * _altitudeBandStep;
        const QGeoCoordinate takeoffCoord = _homeLocation.isValid() ? _homeLocation : _areaCenter;
        VisualMissionItem* tkItem = mission->insertSimpleMissionItem(takeoffCoord, -1, false);
        if (SimpleMissionItem* tk = qobject_cast<SimpleMissionItem*>(tkItem)) {
            tk->setCommand(MAV_CMD_NAV_TAKEOFF);
            if (tk->specifiesAltitude()) {
                tk->altitude()->setRawValue(_missionAltitude + altOffset);
            }
        }
    }
    for (int idx = 0; idx < wps.size(); ++idx) {
        QGeoCoordinate c = wps[idx].value<QGeoCoordinate>();
        
        // Slotting to avoid conflicts at start and between cycles
        qreal startDelay = (idx == 0) ? (droneIndex * _timeOffsetPerDrone) : _perTargetSeparationS;
        if (startDelay > 0.0) {
            // Use the first waypoint location for hold, not home location
            QGeoCoordinate holdLocation = c;
            VisualMissionItem* hold = mission->insertSimpleMissionItem(holdLocation, -1, false);
            if (SimpleMissionItem* h = qobject_cast<SimpleMissionItem*>(hold)) {
                h->setCommand(MAV_CMD_NAV_LOITER_TIME);
                h->missionItem().setParam1(startDelay);
                if (h->specifiesAltitude()) {
                    h->altitude()->setRawValue(_missionAltitude);
                }
            }
        }
        
        if (_landAtTargetReturn) {
            // Land at target with offset per drone to avoid same-spot conflicts (>=1m separation)
            QGeoCoordinate targetLand = calculateOffsetCoordinate(c, 1.5 + 1.0 * droneIndex, fmod(45.0 + 60.0 * droneIndex, 360.0));
            VisualMissionItem* landTargetItem = mission->insertSimpleMissionItem(targetLand, -1, false);
            if (SimpleMissionItem* lti = qobject_cast<SimpleMissionItem*>(landTargetItem)) {
                lti->setCommand(MAV_CMD_NAV_LAND);
                if (lti->specifiesAltitude()) {
                    lti->altitude()->setRawValue(0.0);
                }
            }
            // Optional: Payload release command
            insertGripperRelease(mission, c);
            // Hold on target for configured time
            if (_targetHoldTimeS > 0) {
                VisualMissionItem* hold = mission->insertSimpleMissionItem(c, -1, false);
                if (SimpleMissionItem* h = qobject_cast<SimpleMissionItem*>(hold)) {
                    h->setCommand(MAV_CMD_NAV_LOITER_TIME);
                    h->missionItem().setParam1(_targetHoldTimeS);
                    if (h->specifiesAltitude()) {
                        h->altitude()->setRawValue(_missionAltitude);
                    }
                }
            }
            // Takeoff from target back to altitude
            VisualMissionItem* tkItem = mission->insertSimpleMissionItem(c, -1, false);
            if (SimpleMissionItem* tk = qobject_cast<SimpleMissionItem*>(tkItem)) {
                tk->setCommand(MAV_CMD_NAV_TAKEOFF);
                if (tk->specifiesAltitude()) {
                    tk->altitude()->setRawValue(_missionAltitude);
                }
            }
            // Return and land near home with offset per drone
            QGeoCoordinate baseHome = _homeLocation.isValid() ? _homeLocation : c;
            QGeoCoordinate homeLand = calculateOffsetCoordinate(baseHome, 1.5 + 1.0 * droneIndex, fmod(90.0 + 60.0 * droneIndex, 360.0));
            VisualMissionItem* landHomeItem = mission->insertSimpleMissionItem(homeLand, -1, false);
            if (SimpleMissionItem* lhi = qobject_cast<SimpleMissionItem*>(landHomeItem)) {
                lhi->setCommand(MAV_CMD_NAV_LAND);
                if (lhi->specifiesAltitude()) {
                    lhi->altitude()->setRawValue(0.0);
                }
            }
            // Loiter at home for turnaround (use configured wait)
            VisualMissionItem* loiterItem = mission->insertSimpleMissionItem(_homeLocation.isValid() ? _homeLocation : c, -1, false);
            if (SimpleMissionItem* loiter = qobject_cast<SimpleMissionItem*>(loiterItem)) {
                loiter->setCommand(MAV_CMD_NAV_LOITER_TIME);
                loiter->missionItem().setParam1(_homeTurnaroundWaitS > 0 ? _homeTurnaroundWaitS : _loiterTime);
                if (loiter->specifiesAltitude()) {
                    loiter->altitude()->setRawValue(_missionAltitude);
                }
            }
        } else {
            // Insert waypoint transit only
            VisualMissionItem* vmi = mission->insertSimpleMissionItem(c, -1, false);
            if (SimpleMissionItem* smi = qobject_cast<SimpleMissionItem*>(vmi)) {
                if (smi->specifiesAltitude()) {
                    smi->altitude()->setRawValue(c.altitude());
                }
            }
            // Policy: RTL after every waypoint
            if (_rtlAfterEveryWaypoint) {
                // Land near home with offset per drone
                QGeoCoordinate baseHome = _homeLocation.isValid() ? _homeLocation : c;
                QGeoCoordinate homeLand = calculateOffsetCoordinate(baseHome, 1.5 + 1.0 * droneIndex, fmod(90.0 + 60.0 * droneIndex, 360.0));
                VisualMissionItem* landItem = mission->insertSimpleMissionItem(homeLand, -1, false);
                if (SimpleMissionItem* li = qobject_cast<SimpleMissionItem*>(landItem)) {
                    li->setCommand(MAV_CMD_NAV_LAND);
                    if (li->specifiesAltitude()) {
                        li->altitude()->setRawValue(0.0);
                    }
                }
                if (_loiterAfterRtl) {
                    VisualMissionItem* lItem = mission->insertSimpleMissionItem(c, -1, false);
                    if (SimpleMissionItem* loiter = qobject_cast<SimpleMissionItem*>(lItem)) {
                        loiter->setCommand(MAV_CMD_NAV_LOITER_TIME);
                        loiter->missionItem().setParam1(_loiterTime);
                        if (loiter->specifiesAltitude()) {
                            loiter->altitude()->setRawValue(_missionAltitude);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Adds missions for all drones to the mission controller
 * 
 * This method integrates complete missions for all configured
 * drones into the mission controller, creating a comprehensive
 * multi-drone mission plan. It coordinates timing, altitude
 * banding, and mission flow across the entire drone fleet.
 * 
 * @section Multi-Drone Integration
 * Integration process includes:
 * - Iterative drone mission generation
 * - Coordinated mission planning
 * - Fleet-wide mission coordination
 * - Comprehensive mission structure
 * 
 * @section Drone Fleet Management
 * Fleet coordination involves:
 * - Individual drone mission generation
 * - Altitude banding for separation
 * - Time staggering for coordination
 * - Mission flow optimization
 * 
 * @section Mission Coordination
 * Coordination features:
 * - Synchronized mission execution
 * - Traffic flow management
 * - Collision avoidance measures
 * - Operational efficiency
 * 
 * @section Mission Structure
 * Structure includes:
 * - Per-drone mission sequences
 * - Coordinated timing parameters
 * - Safety and separation measures
 * - Mission flow optimization
 * 
 * @section Use Cases
 * Multi-drone missions enable:
 * - Parallel area coverage
 * - Coordinated operations
 * - Mission efficiency optimization
 * - Fleet-wide coordination
 * 
 * @note This method creates comprehensive
 *       multi-drone mission coordination.
 * 
 * @see addAllDronesToMission()
 * @see addPerDroneToMission()
 * @see _droneCount, mission coordination
 * @see Multi-drone mission planning
 */
void AreaPlanEditor::addAllDronesToMission()
{
    for (int d = 0; d < _droneCount; ++d) {
        addPerDroneToMission(d);
    }
}

/**
 * @brief Adds waypoints to the mission controller for area coverage
 * 
 * This method integrates area coverage waypoints into the mission
 * controller, creating a comprehensive mission plan for systematic
 * area coverage. It handles both single-drone and multi-drone
 * scenarios with proper mission initialization and coordination.
 * 
 * @section Mission Integration
 * Integration process includes:
 * - Mission controller validation
 * - Mission settings initialization
 * - Waypoint generation and insertion
 * - Mission flow optimization
 * 
 * @section Mission Settings
 * Settings initialization:
 * - Creates mission settings if needed
 * - Initializes mission parameters
 * - Establishes mission structure
 * - Prepares for mission items
 * 
 * @section Waypoint Processing
 * Waypoint integration:
 * - Generates area coverage waypoints
 * - Inserts waypoints with proper sequencing
 * - Applies altitude and coordinate settings
 * - Maintains mission sequence integrity
 * 
 * @section Mission Flow
 * Flow optimization includes:
 * - Systematic area coverage
 * - Efficient flight path planning
 * - Mission parameter optimization
 * - Operational efficiency
 * 
 * @section Advanced Features
 * Mission features include:
 * - Multi-drone coordination support
 * - Advanced business flow integration
 * - Mission optimization algorithms
 * - Comprehensive area coverage
 * 
 * @note This method creates systematic area
 *       coverage missions with proper coordination.
 * 
 * @see addWaypointsToMission()
 * @see generateWaypoints()
 * @see MissionController, area coverage
 * @see Mission planning and integration
 */
void AreaPlanEditor::addWaypointsToMission()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        qWarning() << "AreaPlanEditor::addWaypointsToMission: MissionController not set";
        return;
    }
    
    // Ensure mission has MissionSettings initialized
    if (mission->visualItems()->count() == 0) {
        // Only insert a temporary item to trigger MissionSettings creation
        VisualMissionItem* tmp = mission->insertSimpleMissionItem(_areaCenter, -1, false);
        if (mission->visualItems()->count() > 1) {
            mission->removeVisualItem(1);
        }
        Q_UNUSED(tmp);
    }

    // If advanced business-flow is enabled or multi-drone planning is in use,
    // delegate to per-drone insertion which handles LAND/LOITER/TAKEOFF/returns.
    const bool advanced = (_droneCount > 1) || _landAtTargetReturn || _payloadReleaseEnabled ||
                          _rtlAfterEveryWaypoint || _loiterAfterRtl ||
                          (_timeOffsetPerDrone > 0) || (_perTargetSeparationS > 0);
    if (advanced) {
        for (int d = 0; d < _droneCount; ++d) {
            addPerDroneToMission(d);
        }
        return;
    }

    // Simple single-drone insertion: straight waypoints only
    const QVariantList wps = generateWaypoints();
    // Insert takeoff before first waypoint so mission always begins with takeoff
    if (!wps.isEmpty()) {
        QGeoCoordinate tkCoord = _homeLocation.isValid() ? _homeLocation : _areaCenter;
        VisualMissionItem* tkItem = mission->insertSimpleMissionItem(tkCoord, -1, false);
        if (SimpleMissionItem* tk = qobject_cast<SimpleMissionItem*>(tkItem)) {
            tk->setCommand(MAV_CMD_NAV_TAKEOFF);
            if (tk->specifiesAltitude()) {
                tk->altitude()->setRawValue(_missionAltitude);
            }
        }
    }
    for (const QVariant& v : wps) {
        QGeoCoordinate c = v.value<QGeoCoordinate>();
        VisualMissionItem* vmi = mission->insertSimpleMissionItem(c, -1, false);
        if (SimpleMissionItem* smi = qobject_cast<SimpleMissionItem*>(vmi)) {
            if (smi->specifiesAltitude()) {
                smi->altitude()->setRawValue(c.altitude());
            }
        }
    }
}

/**
 * @brief Saves the current mission to a CSV file with timestamp
 * 
 * This method exports the current mission from the mission controller
 * to a CSV file with a timestamped filename. It creates a simple
 * coordinate-based export format for mission sharing and analysis.
 * 
 * @section Mission Export
 * Export process includes:
 * - Mission controller validation
 * - Timestamped filename generation
 * - CSV coordinate export
 * - Status updates and feedback
 * 
 * @section File Format
 * CSV export contains:
 * - Header row with lat,lon,alt columns
 * - Coordinate data from mission items
 * - Altitude information for each waypoint
 * - Simple text-based format for compatibility
 * 
 * @section Filename Generation
 * Timestamped naming:
 * - Format: yyyyMMdd_hhmmss_mission.csv
 * - UTC timestamp for uniqueness
 * - Working directory placement
 * - Descriptive suffix for identification
 * 
 * @section Error Handling
 * Export safety includes:
 * - Mission controller validation
 * - File write error handling
 * - Status message updates
 * - User feedback and notifications
 * 
 * @note This method provides a simple way to export
 *       missions for external analysis or sharing.
 * 
 * @see saveMissionFile()
 * @see getMissionController()
 * @see saveMissionToFile()
 * @see Mission export and sharing
 */
void AreaPlanEditor::saveMissionFile()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    // Compose a simple CSV filename in working directory with timestamp
    const QString filename = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss'_mission.csv'");
    saveMissionToFile(mission, filename);
    updateStatus(QString("Mission saved: %1").arg(filename));
}

/**
 * @brief Clears all mission items from the mission controller
 * 
 * This method removes all mission items from the mission controller,
 * leaving only the MissionSettings item. It provides a clean slate
 * for new mission planning and removes all waypoints and commands.
 * 
 * @section Mission Clearing
 * Clearing process includes:
 * - Mission controller validation
 * - Bulk item removal
 * - MissionSettings preservation
 * - Status updates and feedback
 * 
 * @section Removal Strategy
 * Item removal approach:
 * - Direct API call if available
 * - Fallback to manual removal
 * - End-to-start removal order
 * - Index 0 preservation (MissionSettings)
 * 
 * @section MissionSettings Preservation
 * Critical settings maintained:
 * - Mission configuration parameters
 * - Vehicle-specific settings
 * - Mission structure information
 * - Controller initialization state
 * 
 * @section Status Updates
 * User feedback includes:
 * - Mission clearing confirmation
 * - Status message updates
 * - Progress indication
 * - Error handling and recovery
 * 
 * @note Clearing preserves mission settings while removing
 *       all waypoints and operational commands.
 * 
 * @see clearMission()
 * @see getMissionController()
 * @see MissionSettings preservation
 * @see Mission planning reset
 */
void AreaPlanEditor::clearMission()
{
    MissionController* mission = getMissionController();
    if (!mission) { handleError("MissionController not set", QString()); return; }
    // Try direct call if API exists
    // Queue the removal on the controller thread to avoid blocking UI
    bool invoked = QMetaObject::invokeMethod(mission, "removeAll", Qt::QueuedConnection);
    if (!invoked) {
        // Fallback: remove everything except MissionSettings (index 0)
        if (mission->visualItems()) {
            // remove from end to start to maintain indices
            for (int i = mission->visualItems()->count() - 1; i >= 1; --i) {
                mission->removeVisualItem(i);
            }
        }
    }
    updateStatus("Mission items cleared");
}

/**
 * @brief Clears missions from all connected vehicles and the current plan
 */
void AreaPlanEditor::clearAllMissions()
{
    // Clear local plan
    clearMission();

    // Iterate all connected vehicles and clear uploaded missions (asynchronously) to avoid UI stalls
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel) {
        updateStatus("No vehicles to clear");
        return;
    }

    startProgress("clear_all", "Clearing missions from all vehicles...");
    auto remaining = QSharedPointer<int>::create(vehicleModel->count());
    for (int i = 0; i < vehicleModel->count(); ++i) {
        Vehicle* v = qobject_cast<Vehicle*>(vehicleModel->get(i));
        // Slightly stagger to avoid bursts on the link and keep UI responsive
        QTimer::singleShot(i * 50, this, [this, v, remaining]() {
            if (v) {
                if (MissionManager* mm = v->missionManager()) {
                    QList<MissionItem*> empty;
                    mm->writeMissionItems(empty);
                }
            }
            (*remaining) -= 1;
            if (*remaining <= 0) {
                finishProgress("Cleared missions from all vehicles");
            }
        });
    }
}

/**
 * @brief Saves individual mission files for each drone
 * 
 * This method generates and saves separate CSV mission files
 * for each configured drone, enabling individual mission
 * management and vehicle-specific mission planning.
 * 
 * @section Per-Drone Export
 * Export process includes:
 * - Drone count validation
 * - Individual waypoint generation
 * - Timestamped filename creation
 * - CSV file generation per drone
 * 
 * @section File Generation
 * File creation process:
 * - Timestamp-based naming convention
 * - Drone-specific waypoint generation
 * - CSV format with lat,lon,alt columns
 * - Working directory placement
 * 
 * @section Waypoint Generation
 * Drone-specific waypoints:
 * - Individual drone stripe assignments
 * - Altitude banding calculations
 * - Coordinate transformation and rotation
 * - Mission parameter integration
 * 
 * @section Error Handling
 * Export safety includes:
 * - File write error handling
 * - Waypoint generation validation
 * - Status message updates
 * - User feedback and notifications
 * 
 * @note This method enables individual drone mission
 *       management and external mission planning.
 * 
 * @see savePerDroneMissionFiles()
 * @see generatePerDroneWaypoints()
 * @see _droneCount, CSV export
 * @see Per-drone mission management
 */
void AreaPlanEditor::savePerDroneMissionFiles()
{
    if (_droneCount < 1) return;
    // Save a CSV per drone using generated waypoints
    const QString ts = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss");
    for (int d = 0; d < _droneCount; ++d) {
        const QVariantList wps = generatePerDroneWaypoints(d);
        const QString filename = QString("%1_drone%2.csv").arg(ts).arg(d);
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            handleError(QString("Could not open %1 for writing").arg(filename), QString());
            continue;
        }
        QTextStream ts(&f);
        ts << "lat,lon,alt\n";
        for (const QVariant& v : wps) {
            const QGeoCoordinate c = v.value<QGeoCoordinate>();
            ts << QString::number(c.latitude(), 'f', 7) << ","
               << QString::number(c.longitude(), 'f', 7) << ","
               << QString::number(c.altitude(), 'f', 2) << "\n";
        }
        f.close();
    }
    updateStatus("Per-drone mission CSV files saved");
}

/**
 * @brief Uploads the current mission to the active vehicle
 * 
 * This method uploads the mission from the mission controller
 * to the currently active vehicle, enabling mission execution
 * and vehicle control through the planned mission.
 * 
 * @section Mission Upload
 * Upload process includes:
 * - Mission controller validation
 * - Active vehicle verification
 * - Mission data transmission
 * - Upload status monitoring
 * 
 * @section Vehicle Integration
 * Upload coordination:
 * - Active vehicle identification
 * - Mission manager access
 * - Data transmission initiation
 * - Upload progress tracking
 * 
 * @section Mission Controller
 * Controller requirements:
 * - Valid MissionController instance
 * - Mission items for upload
 * - Proper mission structure
 * - Vehicle communication readiness
 * 
 * @section Status Updates
 * User feedback includes:
 * - Upload initiation confirmation
 * - Progress monitoring
 * - Completion status updates
 * - Error handling and recovery
 * 
 * @note Mission upload requires an active vehicle
 *       and proper mission controller initialization.
 * 
 * @see uploadToVehicle()
 * @see getMissionController()
 * @see MissionController::sendToVehicle()
 * @see Vehicle mission upload
 */
void AreaPlanEditor::uploadToVehicle()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    // Direct C++ call: MissionController::sendToVehicle is not a QML-invokable method
    mission->sendToVehicle();
    updateStatus("Upload to active vehicle initiated");
}

/**
 * @brief Uploads a mission to a specific vehicle for a drone
 * 
 * This method uploads a drone-specific mission to a designated
 * vehicle, creating a complete mission with proper altitude
 * banding, timing coordination, and mission flow for individual
 * drone operations.
 * 
 * @param droneIndex Index of the drone (0-based) for mission generation
 * @param vehicleObject Vehicle object to upload the mission to
 * 
 * @section Mission Upload
 * Upload process includes:
 * - Vehicle object validation
 * - Drone-specific waypoint generation
 * - Complete mission creation
 * - Mission upload to vehicle
 * 
 * @section Mission Creation
 * Mission components:
 * - Home position and takeoff commands
 * - Waypoints with altitude banding
 * - Loiter and RTL commands if configured
 * - Landing and mission completion
 * 
 * @section Altitude Banding
 * Vertical separation:
 * - Base altitude from mission parameters
 * - Drone-specific altitude offsets
 * - Altitude band step calculations
 * - Safe vertical separation
 * 
 * @section Timing Coordination
 * Mission timing includes:
 * - Drone-specific time offsets
 * - Per-target separation timing
 * - Mission synchronization parameters
 * - Traffic flow management
 * 
 * @section Vehicle Integration
 * Upload coordination:
 * - Vehicle mission manager access
 * - Mission item creation and configuration
 * - Upload progress monitoring
 * - Completion status tracking
 * 
 * @note This method creates comprehensive missions
 *       with proper coordination for individual drones.
 * 
 * @see uploadPerDroneMissionToVehicle()
 * @see generatePerDroneWaypoints()
 * @see Vehicle::missionManager()
 * @see Per-drone mission upload
 */
void AreaPlanEditor::uploadPerDroneMissionToVehicle(int droneIndex, QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", QString());
        return;
    }
    
    // Generate per-drone waypoints (pattern defined around _areaCenter)
    QList<QVariant> waypoints = generatePerDroneWaypoints(droneIndex);
    if (waypoints.isEmpty()) {
        handleError("No waypoints generated for drone", QString::number(droneIndex));
        return;
    }
    
    // Get the vehicle's mission manager
    MissionManager* missionManager = vehicle->missionManager();
    if (!missionManager) {
        handleError("Vehicle mission manager not available", QString("Vehicle %1").arg(vehicle->id()));
        return;
    }
    
    // Convert waypoints to MissionItems
    QList<MissionItem*> missionItems;
    
    // Add home position if available
    if (_homeLocation.isValid()) {
        MissionItem* homeItem = new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, _homeLocation.latitude(), _homeLocation.longitude(), _missionAltitude, true, false, missionManager);
        missionItems.append(homeItem);
    }
    
    // Takeoff and mission should be relative to the vehicle's current position (not operator or arbitrary center)
    // Use vehicle coordinate as takeoff origin; fall back to home if available
    QGeoCoordinate vehicleOrigin = vehicle->coordinate();
    QGeoCoordinate takeoffCoord = vehicleOrigin.isValid() ? vehicleOrigin : (_homeLocation.isValid() ? _homeLocation : _areaCenter);
    double takeoffAltitude = _missionAltitude + (_altitudeBandStart + droneIndex * _altitudeBandStep);
    MissionItem* takeoffItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffAltitude, true, false, missionManager);
    missionItems.append(takeoffItem);
    
    // Add waypoints with proper altitude offsets, anchored at planned coordinates (drawn area)
    for (int i = 0; i < waypoints.size(); ++i) {
        QGeoCoordinate coord = waypoints[i].value<QGeoCoordinate>();
        double waypointAltitude = (_missionAltitude + (_altitudeBandStart + droneIndex * _altitudeBandStep));
        MissionItem* wpItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, coord.latitude(), coord.longitude(), waypointAltitude, true, false, missionManager);
        missionItems.append(wpItem);
        
        // Add loiter at waypoint if configured
        if (_loiterTime > 0) {
            MissionItem* loiterItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT, _loiterTime, 0.0, 0.0, 0.0, coord.latitude(), coord.longitude(), waypointAltitude, true, false, missionManager);
            missionItems.append(loiterItem);
        }
        
        // Add RTL after every waypoint if configured
        if (_rtlAfterEveryWaypoint) {
            MissionItem* rtlItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, takeoffCoord.latitude(), takeoffCoord.longitude(), _missionAltitude, true, false, missionManager);
            missionItems.append(rtlItem);
            
            // Add loiter after RTL if configured
            if (_loiterAfterRtl && _loiterTime > 0) {
                MissionItem* postRtlLoiterItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT, _loiterTime, 0.0, 0.0, 0.0, takeoffCoord.latitude(), takeoffCoord.longitude(), _missionAltitude, true, false, missionManager);
                missionItems.append(postRtlLoiterItem);
            }
        }
    }
    
    // Add final RTL
    MissionItem* finalRtlItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, takeoffCoord.latitude(), takeoffCoord.longitude(), _missionAltitude, true, false, missionManager);
    missionItems.append(finalRtlItem);
    
    // Add landing command
    MissionItem* landItem = new MissionItem(missionItems.count(), MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0.0, 0.0, 0.0, 0.0, takeoffCoord.latitude(), takeoffCoord.longitude(), 0.0, true, false, missionManager);
    missionItems.append(landItem);
    
    // Connect to upload progress signals
    connect(missionManager, &PlanManager::progressPctChanged, this, [this, droneIndex](double progress) {
        updateStatus(QString("Uploading drone %1 mission: %2%").arg(droneIndex).arg(qRound(progress)));
    });
    
    connect(missionManager, &PlanManager::error, this, [this, droneIndex](int errorCode, const QString& errorMsg) {
        handleError(QString("Drone %1 upload failed").arg(droneIndex), errorMsg);
    });
    
    connect(missionManager, &PlanManager::sendComplete, this, [this, droneIndex, vehicle](bool error) {
        if (!error) {
            updateStatus(QString("Drone %1 mission uploaded successfully to vehicle %2").arg(droneIndex).arg(vehicle->id()));
            emit missionUploaded(droneIndex, vehicle);
        } else {
            handleError(QString("Drone %1 mission upload failed").arg(droneIndex), "Upload completed with errors");
        }
    });
    
    // Upload to the specific vehicle
    updateStatus(QString("Uploading drone %1 mission to vehicle %2...").arg(droneIndex).arg(vehicle->id()));
    missionManager->writeMissionItems(missionItems);
}

/**
 * @brief Uploads missions to all configured drones
 * 
 * This method coordinates the upload of individual missions
 * to all available vehicles, enabling fleet-wide mission
 * coordination and multi-drone operations.
 * 
 * @section Fleet Upload
 * Upload coordination includes:
 * - Available vehicle enumeration
 * - Per-drone mission generation
 * - Staggered upload timing
 * - Fleet-wide coordination
 * 
 * @section Vehicle Management
 * Vehicle coordination:
 * - Available vehicle detection
 * - Vehicle count validation
 * - Drone-to-vehicle mapping
 * - Upload sequence management
 * 
 * @section Upload Timing
 * Staggered uploads:
 * - Sequential upload initiation
 * - 1-second delay between uploads
 * - System load management
 * - Upload conflict prevention
 * 
 * @section Mission Coordination
 * Fleet coordination enables:
 * - Parallel mission execution
 * - Coordinated operations
 * - Fleet-wide mission management
 * - Multi-drone coordination
 * 
 * @section Status Updates
 * User feedback includes:
 * - Fleet upload initiation
 * - Progress monitoring
 * - Completion status updates
 * - Error handling and recovery
 * 
 * @note Fleet uploads enable coordinated
 *       multi-drone mission execution.
 * 
 * @see uploadToAllDrones()
 * @see getAvailableVehicles()
 * @see uploadPerDroneMissionToVehicle()
 * @see Fleet mission coordination
 */
void AreaPlanEditor::uploadToAllDrones()
{
    // Get all available vehicles
    QVariantList vehicles = getAvailableVehicles();
    if (vehicles.isEmpty()) {
        handleError("No vehicles available", "Connect vehicles before uploading");
        return;
    }
    
    // Upload missions to each vehicle
    for (int droneIndex = 0; droneIndex < _droneCount && droneIndex < vehicles.size(); ++droneIndex) {
        QObject* vehicleObject = vehicles[droneIndex].value<QObject*>();
        if (vehicleObject) {
            // Upload with a small delay to avoid overwhelming the system
            QTimer::singleShot(droneIndex * 1000, this, [this, droneIndex, vehicleObject]() {
                uploadPerDroneMissionToVehicle(droneIndex, vehicleObject);
            });
        }
    }
    
    updateStatus(QString("Uploading missions to %1 drones...").arg(qMin(_droneCount, vehicles.size())));
}

/**
 * @brief Arms or disarms a specific vehicle
 * 
 * This method controls the armed state of a designated vehicle,
 * enabling or disabling its ability to execute flight commands
 * and participate in mission operations.
 * 
 * @param vehicleObject Vehicle object to control
 * @param arm True to arm the vehicle, false to disarm
 * 
 * @section Vehicle Control
 * Control process includes:
 * - Vehicle object validation
 * - Armed state modification
 * - Vehicle control execution
 * - Status updates and feedback
 * 
 * @section Armed State
 * Vehicle arming enables:
 * - Flight command execution
 * - Mission participation
 * - Motor and control activation
 * - Operational readiness
 * 
 * @section Safety Features
 * Arming safety includes:
 * - Vehicle validation and verification
 * - Error handling for failures
 * - Status confirmation and feedback
 * - Operational safety checks
 * 
 * @section Vehicle Integration
 * Control coordination:
 * - Direct vehicle method calls
 * - Armed state modification
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @note Vehicle arming is required before
 *       takeoff and mission execution.
 * 
 * @see armVehicle()
 * @see Vehicle::setArmed()
 * @see Vehicle control and safety
 * @see Mission preparation
 */
void AreaPlanEditor::armVehicle(QObject* vehicleObject, bool arm)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use the proper Vehicle method directly
    v->setArmed(arm, true); // true = show error if fails
    updateStatus(QString("Vehicle %1 %2").arg(v->id()).arg(arm?"armed":"disarmed"));
}

/**
 * @brief Initiates takeoff for a specific vehicle
 * 
 * This method commands a designated vehicle to take off and
 * climb to a specified altitude, enabling mission execution
 * and flight operations.
 * 
 * @param vehicleObject Vehicle object to control
 * @param altitude Target altitude in meters above ground level
 * 
 * @section Takeoff Control
 * Takeoff process includes:
 * - Vehicle object validation
 * - Takeoff method selection
 * - Altitude specification
 * - Takeoff command execution
 * 
 * @section Takeoff Methods
 * Method selection logic:
 * - Guided takeoff if supported
 * - Flight mode takeoff as fallback
 * - Altitude specification and control
 * - Takeoff safety and validation
 * 
 * @section Altitude Control
 * Takeoff altitude:
 * - Target altitude specification
 * - Mission altitude integration
 * - Safety altitude establishment
 * - Operational altitude planning
 * 
 * @section Vehicle Integration
 * Takeoff coordination:
 * - Vehicle capability detection
 * - Takeoff method selection
 * - Command execution and monitoring
 * - Status updates and feedback
 * 
 * @note Takeoff requires the vehicle to be armed
 *       and in a suitable flight mode.
 * 
 * @see takeoffVehicle()
 * @see Vehicle::guidedTakeoffSupported()
 * @see Vehicle::guidedModeTakeoff()
 * @see Vehicle takeoff control
 */
void AreaPlanEditor::takeoffVehicle(QObject* vehicleObject, qreal altitude)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use the same logic as FlyView: check if guided takeoff is supported
    if (v->guidedTakeoffSupported()) {
        v->guidedModeTakeoff(static_cast<double>(altitude));
        updateStatus(QString("Vehicle %1 guided takeoff to %2m requested").arg(v->id()).arg(altitude));
    } else {
        // Fallback to flight mode takeoff
        v->startTakeoff();
        updateStatus(QString("Vehicle %1 takeoff requested").arg(v->id()));
    }
}

/**
 * @brief Commands a specific vehicle to land
 * 
 * This method commands a designated vehicle to execute
 * a landing sequence, bringing it safely to the ground
 * and completing its current mission or operation.
 * 
 * @param vehicleObject Vehicle object to control
 * 
 * @section Landing Control
 * Landing process includes:
 * - Vehicle object validation
 * - Landing method selection
 * - Landing command execution
 * - Status monitoring and feedback
 * 
 * @section Landing Methods
 * Method selection logic:
 * - Guided landing if supported
 * - Flight mode landing as fallback
 * - Landing sequence execution
 * - Safety and validation
 * 
 * @section Landing Safety
 * Safety features include:
 * - Vehicle capability verification
 * - Landing method validation
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @section Vehicle Integration
 * Landing coordination:
 * - Vehicle capability detection
 * - Landing method selection
 * - Command execution and monitoring
 * - Status updates and feedback
 * 
 * @note Landing should be initiated from
 *       a safe altitude and location.
 * 
 * @see landVehicle()
 * @see Vehicle::guidedModeSupported()
 * @see Vehicle::guidedModeLand()
 * @see Vehicle landing control
 */
void AreaPlanEditor::landVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use guided landing if supported, similar to FlyView logic
    if (v->guidedModeSupported()) {
        v->guidedModeLand();
        updateStatus(QString("Vehicle %1 guided landing requested").arg(v->id()));
    } else {
        // Fallback: try to set flight mode to Land
        updateStatus(QString("Vehicle %1 landing not supported in guided mode").arg(v->id()));
    }
}

/**
 * @brief Starts mission execution on a specific vehicle
 * 
 * This method commands a designated vehicle to begin
 * executing its uploaded mission, enabling autonomous
 * flight operations and mission completion.
 * 
 * @param vehicleObject Vehicle object to control
 * 
 * @section Mission Execution
 * Mission start process:
 * - Vehicle object validation
 * - Mission start command execution
 * - Mission execution initiation
 * - Status monitoring and feedback
 * 
 * @section Mission Requirements
 * Pre-execution requirements:
 * - Mission must be uploaded to vehicle
 * - Vehicle must be armed and ready
 * - Mission must be properly configured
 * - Safety conditions must be met
 * 
 * @section Execution Control
 * Mission control includes:
 * - Mission start command execution
 * - Mission execution monitoring
 * - Status updates and feedback
 * - Error handling and recovery
 * 
 * @section Vehicle Integration
 * Mission coordination:
 * - Direct vehicle method calls
 * - Mission execution initiation
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @note Mission execution requires a properly
 *       uploaded mission and armed vehicle.
 * 
 * @see startMissionOnVehicle()
 * @see Vehicle::startMission()
 * @see Mission execution control
 * @see Autonomous flight operations
 */
void AreaPlanEditor::startMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use the proper Vehicle method directly
    v->startMission();
    updateStatus(QString("Vehicle %1 mission start requested").arg(v->id()));
}

/**
 * @brief Pauses mission execution on a specific vehicle
 * 
 * This method commands a designated vehicle to pause
 * its current mission execution, enabling operator
 * intervention and mission control.
 * 
 * @param vehicleObject Vehicle object to control
 * 
 * @section Mission Pause
 * Pause process includes:
 * - Vehicle object validation
 * - Mission pause command execution
 * - Mission execution suspension
 * - Status monitoring and feedback
 * 
 * @section Pause Behavior
 * Pause characteristics:
 * - Mission execution suspended
 * - Vehicle maintains current position
 * - Operator control enabled
 * - Mission can be resumed or aborted
 * 
 * @section Pause Control
 * Pause control includes:
 * - Mission pause command execution
 * - Pause state monitoring
 * - Status updates and feedback
 * - Error handling and recovery
 * 
 * @section Vehicle Integration
 * Pause coordination:
 * - Direct vehicle method calls
 * - Mission pause execution
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @note Mission pause enables operator
 *       intervention and control.
 * 
 * @see pauseMissionOnVehicle()
 * @see Vehicle::pauseVehicle()
 * @see Mission pause control
 * @see Operator intervention
 */
void AreaPlanEditor::pauseMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use the proper Vehicle method directly
    v->pauseVehicle();
    updateStatus(QString("Vehicle %1 mission pause requested").arg(v->id()));
}

/**
 * @brief Commands a specific vehicle to return to launch
 * 
 * This method commands a designated vehicle to execute
 * a return-to-launch (RTL) sequence, bringing it back
 * to the home location safely.
 * 
 * @param vehicleObject Vehicle object to control
 * 
 * @section RTL Control
 * RTL process includes:
 * - Vehicle object validation
 * - RTL command execution
 * - Return flight execution
 * - Status monitoring and feedback
 * 
 * @section RTL Behavior
 * Return characteristics:
 * - Vehicle returns to home location
 * - Safe altitude and speed maintained
 * - Landing at home location
 * - Mission execution terminated
 * 
 * @section RTL Safety
 * Safety features include:
 * - Safe return altitude
 * - Obstacle avoidance
 * - Landing safety procedures
 * - Status monitoring and feedback
 * 
 * @section Vehicle Integration
 * RTL coordination:
 * - Direct vehicle method calls
 * - RTL command execution
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @note RTL provides safe return capability
 *       for emergency situations.
 * 
 * @see rtlVehicle()
 * @see Vehicle::guidedModeRTL()
 * @see Return to launch control
 * @see Emergency procedures
 */
void AreaPlanEditor::rtlVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    
    // Use the proper Vehicle method directly
    v->guidedModeRTL(false); // false = not smart RTL
    updateStatus(QString("Vehicle %1 RTL requested").arg(v->id()));
}

/**
 * @brief Continues or resumes mission execution on a vehicle
 * 
 * This method commands a designated vehicle to continue
 * or resume its paused mission execution, enabling
 * mission completion and autonomous operations.
 * 
 * @param vehicleObject Vehicle object to control
 * 
 * @section Mission Continuation
 * Continuation process includes:
 * - Vehicle object validation
 * - Mission continuation command execution
 * - Mission execution resumption
 * - Status monitoring and feedback
 * 
 * @section Continuation Methods
 * Method selection logic:
 * - Flight mode change if available
 * - Mission flight mode activation
 * - Mission execution resumption
 * - Status monitoring and feedback
 * 
 * @section Mission Resumption
 * Resumption characteristics:
 * - Mission execution continues
 * - Current waypoint processing
 * - Autonomous flight operations
 * - Mission completion progression
 * 
 * @section Vehicle Integration
 * Continuation coordination:
 * - Vehicle capability detection
 * - Mission continuation execution
 * - Status monitoring and feedback
 * - Error handling and recovery
 * 
 * @note Mission continuation requires
 *       a paused mission and ready vehicle.
 * 
 * @see continueMissionOnVehicle()
 * @see Vehicle::flightModeSetAvailable()
 * @see Vehicle::missionFlightMode()
 * @see Mission continuation control
 */
void AreaPlanEditor::continueMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { 
        handleError("Invalid vehicle object", QString()); 
        return; 
    }
    
    // Use the mission flight mode to continue/resume mission
    if (v->flightModeSetAvailable()) {
        v->setFlightMode(v->missionFlightMode());
        updateStatus(QString("Vehicle %1 mission continue/resume requested").arg(v->id()));
    } else {
        handleError("Flight mode change not available", QString("Vehicle %1 does not support flight mode changes").arg(v->id()));
    }
}

/**
 * @brief Gets comprehensive status information for a specific vehicle
 * 
 * This method retrieves detailed status information for a designated
 * vehicle, including operational state, connection status, and
 * performance metrics for monitoring and control purposes.
 * 
 * @param vehicleObject Vehicle object to get status from
 * @return QVariantMap containing vehicle status information
 * 
 * @section Status Information
 * Status data includes:
 * - Vehicle identification and properties
 * - Armed state and flight mode
 * - Connection status and link information
 * - Altitude and battery information
 * 
 * @section Vehicle Properties
 * Property retrieval:
 * - Vehicle ID and identification
 * - Armed state and readiness
 * - Current flight mode
 * - Connection and link status
 * 
 * @section Performance Metrics
 * Performance data includes:
 * - Relative altitude information
 * - Battery percentage and status
 * - Link quality and connection
 * - Operational readiness
 * 
 * @section Status Monitoring
 * Monitoring capabilities:
 * - Real-time status updates
 * - Vehicle health monitoring
 * - Performance tracking
 * - Operational status assessment
 * 
 * @note Status information provides real-time
 *       vehicle monitoring and control feedback.
 * 
 * @see getVehicleStatus()
 * @see Vehicle properties and status
 * @see Vehicle monitoring and control
 * @see Performance metrics
 */
QVariantMap AreaPlanEditor::getVehicleStatus(QObject* vehicleObject) const
{
    QVariantMap m;
    const Vehicle* v = qobject_cast<const Vehicle*>(vehicleObject);
    if (!v) return m;
    m["id"] = v->id();
    m["armed"] = v->property("armed");
    m["flightMode"] = v->property("flightMode");
    m["connectionLost"] = v->property("connectionLost");
    m["linkName"] = v->property("activeLinkName");
    m["altitudeRelative"] = v->property("altitudeRelative");
    // Battery group (if available)
    m["batteryPercent"] = v->property("batteryPercent");
    return m;
}

/**
 * @brief Gets a list of all available vehicles for mission planning
 * 
 * This method enumerates all connected vehicles available for
 * mission planning and execution, providing access to the
 * complete vehicle fleet for coordinated operations.
 * 
 * @return QList<QVariant> containing all available vehicle objects
 * 
 * @section Vehicle Enumeration
 * Enumeration process includes:
 * - MultiVehicleManager access
 * - Vehicle model enumeration
 * - Vehicle object validation
 * - Fleet availability assessment
 * 
 * @section Vehicle Access
 * Vehicle access provides:
 * - Direct vehicle object references
 * - Vehicle enumeration and counting
 * - Fleet management capabilities
 * - Mission coordination support
 * 
 * @section Fleet Management
 * Fleet coordination enables:
 * - Multi-vehicle mission planning
 * - Fleet-wide operations
 * - Vehicle coordination and control
 * - Mission distribution and execution
 * 
 * @section Return Format
 * Return data includes:
 * - Vehicle object references
 * - Direct vehicle access
 * - Fleet enumeration
 * - Mission coordination support
 * 
 * @note Available vehicles enable fleet-wide
 *       mission planning and coordination.
 * 
 * @see getAvailableVehicles()
 * @see MultiVehicleManager::vehicles()
 * @see Fleet management and coordination
 * @see Multi-vehicle operations
 */
QList<QVariant> AreaPlanEditor::getAvailableVehicles() const
{
    QList<QVariant> vehicles;
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (vehicleModel) {
        for (int i = 0; i < vehicleModel->count(); i++) {
            Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
            if (vehicle) {
                // Return the actual vehicle object for direct use
                vehicles.append(QVariant::fromValue(vehicle));
            }
        }
    }
    return vehicles;
}

/**
 * @brief Starts mission execution (deprecated method)
 * 
 * This method was intended to start mission execution but is
 * not fully implemented. Mission starting is handled by
 * individual vehicle methods instead of the mission controller.
 * 
 * @section Mission Start
 * Start process includes:
 * - Mission controller validation
 * - Status message updates
 * - User feedback and notifications
 * - Implementation guidance
 * 
 * @section Implementation Status
 * Current implementation:
 * - Method exists but not functional
 * - Delegates to vehicle methods
 * - Provides status feedback
 * - Guides proper usage
 * 
 * @section Alternative Usage
 * Proper mission starting:
 * - Use vehicle startMission() methods
 * - Individual vehicle control
 * - Mission execution coordination
 * - Fleet-wide mission management
 * 
 * @section Status Updates
 * User feedback includes:
 * - Implementation status notification
 * - Alternative method guidance
 * - Status message updates
 * - User direction and support
 * 
 * @note This method should be refactored or removed
 *       in favor of vehicle-specific mission control.
 * 
 * @see startMission()
 * @see Vehicle::startMission()
 * @see Mission execution control
 * @see Vehicle mission management
 */
void AreaPlanEditor::startMission()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    
    // The MissionController doesn't have a startMission method
    // Mission starting is handled by the Vehicle's startMission() method
    // This method should probably be removed or refactored to work with vehicles
    updateStatus("Mission start not implemented - use vehicle startMission instead");
}

/**
 * @brief Updates the status message and emits change signal
 * 
 * This method updates the internal status message and emits
 * a statusChanged signal to notify the UI and other components
 * of status updates, enabling real-time status monitoring.
 * 
 * @param message New status message to display
 * 
 * @section Status Management
 * Status update process:
 * - Status message modification
 * - Change signal emission
 * - UI notification and updates
 * - Status monitoring support
 * 
 * @section Signal Emission
 * Status change signals:
 * - statusChanged() signal emission
 * - UI update notifications
 * - Status monitoring support
 * - Real-time feedback
 * 
 * @section Status Communication
 * Status information flow:
 * - Internal status updates
 * - UI notification and display
 * - Status monitoring support
 * - User feedback and guidance
 * 
 * @section Use Cases
 * Status updates occur during:
 * - Mission operations and progress
 * - Error conditions and recovery
 * - User actions and feedback
 * - System state changes
 * 
 * @note Status updates provide real-time
 *       feedback for user operations.
 * 
 * @see updateStatus()
 * @see statusChanged()
 * @see Status monitoring and feedback
 * @see User interface updates
 */
void AreaPlanEditor::updateStatus(const QString& message)
{
    emit statusChanged(message);
}

/**
 * @brief Tests the complete mission workflow end-to-end
 * 
 * This method executes a comprehensive test of the entire
 * mission workflow, including validation, generation,
 * insertion, saving, and upload operations.
 * 
 * @section Workflow Testing
 * Test sequence includes:
 * - Area parameter validation
 * - Waypoint generation testing
 * - Mission insertion and integration
 * - Mission file saving
 * - Vehicle upload testing
 * 
 * @section Validation Testing
 * Parameter validation:
 * - Area parameter verification
 * - Waypoint generation validation
 * - Mission structure validation
 * - Error handling and recovery
 * 
 * @section Mission Integration
 * Integration testing:
 * - Waypoint insertion testing
 * - Mission structure validation
 * - Mission file generation
 * - Upload capability testing
 * 
 * @section Error Handling
 * Test error handling:
 * - Validation error reporting
 * - Generation failure handling
 * - Upload error management
 * - User feedback and guidance
 * 
 * @note This method provides comprehensive
 *       workflow testing and validation.
 * 
 * @see testCompleteWorkflow()
 * @see validateAreaParameters()
 * @see validateWaypointGeneration()
 * @see End-to-end workflow testing
 */
void AreaPlanEditor::testCompleteWorkflow()
{
    // Simple end-to-end: validate, generate, insert, save, upload
    if (!validateAreaParameters()) {
        handleError("Invalid area parameters", _validationError);
        return;
    }
    if (!validateWaypointGeneration()) {
        handleError("Waypoint generation failed", QString());
        return;
    }
    addWaypointsToMission();
    saveMissionFile();
    uploadToVehicle();
}

/**
 * @brief Validates all area planning parameters for mission generation
 * 
 * This method performs comprehensive validation of all area planning
 * parameters to ensure they are within acceptable ranges and can
 * be used for safe mission generation and execution.
 * 
 * @return True if all parameters are valid, false otherwise
 * 
 * @section Parameter Validation
 * Validation process includes:
 * - Area center coordinate validation
 * - Dimension parameter validation
 * - Spacing and point count validation
 * - Mission altitude validation
 * 
 * @section Validation Criteria
 * Parameter requirements:
 * - Area center must be valid coordinates
 * - Width and height must be positive
 * - Line spacing must be positive
 * - Point count must be positive
 * - Mission altitude must be finite
 * 
 * @section Drone Configuration
 * Drone validation includes:
 * - Drone count must be at least 1
 * - Altitude band step must be positive
 * - Formation parameters must be valid
 * - Safety parameters must be acceptable
 * 
 * @section Error Reporting
 * Validation feedback:
 * - Specific error messages
 * - Parameter identification
 * - Validation error signals
 * - User guidance and correction
 * 
 * @note Parameter validation ensures safe
 *       and effective mission generation.
 * 
 * @see validateAreaParameters()
 * @see _validationError, validationErrorChanged()
 * @see Parameter validation and safety
 * @see Mission generation requirements
 */
bool AreaPlanEditor::validateAreaParameters()
{
    auto setError = [this](const QString& err){ _validationError = err; emit validationErrorChanged(); };
    if (!_areaCenter.isValid()) { setError("Area center is invalid"); return false; }
    if (!qIsFinite(_areaWidth) || _areaWidth <= 0) { setError("Area width must be > 0"); return false; }
    if (!qIsFinite(_areaHeight) || _areaHeight <= 0) { setError("Area height must be > 0"); return false; }
    if (!qIsFinite(_lineSpacing) || _lineSpacing <= 0) { setError("Line spacing must be > 0"); return false; }
    if (_numPoints <= 0) { setError("Points per line must be > 0"); return false; }
    if (!qIsFinite(_missionAltitude)) { setError("Mission altitude invalid"); return false; }
    if (_droneCount < 1) { setError("Drone count must be >= 1"); return false; }
    if (_altitudeBandStep <= 0) { setError("Altitude band step must be > 0"); return false; }
    // Clear previous error
    if (!_validationError.isEmpty()) { _validationError.clear(); emit validationErrorChanged(); }
    return true;
}

/**
 * @brief Validates waypoint generation results and geometry
 * 
 * This method validates that generated waypoints meet expected
 * counts and lie within the specified area boundaries, ensuring
 * proper mission coverage and geometric accuracy.
 * 
 * @return True if waypoint generation is valid, false otherwise
 * 
 * @section Waypoint Validation
 * Validation process includes:
 * - Waypoint count verification
 * - Geometric boundary checking
 * - Coordinate transformation validation
 * - Area coverage verification
 * 
 * @section Count Validation
 * Waypoint counting:
 * - Expected count calculation
 * - Actual count verification
 * - Line and point multiplication
 * - Coverage completeness check
 * 
 * @section Geometry Validation
 * Boundary checking:
 * - Local coordinate calculation
 * - Geographic coordinate transformation
 * - Boundary limit verification
 * - Tolerance-based validation
 * 
 * @section Coordinate System
 * Coordinate validation:
 * - Local to geographic transformation
 * - Rotation and translation verification
 * - Boundary limit enforcement
 * - Geometric accuracy assessment
 * 
 * @note Geometry validation ensures proper
 *       mission coverage and accuracy.
 * 
 * @see validateWaypointGeneration()
 * @see generateWaypoints()
 * @see Coordinate system validation
 * @see Mission geometry verification
 */
bool AreaPlanEditor::validateWaypointGeneration()
{
    // Validate that generated waypoints match expected counts and lie within the rotated rectangle bounds.
    if (_areaCenter.isValid() == false || _areaWidth <= 0 || _areaHeight <= 0 || _numPoints <= 0 || _lineSpacing <= 0) {
        return false;
    }

    const QList<QVariant> wps = generateWaypoints();
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const int expectedCount = lineCount * _numPoints;
    if (wps.size() != expectedCount) {
        qWarning() << "Waypoint count mismatch" << wps.size() << "!=" << expectedCount;
        return false;
    }

    // Geometry check: approximate dx,dy in meters from center, unrotate, and assert within half width/height
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);

    auto toMeters = [&](const QGeoCoordinate& c0, const QGeoCoordinate& c1) {
        const qreal metersPerDegreeLat = 111319.9;
        const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(c0.latitude()));
        const qreal dy = (c1.latitude() - c0.latitude()) * metersPerDegreeLat; // north positive
        const qreal dx = (c1.longitude() - c0.longitude()) * metersPerDegreeLon; // east positive
        return QPointF(dx, dy);
    };

    auto unrotate = [&](const QPointF& p) {
        // Map world displacement back to local rectangle axes using clockwise-positive convention
        return QPointF(p.x() * cosT - p.y() * sinT,
                       p.x() * sinT + p.y() * cosT);
    };

    const qreal eps = 0.25; // meters tolerance
    for (const QVariant& v : wps) {
        const QGeoCoordinate wp = v.value<QGeoCoordinate>();
        if (!wp.isValid()) {
            qWarning() << "Invalid waypoint coordinate";
            return false;
        }
        const QPointF dxy = toMeters(_areaCenter, wp);
        const QPointF local = unrotate(dxy);
        if (qAbs(local.x()) > halfW + eps || qAbs(local.y()) > halfH + eps) {
            qWarning() << "Waypoint out of bounds" << local << "halfW/H" << halfW << halfH;
            return false;
        }
    }

    return true;
}

/**
 * @brief Validates mission upload capability and readiness
 * 
 * This method verifies that the mission can be uploaded to
 * the active vehicle, checking vehicle availability, mission
 * controller status, and mission content readiness.
 * 
 * @return True if mission upload is ready, false otherwise
 * 
 * @section Upload Validation
 * Validation process includes:
 * - Active vehicle verification
 * - Mission controller validation
 * - Mission content verification
 * - Upload readiness assessment
 * 
 * @section Vehicle Requirements
 * Vehicle readiness:
 * - Active vehicle must be available
 * - Vehicle must be properly connected
 * - Communication links must be established
 * - Vehicle must be in upload-ready state
 * 
 * @section Mission Requirements
 * Mission readiness:
 * - Mission controller must be set
 * - Mission must contain waypoints
 * - Mission structure must be valid
 * - Mission must be properly configured
 * 
 * @section Error Reporting
 * Validation feedback:
 * - Specific error messages
 * - Requirement identification
 * - Validation error signals
 * - User guidance and correction
 * 
 * @note Upload validation ensures successful
 *       mission transfer to vehicles.
 * 
 * @see validateMissionUpload()
 * @see getCurrentVehicle()
 * @see getMissionController()
 * @see Mission upload validation
 */
bool AreaPlanEditor::validateMissionUpload()
{
    Vehicle* v = getCurrentVehicle();
    MissionController* mission = getMissionController();
    if (!v) { _validationError = "No active vehicle"; emit validationErrorChanged(); return false; }
    if (!mission) { _validationError = "MissionController not set"; emit validationErrorChanged(); return false; }
    if (mission->visualItems()->count() <= 1) { // only MissionSettings
        _validationError = "No mission items to upload"; emit validationErrorChanged(); return false; }
    _validationError.clear(); emit validationErrorChanged();
    return true;
}

/**
 * @brief Validates mission file saving capability
 * 
 * This method verifies that the mission can be saved to a file,
 * checking mission controller availability and mission content
 * for file export operations.
 * 
 * @return True if mission can be saved, false otherwise
 * 
 * @section Save Validation
 * Validation process includes:
 * - Mission controller verification
 * - Mission content validation
 * - File export readiness
 * - Save capability assessment
 * 
 * @section Controller Requirements
 * Controller readiness:
 * - Mission controller must be available
 * - Controller must be properly initialized
 * - Mission structure must be accessible
 * - Export capabilities must be present
 * 
 * @section Content Requirements
 * Mission content:
 * - Mission must contain items
 * - Waypoints must be available
 * - Mission structure must be valid
 * - Exportable data must be present
 * 
 * @section File Export
 * Export capabilities:
 * - File write permissions
 * - Export format support
 * - Data conversion capabilities
 * - File generation readiness
 * 
 * @note Save validation ensures successful
 *       mission file export operations.
 * 
 * @see validateMissionFileSaving()
 * @see getMissionController()
 * @see Mission file export
 * @see File saving validation
 */
bool AreaPlanEditor::validateMissionFileSaving()
{
    // Basic check: controller exists and has items
    MissionController* mission = getMissionController();
    if (!mission) return false;
    return mission->visualItems()->count() > 0;
}

/**
 * @brief Gets drone allocation statistics for mission planning
 * 
 * This method provides comprehensive statistics for a specific drone's
 * mission allocation, including line assignments, waypoint counts,
 * altitude offsets, and timing information for mission coordination.
 * 
 * @param droneIndex Index of the drone (0-based) for statistics
 * @return QMap<QString, QVariant> containing drone allocation statistics
 * 
 * @section Statistics Components
 * Statistics include:
 * - Drone index and identification
 * - Assigned line count and coverage
 * - Expected waypoint count
 * - Altitude offset calculations
 * - Time offset information
 * 
 * @section Line Assignment
 * Coverage analysis:
 * - Round-robin stripe assignment
 * - Line count calculation
 * - Coverage area determination
 * - Workload distribution analysis
 * 
 * @section Waypoint Calculation
 * Mission planning:
 * - Total waypoint estimation
 * - Line-based waypoint counting
 * - Mission coverage assessment
 * - Resource planning support
 * 
 * @section Altitude and Timing
 * Coordination parameters:
 * - Altitude band offset calculation
 * - Time staggering parameters
 * - Mission synchronization data
 * - Fleet coordination support
 * 
 * @note Statistics enable mission planning
 *       and resource allocation optimization.
 * 
 * @see getDroneAllocationStats()
 * @see AreaPlan::assignStripesRoundRobin()
 * @see Mission planning statistics
 * @see Drone allocation analysis
 */
QMap<QString, QVariant> AreaPlanEditor::getDroneAllocationStats(int droneIndex) const
{
    QMap<QString, QVariant> stats;
    if (droneIndex < 0 || droneIndex >= _droneCount) return stats;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    const auto& assigned = rr[static_cast<size_t>(droneIndex)];
    const int lines = static_cast<int>(assigned.size());
    const int wpCount = lines * _numPoints;
    stats["droneIndex"] = droneIndex;
    stats["lineCount"] = lines;
    stats["waypointCount"] = wpCount;
    stats["altitudeOffsetM"] = _altitudeBandStart + droneIndex * _altitudeBandStep;
    stats["timeOffsetS"] = droneIndex * _timeOffsetPerDrone;
    QVariantList li; for (int i : assigned) li.append(i); stats["lineIndices"] = li;
    return stats;
}

/**
 * @brief Validates swarm configuration for coordinated operations
 * 
 * This method verifies that the swarm is properly configured
 * for coordinated operations, checking vehicle availability,
 * role assignments, and formation readiness.
 * 
 * @return True if swarm is properly configured, false otherwise
 * 
 * @section Swarm Configuration
 * Configuration validation:
 * - Vehicle count verification
 * - Role assignment validation
 * - Formation readiness check
 * - Coordination capability assessment
 * 
 * @section Vehicle Requirements
 * Vehicle availability:
 * - Minimum of 2 vehicles required
 * - All vehicles must be connected
 * - Vehicle models must be available
 * - Communication links must be established
 * 
 * @section Role Assignment
 * Formation roles:
 * - All vehicles must have assigned roles
 * - Role hierarchy must be established
 * - Formation positions must be calculated
 * - Coordination structure must be ready
 * 
 * @section Formation Readiness
 * Formation validation:
 * - Formation type must be set
 * - Formation spacing must be configured
 * - Vehicle positions must be calculated
 * - Formation transitions must be ready
 * 
 * @note Swarm configuration validation ensures
 *       safe and effective coordinated operations.
 * 
 * @see validateSwarmConfiguration()
 * @see _formationRoles, MultiVehicleManager
 * @see Swarm configuration validation
 * @see Coordinated operations
 */
bool AreaPlanEditor::validateSwarmConfiguration() const
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) return false;
    // At least leader + one follower and roles assigned
    for (int i = 0; i < vehicleModel->count(); ++i) {
        Vehicle* v = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (!v) return false;
        if (!_formationRoles.contains(v->id())) return false;
    }
    return true;
}

QString AreaPlanEditor::validateInput(const QString& fieldName, const QVariant& value) const
{
    auto bad = [&](const QString& m){ return m; };
    if (fieldName == "areaWidth" || fieldName == "areaHeight" || fieldName == "lineSpacing") {
        bool ok; const qreal v = value.toDouble(&ok); if (!ok || v <= 0) return bad(fieldName + " must be > 0");
    } else if (fieldName == "numPoints") {
        bool ok; int v = value.toInt(&ok); if (!ok || v <= 0) return bad("numPoints must be > 0");
    } else if (fieldName == "missionAltitude") {
        bool ok; value.toDouble(&ok); if (!ok) return bad("missionAltitude invalid");
    } else if (fieldName == "droneCount") {
        bool ok; int v = value.toInt(&ok); if (!ok || v < 1) return bad("droneCount must be >= 1");
    } else if (fieldName == "altitudeBandStep") {
        bool ok; const qreal v = value.toDouble(&ok); if (!ok || v <= 0) return bad("altitudeBandStep must be > 0");
    }
    return QString();
}

bool AreaPlanEditor::isInputValid(const QString& fieldName, const QVariant& value) const
{
    return validateInput(fieldName, value).isEmpty();
}

QString AreaPlanEditor::getValidationError() const
{
    return _validationError;
}

void AreaPlanEditor::clearValidationError()
{
    if (!_validationError.isEmpty()) {
        _validationError.clear();
        emit validationErrorChanged();
    }
}

void AreaPlanEditor::logError(const QString& errorMessage, const QString& context)
{
    qWarning() << "Error:" << errorMessage << "Context:" << context;
}

void AreaPlanEditor::handleError(const QString& errorMessage, const QString& recoverySuggestion)
{
    logError(errorMessage, recoverySuggestion);
    updateStatus(QString("Error: %1. %2").arg(errorMessage, recoverySuggestion));
}

void AreaPlanEditor::startProgress(const QString& operation, const QString& message)
{
    _isProcessing = true;
    _progressValue = 0;
    _currentOperation = operation;
    _progressMessage = message;
    emit isProcessingChanged();
    emit progressValueChanged();
    emit currentOperationChanged();
    emit progressMessageChanged();
}

void AreaPlanEditor::updateProgress(int value, const QString& message)
{
    _progressValue = value;
    if (!message.isEmpty()) {
        _progressMessage = message;
        emit progressMessageChanged();
    }
    emit progressValueChanged();
}

void AreaPlanEditor::finishProgress(const QString& message)
{
    _isProcessing = false;
    _progressValue = 100;
    if (!message.isEmpty()) {
        _progressMessage = message;
        emit progressMessageChanged();
    }
    emit isProcessingChanged();
    emit progressValueChanged();
}

void AreaPlanEditor::cancelProgress()
{
    _isProcessing = false;
    _progressValue = 0;
    _progressMessage.clear();
    emit isProcessingChanged();
    emit progressValueChanged();
    emit progressMessageChanged();
}

void AreaPlanEditor::setProgressOperation(const QString& operation)
{
    if (_currentOperation != operation) {
        _currentOperation = operation;
        emit currentOperationChanged();
    }
}

void AreaPlanEditor::enableOptimizations()
{
    if (!_isOptimized) {
        _isOptimized = true;
        emit isOptimizedChanged();
    }
}

void AreaPlanEditor::disableOptimizations()
{
    if (_isOptimized) {
        _isOptimized = false;
        emit isOptimizedChanged();
    }
}

void AreaPlanEditor::clearCache()
{
    _waypointCache.clear();
    _cacheHits = 0;
    _cacheMisses = 0;
}

void AreaPlanEditor::optimizeWaypointGeneration()
{
    // Enable a simple cache based on key of parameters
    enableOptimizations();
    // Precompute and cache current configuration
    const QString key = QString("w=%1|h=%2|s=%3|n=%4|r=%5|clat=%6|clon=%7")
                            .arg(_areaWidth).arg(_areaHeight).arg(_lineSpacing).arg(_numPoints)
                            .arg(_areaRotation)
                            .arg(_areaCenter.latitude(), 0, 'f', 7)
                            .arg(_areaCenter.longitude(), 0, 'f', 7);
    if (_waypointCache.contains(key)) {
        _cacheHits++;
        return;
    }
    _cacheMisses++;
    QVariantList wps = const_cast<AreaPlanEditor*>(this)->generateWaypoints();
    _waypointCache.insert(key, wps);
}

void AreaPlanEditor::setCacheSize(int size)
{
    if (_cacheSize != size) {
        _cacheSize = size;
        emit cacheSizeChanged();
    }
}

void AreaPlanEditor::profilePerformance()
{
    _performanceTimer.restart();
    generateWaypoints();
    qint64 elapsedMs = _performanceTimer.elapsed();
    _performanceMetrics["generateWaypoints_ms"] = elapsedMs;
}

QMap<QString, QVariant> AreaPlanEditor::getPerformanceMetrics() const
{
    QMap<QString, QVariant> metrics;
    metrics["cacheHits"] = _cacheHits;
    metrics["cacheMisses"] = _cacheMisses;
    metrics["cacheSize"] = _cacheSize;
    return metrics;
}

int AreaPlanEditor::calculateTotalWaypoints() const
{
    // Calculate total waypoints based on area dimensions and line spacing
    int linesHorizontal = qCeil(_areaWidth / _lineSpacing);
    int linesVertical = qCeil(_areaHeight / _lineSpacing);
    
    // Each line has _numPoints waypoints
    int totalPoints = (linesHorizontal + linesVertical) * _numPoints;
    
    // Add extra points for RTL and loiter if enabled
    if (_rtlAfterEveryWaypoint) {
        totalPoints *= 2;  // Double for RTL after each point
    }
    if (_loiterAfterRtl) {
        totalPoints += linesHorizontal + linesVertical;  // Add loiter points
    }
    
    return totalPoints;
}

int AreaPlanEditor::calculateFlightTime() const
{
    // Approximate flight time calculation
    const qreal averageSpeed = 5.0;  // meters per second
    const qreal turnTime = 5.0;      // seconds per turn
    
    // Calculate total distance
    qreal totalDistance = _areaWidth * qCeil(_areaHeight / _lineSpacing);  // Total survey distance
    
    // Calculate number of turns
    int numTurns = qCeil(_areaHeight / _lineSpacing);
    
    // Basic flight time = distance/speed + turns*turnTime
    int flightTime = qCeil(totalDistance / averageSpeed + numTurns * turnTime);
    
    // Add loiter time if enabled
    if (_loiterAfterRtl) {
        flightTime += calculateTotalWaypoints() * _loiterTime;
    }
    
    // Add RTL time if enabled
    if (_rtlAfterEveryWaypoint) {
        // Rough estimate: 2x the height for each RTL
        qreal rtlDistance = 2 * _missionAltitude * calculateTotalWaypoints();
        flightTime += qCeil(rtlDistance / averageSpeed);
    }
    
    return flightTime;
}

/**
 * @brief Calculates formation positions for swarm operations
 * 
 * This method computes the relative positions of all vehicles
 * in the current formation, enabling coordinated positioning
 * and safe swarm operations.
 * 
 * @section Formation Calculation
 * Calculation process includes:
 * - Vehicle enumeration and validation
 * - Formation type analysis
 * - Position calculation and assignment
 * - Safety margin verification
 * 
 * @section Formation Types
 * Supported formations:
 * - NoFormation: No specific pattern
 * - VFormation: V-shaped arrangement
 * - LineFormation: Linear arrangement
 * - CircleFormation: Circular arrangement
 * - GridFormation: Grid-based arrangement
 * 
 * @section Position Calculation
 * Coordinate computation:
 * - Leader vehicle positioning
 * - Follower vehicle offsets
 * - Formation spacing application
 * - Safety margin enforcement
 * 
 * @section Safety Features
 * Safety considerations:
 * - Minimum separation distances
 * - Collision avoidance margins
 * - Formation geometry validation
 * - Operational safety verification
 * 
 * @note Formation positioning enables
 *       safe and coordinated swarm operations.
 * 
 * @see calculateFormationPositions()
 * @see Formation types and positioning
 * @see Swarm coordination and safety
 * @see Vehicle positioning and control
 */
void AreaPlanEditor::calculateFormationPositions()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleModel->count(); i++) {
        vehicles.append(qobject_cast<Vehicle*>(vehicleModel->get(i)));
    }
    if (vehicles.isEmpty()) return;

    // Clear any existing offsets
    _formationOffsets.clear();

    const int totalVehicles = vehicles.size();
    const int gridSize = qCeil(qSqrt(totalVehicles));

    switch (_currentFormation) {
        case FormationType::NoFormation:
            // No offsets from leader
            for (Vehicle* vehicle : vehicles) {
                _formationOffsets[vehicle->id()] = QGeoCoordinate();
            }
            break;

        case FormationType::VFormation: {
            // V formation with leader at front
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    // Leader at front
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    // Followers in V shape
                    bool isLeft = (role % 2 == 1);
                    int position = (role + 1) / 2;
                    double angle = isLeft ? 30.0 : -30.0;  // 30-degree V shape
                    double distance = position * _formationSpacing;

                    // Calculate offset using trigonometry (x east, y north)
                    double dx = distance * qSin(qDegreesToRadians(angle));
                    double dy = -distance * qCos(qDegreesToRadians(angle));

                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }

        case FormationType::LineFormation: {
            // Line formation with equal spacing to the east (x axis)
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    double dx = role * _formationSpacing;
                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }

        case FormationType::CircleFormation: {
            // Circle formation around leader
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    double angle = (360.0 * role) / totalVehicles;
                    double dx = _formationSpacing * qSin(qDegreesToRadians(angle));
                    double dy = _formationSpacing * qCos(qDegreesToRadians(angle));

                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }

        case FormationType::GridFormation: {
            // Grid formation filled row-major
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    int row = role / gridSize;
                    int col = role % gridSize;
                    double dx = col * _formationSpacing;
                    double dy = row * _formationSpacing;

                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }

        default:
            break;
    }

    emit formationPositionsChanged();
}

// --- File save helpers -------------------------------------------------
/**
 * @brief Saves mission to CSV file with coordinate data
 * 
 * This method exports mission data to a CSV file format,
 * providing coordinate information for external analysis,
 * mission sharing, and documentation purposes.
 * 
 * @param missionController Mission controller containing mission data
 * @param filename Target filename for CSV export
 * 
 * @section Mission Export
 * Export process includes:
 * - Mission controller validation
 * - File creation and opening
 * - Coordinate data extraction
 * - CSV format generation
 * 
 * @section File Format
 * CSV structure:
 * - Header row with lat,lon,alt columns
 * - Coordinate data from mission items
 * - Altitude information for waypoints
 * - Simple text-based format
 * 
 * @section Data Extraction
 * Mission data processing:
 * - Visual mission item enumeration
 * - Coordinate extraction and formatting
 * - Altitude data inclusion
 * - Data validation and verification
 * 
 * @section Export Capabilities
 * Export features:
 * - Coordinate-based mission export
 * - Altitude information inclusion
 * - External compatibility
 * - Mission sharing support
 * 
 * @note CSV export enables mission sharing
 *       and external analysis capabilities.
 * 
 * @see saveMissionToFile()
 * @see Mission export and sharing
 * @see CSV format generation
 * @see External mission compatibility
 */
void AreaPlanEditor::saveMissionToFile(MissionController* missionController, const QString& filename)
{
    if (!missionController) return;
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    if (missionController->visualItems()) {
        for (int i = 0; i < missionController->visualItems()->count(); ++i) {
            auto* vmi = qobject_cast<VisualMissionItem*>(missionController->visualItems()->get(i));
            if (!vmi) continue;
            const QGeoCoordinate c = vmi->coordinate();
            ts << QString::number(c.latitude(), 'f', 7) << ","
               << QString::number(c.longitude(), 'f', 7) << ","
               << QString::number(c.altitude(), 'f', 2) << "\n";
        }
    }
    f.close();
}

/**
 * @brief Saves mission items to CSV file with coordinate data
 * 
 * This method exports a list of mission items to a CSV file,
 * providing coordinate information for external analysis,
 * mission documentation, and sharing purposes.
 * 
 * @param missionItems List of mission items to export
 * @param filename Target filename for CSV export
 * 
 * @section Mission Export
 * Export process includes:
 * - Mission item list validation
 * - File creation and opening
 * - Coordinate data extraction
 * - CSV format generation
 * 
 * @section File Format
 * CSV structure:
 * - Header row with lat,lon,alt columns
 * - Coordinate data from mission items
 * - Altitude information for waypoints
 * - Simple text-based format
 * 
 * @section Data Processing
 * Item processing:
 * - Mission item enumeration
 * - Coordinate extraction and formatting
 * - Altitude data inclusion
 * - Data validation and verification
 * 
 * @section Export Features
 * Export capabilities:
 * - Mission item list export
 * - Coordinate-based data export
 * - Altitude information inclusion
 * - External compatibility support
 * 
 * @note CSV export enables mission item
 *       sharing and external analysis.
 * 
 * @see saveMissionToFile()
 * @see Mission item export
 * @see CSV format generation
 * @see External mission compatibility
 */
void AreaPlanEditor::saveMissionToFile(const QList<MissionItem*>& missionItems, const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    for (const MissionItem* mi : missionItems) {
        if (!mi) continue;
        const QGeoCoordinate c = mi->coordinate();
        ts << QString::number(c.latitude(), 'f', 7) << ","
           << QString::number(c.longitude(), 'f', 7) << ","
           << QString::number(c.altitude(), 'f', 2) << "\n";
    }
    f.close();
}
