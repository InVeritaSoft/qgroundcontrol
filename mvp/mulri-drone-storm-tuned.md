# Multi-Drone Mission Planning Overview

To enable missions involving multiple drones, the system must support assigning each drone its own set of waypoints, ensuring all target points are covered. Collision avoidance is essential—this may be achieved by assigning different altitudes or implementing other deconfliction strategies.

## Area Plan Adjustments

**Mission Structure for Each Drone:**
- Each drone initiates its own takeoff sequence.
- The geometric shapes, lines, and points used for planning remain unchanged.
    - However, after reaching each waypoint, the drone should perform an RTL (Return to Launch) and then resume its assigned waypoints.
- At each waypoint, the drone should execute a Loiter or Hold action.
    - Confirm that this action is configurable in the input section and properly synchronized with the mission item.
- After every RTL, the drone should immediately perform a Loiter or Hold.
    - This should also be configurable and synchronized with the mission item.

## Implementation Implications

- Mission generation can no longer rely solely on the C++ backend in the Missions Tab.
- An algorithm is required to divide the planned area (drawings) among the available drones.
- The mission plan for each drone (waypoints and RTLs) must be visualized on the map.
- Each drone's mission may use different altitudes or other parameters to prevent in-flight conflicts.

## Per-Drone Mission Sequence

1. Takeoff
2. Waypoint
3. Loiter or Hold
4. RTL
5. Loiter or Hold
6. Repeat steps 2–5 as needed
7. Final RTL (End of Mission)