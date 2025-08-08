# Validate Altitude Bands

- Preconditions
  - Missions uploaded to 2–4 vehicles
  - Area Planner altitudeBandStart/altitudeBandStep set

- Steps
  - Inspect Mission view per vehicle: confirm waypoint altitudes = base + (droneIndex * step)
  - Ensure no overlapping altitudes between adjacent drones
  - Optional: simulate takeoff and verify commanded altitudes hold

- Expected
  - Clear altitude separation equal to `altitudeBandStep`
  - No duplicated target altitudes across drones

- Notes
  - Firmware may report relative/absolute; compare consistently
  - If overlap detected, increase `altitudeBandStep` and regenerate
