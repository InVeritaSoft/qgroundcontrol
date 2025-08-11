# SITL Connections (2–4 Vehicles)

- Start SITL
  - Launch 2–4 instances with unique sysids and MAVLink ports
  - Route each to QGC (e.g., UDP 14550, 14551, 14552, 14553)

- QGC Detection
  - Open QGC and wait for vehicles to appear in toolbar
  - Verify link indicators and names/IDs are unique

- Health Check
  - Ensure parameters load without errors
  - Confirm GPS/home set (as applicable)

- Notes
  - If not detected, verify firewall rules and UDP routing
  - Use stable links for upload validation; avoid high-latency profiles
