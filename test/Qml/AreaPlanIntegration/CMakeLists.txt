# Register QML multi-drone integration tests for ctest
if(NOT QGC_BUILD_TESTING)
    return()
endif()

if(ANDROID OR IOS)
    return()
endif()

find_program(QMLTESTRUNNER_EXE NAMES qmltestrunner HINTS $ENV{QT_ROOT_DIR}/bin)
if(NOT QMLTESTRUNNER_EXE)
    message(WARNING "qmltestrunner not found; AreaPlanIntegration tests skipped")
    return()
endif()

add_test(
    NAME Qml_AreaPlanMultiDroneIntegration
    COMMAND ${QMLTESTRUNNER_EXE}
            -input ${CMAKE_CURRENT_SOURCE_DIR}
            -import ${CMAKE_SOURCE_DIR}/build
            -import ${CMAKE_SOURCE_DIR}/src
            -import ${CMAKE_BINARY_DIR}/qml
)

set_tests_properties(Qml_AreaPlanMultiDroneIntegration PROPERTIES LABELS "qml;integration;area-plan")

