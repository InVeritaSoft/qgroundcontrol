# QML test grouping
if(NOT QGC_BUILD_TESTING)
    return()
endif()

add_subdirectory(AreaPlanVisuals)
add_subdirectory(AreaPlanValidation)
add_subdirectory(AreaPlanEdgeCases)
add_subdirectory(AreaPlanIntegration)
add_subdirectory(AreaPlanPerformance)
