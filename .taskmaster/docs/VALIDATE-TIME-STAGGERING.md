# Validate Time Staggering

- Preconditions
  - `timeOffsetPerDrone` > 0 in Area Planner
  - Missions uploaded per vehicle

- Steps
  - Confirm a pre-start loiter (or delay) is present for drones with index > 0
  - Observe launch/order: Drone 0 first, Drone 1 after offset, etc.
  - Check mission timeline: WP arrival times are offset by configured seconds

- Expected
  - Takeoffs and early mission items occur with configured separations
  - `sendComplete(false)` without errors during upload

- Notes
  - If firmware ignores loiter-before-start, consider a takeoff delay alternative
  - Increase offset if overlap persists
