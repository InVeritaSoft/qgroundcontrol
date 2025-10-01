# Custom Mission Generator Module
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MissionGenerator/MissionGeneratorDialogue.cc
        MissionGenerator/MissionGeneratorDialogue.h
        MissionGenerator/MissionService.cc
        MissionGenerator/MissionService.h
        MissionGenerator/VehicleService.cc
        MissionGenerator/VehicleService.h
        MissionGenerator/MissionUploadService.cc
        MissionGenerator/MissionUploadService.h
        MissionGenerator/PtahMissionGenerator.cc
        MissionGenerator/PtahMissionGenerator.h
        MissionGenerator/CollisionDetectionService.cc
        MissionGenerator/CollisionDetectionService.h
        MissionGenerator/ShortcutManager.cc
        MissionGenerator/ShortcutManager.h
        MissionGenerator/CustomFirmwarePlugin.cc
        MissionGenerator/CustomFirmwarePlugin.h
        MissionGenerator/CustomFirmwarePluginFactory.cc
        MissionGenerator/CustomFirmwarePluginFactory.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # Add src directory to include path
)

# Link Qt modules for positioning
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt6::Positioning
    Qt6::Core
)