# PRD Validation: Multi-Drone Area Planner

Source: `.taskmaster/docs/Multi-Drone-Area-Planner-PRD.md`

- Core Features
  - Multi-drone area planning: Implemented (properties in AreaPlanEditor; UI in AreaPlanEditor.qml)
  - Per-drone mission generation: Implemented (stripe split + per-drone waypoints)
  - Policy injection (RTL after WP, Loiter after RTL): Implemented (toggles present)
  - Policy: Loiter/Hold at each waypoint (configurable): Partial (loiterTime exists; missing explicit "loiter at each waypoint" toggle/injection)
  - Deconfliction v1 (Altitude banding, Time staggering): Implemented (both parameters available); Missing explicit deconfliction-mode UI toggle (Alt bands | Time staggering)

- Visualization
  - Per-drone overlays (grid, waypoint markers): Implemented (AreaPlanMapVisuals.qml)
  - Center markers: Missing (not currently rendered)
  - Legend + quick visibility toggles: Implemented

- Operator Controls
  - Inputs (spacing, points, altitude, loiter time, RTL/Loiter-After-RTL): Implemented (qsTr covered)
  - Generate, preview, save per-drone missions, upload: Implemented

- Compatibility
  - Offline planning without vehicles: Implemented
  - QGC Plan View integration and upload workflow: Implemented (vehicle mapping + per-drone/mapped upload)

- Roadmap (Phase 2/3)
  - Time-based deconfliction preview (Gantt): Not in MVP (N/A now)
  - Advanced partitioning/non-rectangular footprints: N/A now
  - Advanced deconfliction/corridors, battery-aware, hetero fleets, live progress: N/A now

- Acceptance Criteria (MVP)
  - N missions/files with correct per-drone waypoints: Implemented
  - Altitude bands + policy options applied: Implemented for RTL/Loiter-after-RTL; Partial for "Loiter at each waypoint"
  - Distinct per-drone overlays + legend: Implemented
  - Save per-drone + upload per matching vehicle: Implemented

- Gaps & Follow-ups
  1) Add deconfliction mode UI toggle (Altitude bands | Time staggering) and apply mode to preview/generation
  2) Implement "Loiter at each waypoint" policy toggle and injection using `loiterTime`
  3) Add center marker overlay to AreaPlanMapVisuals (bind to area center)

- Test/Docs
  - Unit tests cover partitioning, waypoint gen, policy sequencing, and preview counts
  - SITL E2E docs/checklists added (upload, validation, logs, screenshots)
