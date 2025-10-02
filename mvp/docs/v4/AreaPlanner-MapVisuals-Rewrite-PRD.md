# Area Planner Map Visuals Rewrite – Product Requirements Document (PRD)

Overview
- Goal: Redesign and re-implement the Area Planner visual stack and adjacent UI (AreaPlan*, PlanView.qml, FlyView.qml) from the ground up for clarity, performance, and testability.
- Scope: Replace legacy or experimental visuals with a robust, modular architecture that cleanly separates data, business logic, and rendering, with automated tests to validate typical and edge user flows.
- Target Platforms: Desktop (Windows primary), with parity for macOS/Linux where applicable.
- Non-Goals (for this phase): Advanced polygon footprints, terrain-aware coverage planning, complex deconfliction beyond altitude/time staggering.

Primary Objectives
1) Visual rewrite
   - AreaPlanMapVisuals.qml: re-architect to render area rectangle, grid, waypoint markers, and per-drone overlays with strong property-driven updates.
   - PlanView.qml: simplify and reflow the Area Planning panel; ensure no horizontal scrolling, consistent typography, and accessibility in QGC standards.
   - FlyView.qml: establish a minimal, clear visual contract to reflect planned overlays in-flight (read-only overlays, basic visibility toggles), without coupling.
2) Interaction model
   - Clean drawing mode: enter/exit, set center, resize, rotate, move (N/E/S/W/Center).
   - Fast and predictable binding updates: property changes propagate to visuals deterministically with no stale states.
   - Debounce expensive re-renders; reuse objects when possible.
3) Multi-drone overlays
   - Per-drone color scheme with legend and per-drone visibility toggles.
   - Basic deconfliction: altitude bands and time staggering; preview and mission generation alignment.
4) Testability
   - Deterministic unit tests for geometry, partitioning, mission item sequencing, and IO.
   - QML visuals tests for overlay presence, counts, and bindings with a known seed.
   - E2E smoke checks that validate mission insertion and per-drone export on a CI-friendly subset.
5) Developer UX
   - Clear API surface in AreaPlanEditor C++ backend with Q_PROPERTY/Q_INVOKABLE.
   - Strict qsTr() usage and ScreenTools sizing in QML; QGCPalette for colors.

Current State (Context Summary)
- AreaPlanEditor.(h|cc|qml) exposes properties and actions for area dimensions, spacing, points/line, rotation, altitude, multi-drone (count, bands, staggering), and policies (RTL per WP, loiter after RTL), with per-drone preview/generation and upload helpers.
- AreaPlanMapVisuals.qml draws rectangle, grid, waypoint markers, and per-drone overlays with legend, including some debounce/object reuse.
- PlanView integrates AreaPlanEditor UI; FlyView has custom overlay hooks (reviewed minimally here, targeted for a light refactor).
- Tests exist for AreaPlanEditor in test/MissionManager; no dedicated Qt Quick tests for map visuals.

User Stories
- As a field operator, I want to define an area quickly, see clear overlays, and upload missions with minimal clicks.
- As a mission planner, I need precise control over spacing, counts, altitude, and multi-drone policies, and I must verify the plan visually.
- As a tester, I require robust automated tests for geometry counts, map overlay visibility, and mission item sequences.

UX and UI Principles
- Consistent QGC look-and-feel: QGCPalette, ScreenTools, qsTr, readable labels, and grouped controls.
- No horizontal scroll in parameter panels; group by task (Area, Interaction, Mission, Multi-Drone, Status/Preview).
- Visual legend for per-drone overlays with quick toggles and synchronized colors in previews and map.

Functional Requirements
A. Area and Grid
- Define rectangle by center, width, height, rotation (degrees, normalized 0–360).
- Grid lines based on lineSpacing; number of lines floor(height/lineSpacing).
- Waypoints per line determined by numPoints; equal spacing along width axis, honoring rotation.
B. Multi-Drone
- Properties: droneCount (>=1), altitudeBandStart, altitudeBandStep, timeOffsetPerDrone.
- Partitioning strategy: round-robin assignment of stripes/lines to drones for MVP.
- Policies: toggles for RTL after every waypoint; Loiter after RTL; loiterTime.
- Outputs: per-drone waypoint sets; consistent overlay counts; per-drone files.
C. Visualization
- Overlays: area polygon, grid polylines, waypoint markers, per-drone markers with legend.
- Performance: debounce updates (e.g., Timer or queued connections), object reuse where possible.
- Z-order: base map < area < grid < waypoints < vehicle icons < UI controls.
D. Mission I/O
- Add to Mission Tab (single and per-drone); serialization to QGC WPL 110.
- Upload flows: vehicle mapping, single-drone upload, upload all mapped.
E. Status and Feedback
- Status messages from backend (signals) displayed in UI.
- Validation errors surfaced with clear text and suggested fixes.

