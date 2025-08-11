# QML visual test harness for AreaPlan visuals
# Runs with ctest by invoking qmltestrunner from PATH.

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# Only desktop for now
if(ANDROID OR IOS)
    return()
endif()

# Minimal smoke test that qmltestrunner is available in PATH
find_program(QMLTESTRUNNER_EXE NAMES qmltestrunner HINTS $ENV{QT_ROOT_DIR}/bin)

if(NOT QMLTESTRUNNER_EXE)
    message(WARNING "qmltestrunner not found in PATH (or $ENV{QT_ROOT_DIR}/bin). QML visual tests will be skipped.")
    return()
endif()

# Register the test directory with ctest
add_test(
    NAME Qml_AreaPlanVisuals
    COMMAND ${QMLTESTRUNNER_EXE}
            -input ${CMAKE_CURRENT_SOURCE_DIR}
            -import ${CMAKE_SOURCE_DIR}/build  # allow built QML modules
            -import ${CMAKE_SOURCE_DIR}/src    # source QML
            -import ${CMAKE_BINARY_DIR}/qml    # generated qml cache dir
)

set_tests_properties(Qml_AreaPlanVisuals PROPERTIES
    LABELS "qml;visual;area-plan"
)
