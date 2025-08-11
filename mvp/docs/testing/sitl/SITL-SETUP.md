# SITL Setup (Multi-Vehicle)

- Prerequisites
  - Install ArduPilot/PX4 SITL (per project need)
  - Ensure QGC networking permits local UDP/TCP connections

- Launch
  - Start 2–4 SITL instances with distinct sysids and ports
  - Verify each instance broadcasts to QGC (e.g., UDP 14550/14551/…)

- QGC
  - Open QGC and confirm each vehicle appears in the toolbar vehicle list
  - Check link indicators show active/stable connection

- Notes
  - Use unique home positions if testing altitude/spacing visuals
  - Prefer normal latency links for upload progress validation
