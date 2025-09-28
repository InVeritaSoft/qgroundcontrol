#include "MissionGeneratorDialogue.h"
#include "MissionService.h"
#include "VehicleService.h"
#include "QGCLoggingCategory.h"
#include <QObject>
#include <QString>

QGC_LOGGING_CATEGORY(MissionGeneratorDialogueLog, "MissionGeneratorDialogueLog")

MissionGeneratorDialogue::MissionGeneratorDialogue(QObject* parent)
    : QObject(parent)
    , m_missionService(nullptr)
    , m_vehicleService(nullptr)
{
    m_missionService = new MissionService(this);
    m_vehicleService = new VehicleService(this);
    
    // Connect service signals to dialogue signals
    connect(m_missionService, &MissionService::missionGenerationStarted,
            this, &MissionGeneratorDialogue::missionGenerationStarted);
    connect(m_missionService, &MissionService::missionGenerationProgress,
            this, &MissionGeneratorDialogue::missionGenerationProgress);
    connect(m_missionService, &MissionService::missionGenerationCompleted,
            this, &MissionGeneratorDialogue::onMissionGenerationCompleted);
    
    connect(m_vehicleService, &VehicleService::vehicleInfoReady,
            this, &MissionGeneratorDialogue::vehicleInfoReady);
    connect(m_vehicleService, &VehicleService::vehicleDataReady,
            this, &MissionGeneratorDialogue::vehicleDataReady);
}

MissionGeneratorDialogue::~MissionGeneratorDialogue()
{
    // Qt will handle cleanup of child objects
}

void MissionGeneratorDialogue::generateMission(const QString& missionType, 
                                             int areaSize, 
                                             int altitude, 
                                             double speed, 
                                             const QString& description)
{
    qCDebug(MissionGeneratorDialogueLog) << "Generating mission:" 
                                        << "Type:" << missionType
                                        << "Area Size:" << areaSize
                                        << "Altitude:" << altitude
                                        << "Speed:" << speed
                                        << "Description:" << description;

    // Delegate to mission service
    m_missionService->generateMission(missionType, areaSize, altitude, speed, description);
}

void MissionGeneratorDialogue::getAllVehicles()
{
    qCDebug(MissionGeneratorDialogueLog) << "Getting all vehicles";
    
    // Delegate to vehicle service
    m_vehicleService->getAllVehicles();
}

void MissionGeneratorDialogue::onMissionGenerationCompleted(bool success, const QString& message)
{
    qCDebug(MissionGeneratorDialogueLog) << "Mission generation completed:" << success << message;
    
    // Emit the original missionGenerated signal for backward compatibility
    // Note: We don't have the original parameters here, so we emit with defaults
    // In a real implementation, you might want to store these parameters
    emit missionGenerated("Generated Mission", 0, 0, 0.0, message);
    
    // Also emit the new completion signal
    emit missionGenerationCompleted(success, message);
}

void MissionGeneratorDialogue::onMissionGenerationProgress(int current, int total)
{
    qCDebug(MissionGeneratorDialogueLog) << "Mission generation progress:" << current << "/" << total;
    emit missionGenerationProgress(current, total);
}
