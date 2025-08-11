# Upload Testing (Disconnected Vehicles)

- Prerequisites
  - Launch 1–2 SITL instances and connect in QGC
  - Identify link controls (to disconnect a vehicle/link)

- Steps
  - Start an upload (per-drone or aggregated)
  - While uploading, disconnect the link of the target vehicle
  - Observe UI: error signal should fire; progress stops

- Expected
  - `PlanManager::error` or `sendComplete(true)` received
  - Status label shows a failure message
  - After reconnecting, reattempt upload succeeds

- Notes
  - High-latency links may slow progress; prefer normal links for this test
  - If errors persist post reconnect, verify active vehicle selection and retry
