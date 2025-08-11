# Upload Testing (Connected Vehicles)

- Prerequisites
  - Launch 2 SITL instances; connect in QGC (verify toolbar vehicle list)
  - Ensure each vehicle has a stable link (no high-latency preferred)

- Steps
  - Open Plan view → Area Planner panel
  - Set area params; click Refresh Preview to populate per-drone list
  - Map Drone 0 → Vehicle 1; Drone 1 → Vehicle 2
  - Click "Upload All Mapped"

- Expected
  - Progress text updates from 0–100% for each upload
  - No error signal; `sendComplete(false)` fires
  - Each vehicle shows the uploaded mission in the Mission view

- Notes
  - If upload appears stuck, verify links and retry
  - Firmware may skip home item; PlanManager adjusts sequence accordingly
