# Testing QGroundControl Area Planner (QML + ctest)

This document explains how to run and extend the QML-based tests added for the Area Planner, and how they integrate with CTest/CI.

Prerequisites
- Build with tests enabled: pass -DQGC_BUILD_TESTING=ON to CMake.
- Ensure qmltestrunner is available on PATH (comes with your Qt install). Optionally set QT_ROOT_DIR so CMake can locate it.

Where tests live
- QML tests are under test/Qml/...
  - AreaPlanValidation: validation of input parameter rules
  - AreaPlanEdgeCases: zero/min bounds, rotation wraparound
  - AreaPlanIntegration: multi-drone preview and policy toggles
  - AreaPlanPerformance: basic timing for generateWaypoints
- C++ unit tests remain under test/… and are wired through the existing add_qgc_test function.

Running tests
- From your build directory:
  - ctest --output-on-failure
  - Or build the convenience target:
    - cmake --build <build_dir> --target check --config Debug

Filtering tests
- By name: ctest -R Qml_AreaPlan
- By label: ctest -L qml, ctest -L validation, ctest -L integration, ctest -L performance

What the QML tests do
- Validation (Qml_AreaPlanValidation):
  - Exercises AreaPlanEditor.validateInput for width/height/spacing/numPoints/droneCount/altitudeBandStep/missionAltitude.
- Edge cases (Qml_AreaPlanEdgeCases):
  - Ensures zero dimensions produce empty waypoint lists and that minimal valid inputs still generate waypoints. Checks rotation across multiple angles.
- Integration (Qml_AreaPlanMultiDroneIntegration):
  - Configures multi-drone parameters and asserts equal waypoint distribution across drones. Verifies landAtTargetReturn toggle doesn’t break preview generation.
- Performance (Qml_AreaPlanPerformance):
  - Measures generateWaypoints runtime over several iterations and logs averages.

Skipping tests gracefully
- Tests check for QGroundControl.areaPlanEditor; if the app/QML context isn’t available, they call skip() to avoid false failures in limited environments.

Extending tests
- Add a new folder under test/Qml/<YourSuiteName>/ with tst_*.qml tests using QtTest.
- Create a CMakeLists.txt similar to the added suites, and register it in test/Qml/CMakeLists.txt using add_subdirectory.
- Prefer small deterministic scenarios and avoid external dependencies.

Troubleshooting
- qmltestrunner not found: ensure your Qt/bin is on PATH or set QT_ROOT_DIR. The CMake for each suite will warn and skip if not found.
- Visuals not refreshing: use the Force Refresh button in AreaPlanMapVisuals, or verify bindings by clearing HTML-escaped sequences (\u003c, \u003e, \u0026) in modified QML/C++ files.
- SITL integration: for end-to-end testing with vehicles, see mvp/docs/testing/sitl/SITL-SETUP.md and SITL-CONNECT.md, and mvp/docs/testing/e2e.

CI integration
- With -DQGC_BUILD_TESTING=ON and qmltestrunner in the CI image, these tests will be picked up automatically by ctest. Use ctest -L qml to run only QML tests.

