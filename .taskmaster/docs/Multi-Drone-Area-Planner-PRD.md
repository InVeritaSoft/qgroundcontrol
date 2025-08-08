# Multi-Drone Area Planner – Product Requirements Document

## Overview
- QGroundControl enhancement to plan, generate, visualize, and execute coordinated missions for multiple drones over a shared area.
- Solves multi-vehicle coverage: splits a planned area into per-drone waypoint sets, adds per-waypoint RTL and Loiter/Hold as a configurable policy, and ensures basic deconfliction (e.g., altitude bands).
- Target users: field operators and mission planners who need reliable coverage by multiple vehicles with minimal conflict risk.

## Core Features
- Multi-drone area planning
  - Configure number of drones, vehicle IDs, and per-drone parameters.
  - Split a rectangular planned area into N assignments (grid slicing).
- Per-drone mission generation
  - Generate waypoints per assignment with line spacing and rotation.
  - Inject policy steps:
    - Loiter/Hold at each waypoint (configurable).
    - RTL after each waypoint (configurable) and optional Loiter/Hold after RTL.
- Deconfliction strategies (v1)
  - Altitude banding per drone (fixed offsets).
  - Optional start-time staggering (simple temporal separation).
- Visualization
  - Per-drone overlays: grid lines, waypoint markers, center markers.
  - Color-coded by drone; legend and quick filter toggles.
- Operator controls
  - Input fields for line spacing, number of points per line, altitude, loiter time, RTL-after-waypoint policy.
  - Generate, preview, save mission files, and upload.
- Compatibility
  - Works without connected vehicles (offline planning).
  - Compatible with QGC Plan View and mission upload workflow.

## User Experience
- Personas
  - Field Operator: runs prepared missions quickly, checks deconfliction status, uploads to vehicles.
  - Mission Planner: configures area parameters, policies, and validates plan coverage.
- Key flows
  - Area tab → define area center/size/rotation → set drones count and deconfliction policy → set line spacing and points per line → Generate → Review per-drone overlays → Save/Upload per-drone missions.
- UI/UX considerations
  - Integrate controls into Area tab’s right panel; no horizontal scrolling.
  - Use QGCPalette and ScreenTools-based sizing; use qsTr() for all texts.
  - Toggle controls for:
    - Loiter/Hold at waypoint
    - RTL after waypoint
    - Loiter after RTL
    - Deconfliction mode (Altitude bands | Time staggering)
  - Map legend showing drone-to-color mapping and quick visibility toggles.

## Technical Architecture
- Frontend (QML)
  - Extend `PlanView.qml` Area tab section with a “Multi-Drone Planning” panel: drone count, altitude band start/step, time offset, RTL/Loiter policies.
  - `AreaPlanMapVisuals.qml`: multi-series overlays for each drone assignment (distinct colors from palette).
  - `AreaPlanEditor.qml`: expose new properties for policies and deconfliction parameters; keep logging.
- Backend (C++/QML integration)
  - Extend `AreaPlanEditor` to:
    - Accept planning params: `droneCount`, `altitudeBandStart`, `altitudeBandStep`, `timeOffsetPerDrone`, `rtlAfterEveryWaypoint`, `loiterAfterRtl`, `loiterTime`.
    - Partition area into N assignments (grid rows per drone or stripe slicing).
    - Generate per-drone waypoint lists from assignments.
    - Inject policy steps (Loiter, RTL) per waypoint as configured.
  - Data model
    - DroneAssignment { droneIndex, altitudeOffset, timeOffset, waypoints[] }
    - Waypoint { lat, lon, alt, loiterTime }
  - Algorithms
    - Partitioning: Stripe-based split along the short axis; round-robin grid-line assignment if preferred.
    - Deconfliction: altitudeOffset = altitudeBandStart + (droneIndex * altitudeBandStep).
    - Policy injection: if `rtlAfterEveryWaypoint`: append RTL item; if `loiterAfterRtl`: append Loiter after RTL.
- File I/O and upload
  - Save per-drone missions as separate QGC WPL files: `area_mission_drone_<idx>.waypoints`.
  - Upload flow: select a vehicle → load corresponding per-drone mission → upload.
- Visualization
  - Render per-drone grid lines and waypoint markers using palette colors (e.g., `qgcPal.colorGreen`, `qgcPal.colorBlue`, etc., cycling).
- Testing/SITL (optional but recommended)
  - Use `sitl-demo/` to validate multi-vehicle connectivity and mission uploads.
  - Simulate different altitude bands and timing offsets.

## Development Roadmap
- MVP
  - Inputs: droneCount, altitude band start/step, time offset per drone, Loiter time, RTL/Loiter policies.
  - Area split (stripe or line-based round-robin).
  - Per-drone mission generation with policy injection and altitude banding.
  - Per-drone visualization overlays and legend.
  - Save per-drone mission files.
- Phase 2
  - Time-based deconfliction preview (Gantt-style start times).
  - Enhanced partitioning (balanced workload by line length; non-rectangular footprints if available).
  - Bulk upload UX for multiple connected vehicles.
- Phase 3
  - Advanced deconfliction (avoidance corridors, inter-drone separation checks).
  - Battery-aware segmentation and mid-mission swap/RTL optimization.
  - Heterogeneous fleets (altitude limits, speeds).
  - Live progress overlay per drone.

## Logical Dependency Chain
1. Extend backend data model and AreaPlanEditor properties.
2. Partitioning and per-drone waypoint generation (no policy).
3. Policy injection (Loiter, RTL, Loiter after RTL) and altitude banding.
4. Per-drone overlays and legend.
5. Save per-drone missions and selective upload flow.
6. Advanced deconfliction/time staggering and UX refinements.

## Risks and Mitigations
- RTL-after-each-waypoint inflates mission time
  - Mitigation: Make policy optional and clearly labeled; warn about duration.
- Firmware variances in command semantics
  - Mitigation: Validate per-firmware command mappings; fallbacks for unsupported items.
- Operator confusion with many overlays
  - Mitigation: Legend, per-drone visibility toggles, color consistency.
- Performance with many waypoints/drones
  - Mitigation: Efficient generation, caching, incremental overlays.
- Deconfliction oversimplification
  - Mitigation: Start with altitude bands/time staggering; plan for future geometry-based checks.

## Appendix
- Source references
  - Template: `.taskmaster/templates/example_prd.txt` (section structure).
  - Requirements: `mvp/mulri-drone-storm-tuned.md` (policies: Loiter/Hold, RTL after each waypoint; area split; visualization; deconfliction).
- QGC integration points
  - QML: `src/QmlControls/PlanView.qml`, `src/QmlControls/AreaPlanEditor.qml`, `src/QmlControls/AreaPlanMapVisuals.qml`.
  - C++: `src/QmlControls/AreaPlanEditor.(h|cc)` (properties, generation, policy injection, file save/upload helpers).
- Acceptance criteria (MVP)
  - Given N drones and area parameters, system generates N mission files with:
    - Correct per-drone waypoint sets.
    - Applied altitude bands and configured Loiter/RTL policies.
    - Visual overlays distinct per drone.
  - Operator can save per-drone missions and upload each to the matching vehicle.

---

Key outcomes: Multi-drone area coverage with configurable per-waypoint RTL/Loiter policies, basic altitude/time deconfliction, clear map visualization, and an operator workflow integrated into the Area tab.


