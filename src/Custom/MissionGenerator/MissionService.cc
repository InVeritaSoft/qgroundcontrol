#include "MissionService.h"
#include "PtahMissionGenerator.h"
#include "MissionUploadService.h"
#include "VehicleService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>

QGC_LOGGING_CATEGORY(MissionServiceLog, "MissionServiceLog")

MissionService::MissionService(QObject* parent)
    : QObject(parent)
    , m_ptahMissionGenerator(nullptr)
    , m_uploadService(nullptr)
    , m_vehicleService(nullptr)
{
    m_ptahMissionGenerator = new PtahMissionGenerator(this);
    m_uploadService = new MissionUploadService(this);
    m_vehicleService = new VehicleService(this);
    
    // Connect signals
    connect(m_vehicleService, &VehicleService::vehicleCoordinatesReady,
            this, &MissionService::onVehicleDataReady);
    connect(m_uploadService, &MissionUploadService::missionUploadCompleted,
            this, &MissionService::onMissionUploadCompleted);
}

MissionService::~MissionService()
{
    // Qt will handle cleanup of child objects
}

void MissionService::generateMission(const QString& missionType, 
                                   int areaSize, 
                                   int altitude, 
                                   double speed, 
                                   const QString& description)
{
    // GenCall8: MissionService::generateMission() - Start mission generation
    qCDebug(MissionServiceLog) << "GenCall8: MissionService::generateMission() - Starting mission generation";
    qCDebug(MissionServiceLog) << "Starting mission generation:" 
                              << "Type:" << missionType
                              << "Area Size:" << areaSize
                              << "Altitude:" << altitude
                              << "Speed:" << speed
                              << "Description:" << description;

    // GenCall9: Store current mission parameters
    qCDebug(MissionServiceLog) << "GenCall9: Storing current mission parameters";
    m_currentMissionType = missionType;
    m_currentAreaSize = areaSize;
    m_currentAltitude = altitude;
    m_currentSpeed = speed;
    m_currentDescription = description;

    // GenCall10: Emit mission generation started signal
    qCDebug(MissionServiceLog) << "GenCall10: Emitting mission generation started signal";
    emit missionGenerationStarted();
    
    // GenCall11: Get active vehicle data directly
    qCDebug(MissionServiceLog) << "GenCall11: Getting active vehicle data directly";
    
    // Get the active vehicle from MultiVehicleManager
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    Vehicle* activeVehicle = multiVehicleManager ? multiVehicleManager->activeVehicle() : nullptr;
    
    if (!activeVehicle) {
        qCWarning(MissionServiceLog) << "No active vehicle found";
        emit missionGenerationCompleted(false, "No active vehicle connected");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Active vehicle found:" << activeVehicle->id();
    
    // Get coordinates from active vehicle
    QGeoCoordinate vehicleCoord = activeVehicle->coordinate();
    if (!vehicleCoord.isValid()) {
        qCWarning(MissionServiceLog) << "Active vehicle has invalid coordinates";
        emit missionGenerationCompleted(false, "Active vehicle has invalid coordinates");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Active vehicle coordinates:" << vehicleCoord.toString();
    
    // Generate waypoints directly for active vehicle (10 meters in front)
    generateWaypointsForActiveVehicle(activeVehicle, vehicleCoord, 10.0);
}

void MissionService::generateWaypointsForActiveVehicle(Vehicle* vehicle, const QGeoCoordinate& vehicleCoord, double frontDistanceMeters)
{
    // GenCall12: MissionService::generateWaypointsForActiveVehicle() - Generate waypoints for active vehicle only
    qCDebug(MissionServiceLog) << "GenCall12: MissionService::generateWaypointsForActiveVehicle() - Generating waypoints for active vehicle only";
    qCDebug(MissionServiceLog) << "Vehicle ID:" << vehicle->id() << "Coordinates:" << vehicleCoord.toString() << "Front distance:" << frontDistanceMeters << "m";
    
    // Generate waypoints from the vehicle's position with front offset
    generateWaypointsFromPosition(vehicleCoord, m_currentAreaSize, m_currentAltitude, frontDistanceMeters);
}

void MissionService::onVehicleDataReady(const QList<QGeoCoordinate>& vehicleCoordinates)
{
    // GenCall16: MissionService::onVehicleDataReady() - Process vehicle coordinates
    qCDebug(MissionServiceLog) << "GenCall16: MissionService::onVehicleDataReady() - Processing vehicle coordinates";
    qCDebug(MissionServiceLog) << "Received vehicle coordinates:" << vehicleCoordinates.size();
    
    // GenCall17: Validate coordinates
    qCDebug(MissionServiceLog) << "GenCall17: Validating coordinates";
    if (vehicleCoordinates.isEmpty()) {
        emit missionGenerationCompleted(false, "No valid vehicle coordinates found");
        return;
    }
    
    // GenCall18: Generate waypoints from vehicle positions (legacy method - not used in single vehicle mode)
    qCDebug(MissionServiceLog) << "GenCall18: Legacy multi-vehicle method called - not used in single vehicle mode";
    // This method is kept for compatibility but single vehicle mode bypasses it
}

void MissionService::generateWaypointsFromPosition(const QGeoCoordinate& vehiclePosition,
                                                   int areaSize, 
                                                   int altitude,
                                                   double frontDistanceMeters)
{
    // GenCall19: MissionService::generateWaypointsFromPosition() - Generate waypoints for single vehicle
    qCDebug(MissionServiceLog) << "GenCall19: MissionService::generateWaypointsFromPosition() - Generating waypoints for single vehicle";
    if (!m_ptahMissionGenerator) {
        emit missionGenerationCompleted(false, "Mission generator not available");
        return;
    }
    
    // GenCall20: Calculate point in front of vehicle for waypoint generation
    qCDebug(MissionServiceLog) << "GenCall20: Calculating point in front of vehicle";
    QGeoCoordinate referencePoint = m_ptahMissionGenerator->calculateNewCoordinates(vehiclePosition, 0.0, frontDistanceMeters);
    if (!referencePoint.isValid()) {
        emit missionGenerationCompleted(false, "Could not calculate front position");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Vehicle position:" << vehiclePosition.toString();
    qCDebug(MissionServiceLog) << "Front reference point:" << referencePoint.toString();
    qCDebug(MissionServiceLog) << "Reference point values - Lat:" << referencePoint.latitude() << "Lng:" << referencePoint.longitude() << "Alt:" << referencePoint.altitude();
    
    // GenCall21: Use a fixed bearing for waypoint generation (East-West pattern)
    qCDebug(MissionServiceLog) << "GenCall21: Using fixed bearing for waypoint generation";
    double bearing = 90.0; // East-West pattern (90 degrees = East)
    
    qCDebug(MissionServiceLog) << "Using fixed bearing:" << bearing << "degrees (East-West pattern)";
    
    // GenCall22: Generate waypoints using PtahMissionGenerator
    qCDebug(MissionServiceLog) << "GenCall22: Generating waypoints using PtahMissionGenerator";
    QList<QGeoCoordinate> waypoints = m_ptahMissionGenerator->generateCoordinatesInBothDirections(
        referencePoint, bearing, 3.0, static_cast<double>(areaSize));
    
    qCDebug(MissionServiceLog) << "Generated" << waypoints.size() << "waypoints";
    
    // Log first few waypoints for debugging
    int logCount = waypoints.size() < 3 ? waypoints.size() : 3;
    for (int i = 0; i < logCount; i++) {
        qCDebug(MissionServiceLog) << "Waypoint" << i << ":" << waypoints[i].toString();
    }
    if (waypoints.size() > 3) {
        qCDebug(MissionServiceLog) << "... and" << (waypoints.size() - 3) << "more waypoints";
    }
    
    // GenCall23: Set altitude for all waypoints
    qCDebug(MissionServiceLog) << "GenCall23: Setting altitude for all waypoints to" << altitude << "meters";
    for (QGeoCoordinate& coord : waypoints) {
        coord.setAltitude(altitude);
    }
    
    // GenCall24: Emit waypoints generated signal
    qCDebug(MissionServiceLog) << "GenCall24: Emitting waypoints generated signal";
    emit waypointsGenerated(waypoints);
    
    // GenCall25: Distribute waypoints among NON ID 1 drones
    qCDebug(MissionServiceLog) << "GenCall25: Distributing waypoints among NON ID 1 drones";
    
    // Get all connected vehicles
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        emit missionGenerationCompleted(false, "MultiVehicleManager not available");
        return;
    }
    
    QmlObjectListModel* vehiclesModel = multiVehicleManager->vehicles();
    QList<Vehicle*> nonId1Vehicles;
    
    // Filter out vehicle ID 1 (for special mission later)
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() != 1) {
            nonId1Vehicles.append(vehicle);
        }
    }
    
    if (nonId1Vehicles.isEmpty()) {
        emit missionGenerationCompleted(false, "No NON ID 1 vehicles found");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Found" << nonId1Vehicles.size() << "NON ID 1 vehicles";
    
    // Distribute waypoints among NON ID 1 vehicles
    QList<QList<QGeoCoordinate>> distributedWaypoints = m_ptahMissionGenerator->distributeWaypointsAmongDrones(waypoints, nonId1Vehicles.size());
    
    // Upload missions with different altitudes for each drone
    // Each mission will include: TAKEOFF -> WAYPOINTS -> LAND
    for (int i = 0; i < nonId1Vehicles.size() && i < distributedWaypoints.size(); i++) {
        Vehicle* vehicle = nonId1Vehicles[i];
        QList<QGeoCoordinate> vehicleWaypoints = distributedWaypoints[i];
        
        // Set different altitude for each drone to avoid collisions
        int vehicleAltitude = altitude + (i * 10); // 10m altitude difference per drone
        
        qCDebug(MissionServiceLog) << "Uploading mission to vehicle ID" << vehicle->id() << "at altitude" << vehicleAltitude;
        qCDebug(MissionServiceLog) << "Mission sequence: TAKEOFF ->" << vehicleWaypoints.size() << "WAYPOINTS -> LAND";
        
        // Set altitude for all waypoints
        for (QGeoCoordinate& coord : vehicleWaypoints) {
            coord.setAltitude(vehicleAltitude);
        }
        
        // Upload complete mission (takeoff + waypoints + land) to this specific vehicle
        qCDebug(MissionServiceLog) << "Calling uploadMissionToVehicle for vehicle ID" << vehicle->id() << "with" << vehicleWaypoints.size() << "waypoints at altitude" << vehicleAltitude;
        m_uploadService->uploadMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude);
    }
    
    // GenCall26: Create special observation mission for drone ID 1
    qCDebug(MissionServiceLog) << "GenCall26: Creating special observation mission for drone ID 1";
    
    // Find drone ID 1
    Vehicle* droneId1 = nullptr;
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() == 1) {
            droneId1 = vehicle;
            break;
        }
    }
    
    if (droneId1) {
        // Calculate safe observation position
        QGeoCoordinate missionCenter = m_ptahMissionGenerator->calculateMiddlePoint(waypoints);
        QGeoCoordinate observationPosition = m_ptahMissionGenerator->calculateSafeObservationPosition(missionCenter, waypoints, altitude);
        
        if (observationPosition.isValid()) {
            qCDebug(MissionServiceLog) << "Drone ID 1 observation position:" << observationPosition.toString();
            qCDebug(MissionServiceLog) << "Drone ID 1 mission sequence: TAKEOFF -> LOITER -> LAND";
            
            // Upload loiter mission to drone ID 1 (includes takeoff and land)
            qCDebug(MissionServiceLog) << "Calling uploadLoiterMissionToVehicle for drone ID 1 at position:" << observationPosition.toString() << "altitude:" << (altitude + 20);
            m_uploadService->uploadLoiterMissionToVehicle(droneId1, observationPosition, altitude + 20); // 20m higher than mission altitude
        } else {
            qCWarning(MissionServiceLog) << "Could not calculate safe observation position for drone ID 1";
        }
    } else {
        qCDebug(MissionServiceLog) << "Drone ID 1 not found - skipping observation mission";
    }
}

void MissionService::onMissionUploadCompleted(bool success, const QString& message)
{
    // GenCall47: MissionService::onMissionUploadCompleted() - Final completion callback
    qCDebug(MissionServiceLog) << "Mission upload completed:" << success << message;
    
    // GenCall48: Emit final mission generation completed signal
    emit missionGenerationCompleted(success, message);
}

void MissionService::processMissionGeneration(const QString& missionType, 
                                            int areaSize, 
                                            int altitude, 
                                            double speed, 
                                            const QString& description)
{
    // This method is kept for compatibility but delegates to generateMission
    generateMission(missionType, areaSize, altitude, speed, description);
}
