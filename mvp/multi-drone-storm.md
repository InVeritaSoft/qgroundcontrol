Need to have an ability to send more than one drone on a mission/s. So that they cover all points, each will have its own set of waypoints to cover. Obvious to say that they have to avoid each other in fly, it could be a different height algo, or something else. 

Shortly about the Area Plan changes, 

Mission is 
- takeoff, obviously - each drone should have its own takeoff
- the shape and lines and points, their drawings remain unchanged
-- except that after each waypoint each drone should do RTL and resume covering its points (waypoints)
- Loiter or Hold on the point (validate whether it is configurable where the input section is and synced into mission item)
- Loiter or Hold immidietly after RTL each time (also should be configurable and synced into mission item)

What above means that we no more just create missions via C++ Backend on Missions Tab, we need to build an algo of splitting our drawings between drones, and show it on the map, each drone = its own set of waypoints and RTLs, different height or anything else that could help avoid collisions

For One Drone (each)
1) takeoff
2) waypoint
3) loiter or hold
4) RTL
5) loiter or hold
6) 2,3,4,5
7) Final RTL (END)