Non-Functional Requirements
- Performance: typical changes must update overlays within 100ms on mid-range Windows dev machines; large plans (e.g., >1000 waypoints) may be slower but must not freeze UI (debounce/prune).
- Memory: avoid unbounded growth; reuse objects; clear on tab switch.
- Stability: property changes must be idempotent and test-covered.

Architecture and Components
1) Backend (C++)
- AreaPlanEditor
  - Q_PROPERTY surface for all parameters and states (areaWidth/Height, lineSpacing, numPoints, missionAltitude, areaCenter, areaRotation, loiterTime, droneCount, altitudeBand*, timeOffsetPerDrone, rtlAfterEveryWaypoint, loiterAfterRtl, isDrawingMode).
  - Q_INVOKABLE API: generateWaypoints, computePerDroneWaypointPreview, addWaypointsToMission, addPerDroneToMission, addAllDronesToMission, saveMissionFile, savePerDroneMissionFiles, uploadPerDroneMissionToVehicle, startMission, movement/rotation setters, resetArea.
  - Algorithms: geodesic offset for rectangle corners; stripe/line generation; round-robin assignment; policy injection; mission file IO.
  - Signals: statusChanged(QString), properties changed notifications.
2) Frontend (QML)
- AreaPlanEditor.qml: parameter panels, drawing mode controls, mission controls, per-drone vehicle mapping, preview, status.
- AreaPlanMapVisuals.qml: area polygon, grid polylines, waypoint markers, per-drone overlays, legend; Connections to AreaPlanEditor properties.
- PlanView.qml: container integration of AreaPlanEditor and map visuals with simplified layout and clear grouping.
- FlyView.qml: minimal read-only overlays view and visibility toggles; no planning controls.

Rewrite Plan (Deliverables)
Phase 1 – Foundations
- Extract a clean VisualModel (pure JS/QML structs) for overlays from backend data: area polygon points, grid polyline arrays, waypoint marker coordinates, per-drone color mapping.
- Implement deterministic bindings: a single source-of-truth object that AreaPlanMapVisuals.qml renders.
- Add property change coalescing (queued updates) and object reuse pools for markers/lines.

Phase 2 – PlanView and Controls
- Reflow AreaPlanEditor UI: reorganize into Area, Interaction, Rotation, Mission, Multi-Drone, Preview/Status.
- Ensure accessibility: label widths, consistent sizes, ScreenTools-derived dimensions; no horizontal scroll.

Phase 3 – FlyView Display
- Introduce a lean overlay presenter for FlyView with read-only toggles and legend; decouple from PlanView logic.
- Confirm no cross-tab state leakage (activate/deactivate hooks).

Phase 4 – Persistence and Upload UX
- Ensure consistent per-drone file naming; enforce vehicle mapping UX in UI; add “Upload All Mapped”.
- Strengthen error feedback: vehicle not available, upload error paths.

Phase 5 – Performance and Polish
- Debounce tuning; offload heavy geometry recalcs when possible.
- Finalize legend semantics and colors; confirm light/dark theme legibility.

Acceptance Criteria
- Geometry and Counts
  - Grid lines count equals floor(height / lineSpacing) with correct rotation applied.
  - Waypoint counts per line equal numPoints; total equals lines × numPoints (single-drone), and equals sum across drones (multi-drone, round-robin).
- Visualization
  - Area polygon corners track center, width/height, rotation changes deterministically.
  - Overlays update within 100ms for typical plans (<200 waypoints) on a dev Windows system.
  - Per-drone colors appear consistently in map overlays, legend, and preview.
- Mission Generation and Upload
  - Single and per-drone mission insertion succeed; per-drone WPL files serialize with expected items.
  - Policy toggles apply Loiter/RTL as configured.
  - Uploads to selected vehicle(s) succeed with progress and error reporting signals.
- UI Quality
  - All user-facing text uses qsTr(); sizes via ScreenTools; colors via QGCPalette.
  - No horizontal scroll in control areas; consistent margins and spacing.

Test Strategy

Test Authoring Rules (QML Visuals)
- Structure: place QML tests under test/Qml/AreaPlanVisuals with files named tst_*.qml and a local CMakeLists.txt.
- Runner: tests run via qmltestrunner through ctest; ensure qmltestrunner is in PATH or available in %QT_ROOT_DIR%/bin.
- Determinism: avoid timers where possible; if debounce is required, use short, explicit waits (≤100ms) and document them.
- Isolation: do not require network, logs, or external services; do not depend on actual vehicles. Visuals should bind to data-only models.
- Assertions: prefer counts/visibility checks over pixel comparisons. Examples: number of grid lines, total waypoint markers, per-drone visibility toggles.
- Skips: gracefully skip tests when QGC QML context is not available (headless or minimal CI runners).
- Naming: use clear, intent-revealing test names (e.g., test_grid_line_count_matches_height_over_spacing).
- Internationalization: never assert on UI strings; assert on numeric properties or item counts.
- Performance: keep each test under 1s; suites under 10s.
- Ownership: any new visual feature must include at least one QML visual test and one C++ unit test for its data model.
A. Unit Tests (C++: test/MissionManager and related)
- Geometry math
  - Rectangle corner computation from center/width/height/rotation (tolerance in meters).
  - Grid line count and endpoints for canonical inputs.
