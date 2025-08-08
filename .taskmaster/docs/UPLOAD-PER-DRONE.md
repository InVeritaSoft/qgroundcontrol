# Upload Per-Drone Missions

- Options
  - Single: Use "Upload Drone # to Active Vehicle" (Area Planner)
  - Mapped: Use "Upload All Mapped" after assigning vehicles

- Observe
  - Progress updates (PlanManager::progressPctChanged)
  - Errors reported (PlanManager::error / sendComplete(true))
  - Success (sendComplete(false)) and Mission view reflects uploaded items

- Verification
  - Each vehicle shows the expected items/altitudes
  - Sequence and policy items (RTL/Loiter) present if toggled

- Troubleshooting
  - Ensure vehicle is connected/active
  - Retry if link hiccups; verify UDP routing and toolbar indicators
