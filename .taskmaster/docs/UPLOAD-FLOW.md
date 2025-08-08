# Area Planner Upload Flow

- UI: `src/QmlControls/AreaPlanEditor.qml`
  - Vehicle selector (`QGCComboBox`) bound to `QGroundControl.multiVehicleManager.vehicles`
  - Per-drone mapping table (drone → vehicle) and "Upload All Mapped"
  - Per-drone upload: calls `areaPlanEditor.uploadPerDroneMissionToVehicle(idx, vehicle)`

- Backend: `src/QmlControls/AreaPlanEditor.cc/.h`
  - `uploadToVehicle()` builds aggregated mission and uploads via `MissionManager::writeMissionItems`
  - `uploadPerDroneMissionToVehicle(int, QObject*)` builds single-drone mission and uploads to selected `Vehicle`
  - Progress & errors: connects to `PlanManager::progressPctChanged`, `error(int, QString)`, `sendComplete(bool)` to update status

- Signals used
  - `PlanManager::progressPctChanged(double)` → updates progress text
  - `PlanManager::error(int, QString)` → cancels progress and shows error
  - `PlanManager::sendComplete(bool error)` → final success/failure

- Troubleshooting
  - "No vehicle connected": select an active vehicle or map per-drone vehicles
  - Upload appears stuck: ensure link is active (see toolbar link indicator), retry upload
  - ArduPilot home handling: first WP may be skipped per firmware; QGC PlanManager adjusts sequence
  - Disconnected during upload: `error`/`sendComplete(true)` fires; reconnect and retry

- Manual testing (SITL)
  - Connect two SITL vehicles, verify vehicle list populates
  - Map drones to different vehicles and use "Upload All Mapped"
  - Observe per-drone progress and completion; verify missions on each vehicle