- Waypoint generation
  - Counts for various numPoints, lineSpacing, width/height combinations.
  - Rotation invariance: rotated vs non-rotated plans yield correctly transformed coordinates.
- Multi-drone partitioning
  - Round-robin stripe assignment correctness; sum of per-drone waypoints equals total.
  - Altitude/time offsets applied per drone.
- Policy injection
  - Presence/ordering of RTL and Loiter items when toggled; none when disabled.
- IO
  - WPL 110 serialization and basic parse round-trip sanity (counts/types).

B. QML/Visual Tests (Qt Quick Test / qmltestrunner)
- AreaPlanMapVisuals.qml
  - Overlay item presence: polygon, grid polylines, markers, per-drone markers.
  - Counts (e.g., grid line items == expected; marker count == expected) under seeded parameters.
  - Visibility toggles: per-drone show/hide affects item visibility without removing underlying data.
- PlanView.qml integration
  - Property bindings propagate to visual model and render items after debounce window.
  - Drawing mode UI toggles wired; rotation/position controls invoke backend methods.

C. E2E/SITL (Manual + Optional Automated Harness)
- Single-drone workflow: define area, generate, insert, save, upload (if vehicle present).
- Multi-drone: set droneCount>1, bands/offsets, insert per-drone, save per-drone files; optional upload to multiple vehicles (SITL).
- Validation: screenshots and logs captured per mvp/docs/testing checklists.

Tooling and Test Infra
- Extend test/MissionManager with new unit tests for AreaPlanEditor and helpers (AreaPlanEditorTest.cc/h already present—augment as needed).
- Add a new QML test directory (e.g., test/Qml/AreaPlanVisuals) with Qt Quick Test; ensure CMake enables Qt::Test and qmltestrunner targets on desktop.
- CI-friendly seeds and deterministic waits (debounce intervals exposed/shortened in test builds).

Risks and Mitigations
- Visual jitter or stale overlays: mitigate with single source-of-truth visual model and queued updates.
- Performance degradation on large plans: pool QML objects; cap per-frame work; debounce.
- Cross-tab state coupling (Plan vs Fly): introduce explicit activation gates and independent presenters.
- Firmware variance for mission items: guard with firmware-specific mapping or fallbacks.

Deliverables Checklist
- Refactored AreaPlanMapVisuals.qml with pooled items and visual model bindings.
- Reflowed PlanView.qml section for Area Planning; all labels and controls standardized.
- Lean FlyView overlays integration with toggles.
- Extended AreaPlanEditor tests (C++); new QML tests for visuals.
- Updated docs: user flows, developer notes, and testing checklists.

Build and Smoke Test (Windows, Qt6, ccache)
Prerequisites
- Ninja available in PATH.
- ccache installed and available in PATH.
- Qt6 installed; set environment variable QT_ROOT_DIR to the Qt installation root containing lib/cmake/Qt6/qt.toolchain.cmake.
  - Example (PowerShell):
    - $env:QT_ROOT_DIR = "C:/Qt/6.8.3/msvc2022_64"
- Optional: Ensure Visual Studio Build Tools (MSVC) or LLVM toolchain is configured as required by the Qt toolchain file.

Configure (ccache-enabled)
- cmake --preset Windows-ccache
  - This preset inherits "ccache" settings from cmake/presets/common.json and sets CMAKE_C/CXX_COMPILER_LAUNCHER to ccache.
  - Binary dir: ../build/qt6-Windows

Build (Release)
- cmake --build --preset Windows-ccache

Build (Debug)
- cmake --build --preset Windows-debug-ccache

Run Tests (if enabled)
- ctest --preset Windows-ccache --output-on-failure
  - Note: QGC_BUILD_TESTING defaults to ON for Debug only (see cmake/CustomOptions.cmake). To force tests ON, pass -DQGC_BUILD_TESTING=ON during configure via a user preset or command line override.

Smoke Launch
- After build, run the produced QGroundControl binary from build/qt6-Windows/<Config>/
- Verify the Area Planning tab loads; try toggling drawing mode and basic parameter edits.

Notes and Tips
- If cmake cannot locate the Qt toolchain, verify QT_ROOT_DIR is set in your environment before running presets.
- To change Qt version bounds, see top-level CMakeLists.txt (QGC_QT_MINIMUM_VERSION and MAXIMUM). Only documented versions are supported.
- To ensure caching is ON: QGC_USE_CACHE=ON (default). The Windows-ccache preset already enables compiler launcher to ccache.

Follow-ups (Post-PRD)
- Add a user preset file (CMakeUserPresets.json, git-ignored) for local overrides like custom QT_ROOT_DIR if needed.
- Consider adding a minimal qmltestrunner invocation target in test CMake to standardize QML visual tests.
- Optionally expose a build workflow preset for “configure+build+test (Debug)” to speed local smoke checks.

