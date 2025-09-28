# QGroundControl Waypoint Creation Guide

This document provides comprehensive information about creating waypoints in QGroundControl, including different types of waypoints, their parameters, and implementation examples.

## Table of Contents

1. [Overview](#overview)
2. [Basic Waypoint Types](#basic-waypoint-types)
3. [MAVLink Commands](#mavlink-commands)
4. [Coordinate Frames](#coordinate-frames)
5. [Waypoint Parameters](#waypoint-parameters)
6. [Implementation Examples](#implementation-examples)
7. [Best Practices](#best-practices)

## Overview

Waypoints in QGroundControl are represented by `MissionItem` objects that contain MAVLink commands with specific parameters. Each waypoint has:

- **Sequence Number**: Position in the mission
- **Command**: MAVLink command type (e.g., `MAV_CMD_NAV_WAYPOINT`)
- **Frame**: Coordinate frame (e.g., `MAV_FRAME_GLOBAL_RELATIVE_ALT`)
- **Parameters**: 7 parameters (param1-param7) with specific meanings
- **Auto Continue**: Whether to proceed to next waypoint automatically
- **Current Item**: Whether this is the currently active waypoint

## Basic Waypoint Types

### 1. Navigation Waypoints

#### MAV_CMD_NAV_WAYPOINT
Basic waypoint for navigation.

**Parameters:**
- `param1`: Hold time in seconds
- `param2`: Acceptance radius in meters
- `param3`: Pass radius in meters (0 = pass through)
- `param4`: Yaw/heading in degrees
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

**Example:**
```cpp
MissionItem* waypoint = new MissionItem(
    sequenceNumber,
    MAV_CMD_NAV_WAYPOINT,
    MAV_FRAME_GLOBAL_RELATIVE_ALT,
    10.0,    // Hold for 10 seconds
    5.0,     // 5 meter acceptance radius
    0.0,     // Pass through (no pass radius)
    45.0,    // 45 degree heading
    latitude,
    longitude,
    altitude,
    true,    // Auto continue
    false,   // Not current item
    parent
);
```

#### MAV_CMD_NAV_TAKEOFF
Takeoff waypoint.

**Parameters:**
- `param1`: Pitch angle in degrees
- `param2`: Empty
- `param3`: Empty
- `param4`: Yaw angle in degrees
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

**Example:**
```cpp
MissionItem* takeoff = new MissionItem(
    sequenceNumber,
    MAV_CMD_NAV_TAKEOFF,
    MAV_FRAME_GLOBAL_RELATIVE_ALT,
    0.0,     // No pitch
    0.0,     // Empty
    0.0,     // Empty
    0.0,     // No yaw
    latitude,
    longitude,
    altitude,
    true,
    false,
    parent
);
```

#### MAV_CMD_NAV_LAND
Landing waypoint.

**Parameters:**
- `param1`: Abort altitude in meters
- `param2`: Landing mode
- `param3`: Empty
- `param4`: Yaw angle in degrees
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

**Example:**
```cpp
MissionItem* landing = new MissionItem(
    sequenceNumber,
    MAV_CMD_NAV_LAND,
    MAV_FRAME_GLOBAL_RELATIVE_ALT,
    50.0,    // Abort at 50m altitude
    0.0,     // Default landing mode
    0.0,     // Empty
    0.0,     // No yaw
    latitude,
    longitude,
    altitude,
    true,
    false,
    parent
);
```

#### MAV_CMD_NAV_RETURN_TO_LAUNCH
Return to launch waypoint.

**Parameters:**
- `param1`: Empty
- `param2`: Empty
- `param3`: Empty
- `param4`: Empty
- `param5`: Empty
- `param6`: Empty
- `param7`: Empty

**Example:**
```cpp
MissionItem* rtl = new MissionItem(
    sequenceNumber,
    MAV_CMD_NAV_RETURN_TO_LAUNCH,
    MAV_FRAME_MISSION,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    true,
    false,
    parent
);
```

### 2. Action Commands

#### MAV_CMD_DO_SET_ROI_LOCATION
Set Region of Interest (ROI).

**Parameters:**
- `param1`: Empty
- `param2`: Empty
- `param3`: Empty
- `param4`: Empty
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

**Example:**
```cpp
MissionItem* roi = new MissionItem(
    sequenceNumber,
    MAV_CMD_DO_SET_ROI_LOCATION,
    MAV_FRAME_GLOBAL_RELATIVE_ALT,
    0.0, 0.0, 0.0, 0.0,
    latitude,
    longitude,
    altitude,
    true,
    false,
    parent
);
```

#### MAV_CMD_DO_SET_ROI_NONE
Cancel Region of Interest.

**Parameters:**
- All parameters are empty

**Example:**
```cpp
MissionItem* cancelRoi = new MissionItem(
    sequenceNumber,
    MAV_CMD_DO_SET_ROI_NONE,
    MAV_FRAME_MISSION,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    true,
    false,
    parent
);
```

#### MAV_CMD_DO_CHANGE_SPEED
Change vehicle speed.

**Parameters:**
- `param1`: Speed type (0=air speed, 1=ground speed)
- `param2`: Speed value in m/s
- `param3`: Throttle (-1=no change)
- `param4`: Absolute/relative (0=absolute, 1=relative)
- `param5`: Empty
- `param6`: Empty
- `param7`: Empty

**Example:**
```cpp
MissionItem* speedChange = new MissionItem(
    sequenceNumber,
    MAV_CMD_DO_CHANGE_SPEED,
    MAV_FRAME_MISSION,
    0.0,     // Air speed
    15.0,    // 15 m/s
    -1.0,    // No throttle change
    0.0,     // Absolute speed
    0.0, 0.0, 0.0,
    true,
    false,
    parent
);
```

#### MAV_CMD_DO_MOUNT_CONTROL
Control gimbal/camera mount.

**Parameters:**
- `param1`: Pitch angle in degrees
- `param2`: Roll angle in degrees
- `param3`: Yaw angle in degrees
- `param4`: Altitude in meters
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Mount mode

**Example:**
```cpp
MissionItem* gimbal = new MissionItem(
    sequenceNumber,
    MAV_CMD_DO_MOUNT_CONTROL,
    MAV_FRAME_MISSION,
    -45.0,   // -45 degree pitch (look down)
    0.0,     // No roll
    0.0,     // No yaw
    0.0,     // No altitude
    0.0,     // No latitude
    0.0,     // No longitude
    MAV_MOUNT_MODE_MAVLINK_TARGETING,
    true,
    false,
    parent
);
```

### 3. VTOL Commands

#### MAV_CMD_NAV_VTOL_TAKEOFF
VTOL takeoff waypoint.

**Parameters:**
- `param1`: Pitch angle in degrees
- `param2`: Empty
- `param3`: Empty
- `param4`: Yaw angle in degrees
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

#### MAV_CMD_NAV_VTOL_LAND
VTOL landing waypoint.

**Parameters:**
- `param1`: Abort altitude in meters
- `param2`: Landing mode
- `param3`: Empty
- `param4`: Yaw angle in degrees
- `param5`: Latitude
- `param6`: Longitude
- `param7`: Altitude

## Coordinate Frames

### MAV_FRAME_GLOBAL_RELATIVE_ALT
Global coordinate frame with altitude relative to home position.

**Use for:** Most waypoints, altitude relative to takeoff point.

### MAV_FRAME_GLOBAL_TERRAIN_ALT
Global coordinate frame with altitude relative to terrain.

**Use for:** Terrain-following missions.

### MAV_FRAME_LOCAL_NED
Local coordinate frame (North-East-Down).

**Use for:** Local missions, relative positioning.

### MAV_FRAME_MISSION
Mission coordinate frame.

**Use for:** Commands that don't require coordinates.

## Waypoint Parameters

### Common Parameters

| Parameter | Description | Units | Notes |
|-----------|-------------|-------|-------|
| param1 | Hold time | seconds | Time to wait at waypoint |
| param2 | Acceptance radius | meters | Distance to consider waypoint reached |
| param3 | Pass radius | meters | 0 = pass through, >0 = orbit |
| param4 | Heading/Yaw | degrees | 0-360, NaN = no change |
| param5 | Latitude | degrees | -90 to +90 |
| param6 | Longitude | degrees | -180 to +180 |
| param7 | Altitude | meters | Altitude above reference |

### Parameter Validation

```cpp
// Validate latitude
if (latitude < -90.0 || latitude > 90.0) {
    qWarning() << "Invalid latitude:" << latitude;
    return nullptr;
}

// Validate longitude
if (longitude < -180.0 || longitude > 180.0) {
    qWarning() << "Invalid longitude:" << longitude;
    return nullptr;
}

// Validate altitude
if (altitude < -1000.0 || altitude > 50000.0) {
    qWarning() << "Invalid altitude:" << altitude;
    return nullptr;
}
```

## Implementation Examples

### Using MissionController

```cpp
// Get the mission controller
MissionController* missionController = planMasterController->missionController();

// Create a waypoint
QGeoCoordinate coordinate(37.7749, -122.4194, 100.0);
int nextIndex = missionController->visualItems()->count();
VisualMissionItem* waypoint = missionController->insertSimpleMissionItem(
    coordinate, 
    nextIndex, 
    true  // makeCurrentItem
);

// Set custom parameters
if (waypoint) {
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(waypoint);
    if (simpleItem) {
        simpleItem->missionItem().setParam1(10.0);  // Hold time
        simpleItem->missionItem().setParam2(5.0);   // Acceptance radius
    }
}
```

### Using Direct MissionItem Creation

```cpp
// Create waypoint directly
MissionItem* waypoint = new MissionItem(
    1,                                    // sequence number
    MAV_CMD_NAV_WAYPOINT,                // command
    MAV_FRAME_GLOBAL_RELATIVE_ALT,       // frame
    10.0,                                // param1: hold time
    5.0,                                 // param2: acceptance radius
    0.0,                                 // param3: pass radius
    45.0,                                // param4: heading
    37.7749,                             // param5: latitude
    -122.4194,                           // param6: longitude
    100.0,                               // param7: altitude
    true,                                // auto continue
    false,                               // is current item
    this                                 // parent
);
```

### Creating Mission Sequences

```cpp
// Create a complete mission
QList<MissionItem*> mission;

// Takeoff
mission.append(new MissionItem(1, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT,
    0.0, 0.0, 0.0, 0.0, 37.7749, -122.4194, 50.0, true, false, this));

// Waypoints
mission.append(new MissionItem(2, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
    10.0, 5.0, 0.0, 0.0, 37.7849, -122.4094, 100.0, true, false, this));

mission.append(new MissionItem(3, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
    5.0, 10.0, 0.0, 90.0, 37.7649, -122.4294, 120.0, true, false, this));

// ROI
mission.append(new MissionItem(4, MAV_CMD_DO_SET_ROI_LOCATION, MAV_FRAME_GLOBAL_RELATIVE_ALT,
    0.0, 0.0, 0.0, 0.0, 37.7549, -122.4394, 90.0, true, false, this));

// Landing
mission.append(new MissionItem(5, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
    50.0, 0.0, 0.0, 0.0, 37.7449, -122.4494, 0.0, true, false, this));
```

## Best Practices

### 1. Parameter Validation
Always validate input parameters before creating waypoints:

```cpp
bool validateWaypoint(double latitude, double longitude, double altitude) {
    return (latitude >= -90.0 && latitude <= 90.0) &&
           (longitude >= -180.0 && longitude <= 180.0) &&
           (altitude >= -1000.0 && altitude <= 50000.0);
}
```

### 2. Error Handling
Always check for null pointers and handle errors gracefully:

```cpp
VisualMissionItem* waypoint = missionController->insertSimpleMissionItem(coordinate, index, true);
if (!waypoint) {
    qWarning() << "Failed to create waypoint";
    return;
}
```

### 3. Memory Management
Properly manage memory for MissionItem objects:

```cpp
// Clean up mission items
for (MissionItem* item : mission) {
    delete item;
}
mission.clear();
```

### 4. Coordinate Precision
Use appropriate precision for coordinates:

```cpp
// Good: 6 decimal places for ~0.1m precision
double latitude = 37.774900;
double longitude = -122.419400;

// Avoid: Too many decimal places
double latitude = 37.774900000000001;  // Unnecessary precision
```

### 5. Mission Planning
Plan missions with proper sequencing:

1. Start with takeoff waypoint
2. Add navigation waypoints
3. Include action commands as needed
4. End with landing or RTL waypoint

### 6. Altitude Considerations
Consider altitude modes and terrain:

```cpp
// Use relative altitude for most missions
MAV_FRAME_GLOBAL_RELATIVE_ALT

// Use terrain altitude for terrain following
MAV_FRAME_GLOBAL_TERRAIN_ALT
```

### 7. Testing
Always test waypoints in simulation before real flights:

```cpp
// Set waypoint as current for testing
waypoint->setIsCurrentItem(true);
```

## Common Issues and Solutions

### Issue: Waypoint not reached
**Solution:** Check acceptance radius (param2) and ensure it's appropriate for the vehicle's navigation accuracy.

### Issue: Altitude not respected
**Solution:** Verify coordinate frame and altitude mode settings.

### Issue: Mission not executing
**Solution:** Check sequence numbers and ensure proper mission structure with takeoff and landing waypoints.

### Issue: Gimbal not pointing correctly
**Solution:** Verify gimbal control commands and mount mode parameters.

## Conclusion

Creating waypoints in QGroundControl requires understanding of MAVLink commands, coordinate frames, and parameter meanings. By following the examples and best practices in this guide, you can create robust and reliable missions for autonomous vehicles.

For more information, refer to the QGroundControl source code and MAVLink documentation.