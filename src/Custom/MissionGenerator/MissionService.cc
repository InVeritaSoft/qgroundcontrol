#include "MissionService.h"
#include "PtahMissionGenerator.h"
#include "MissionUploadService.h"
#include "VehicleService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "MissionManager/MissionManager.h"
// #include "QGCApplication.h"  // Not used directly
#include "QGCLoggingCategory.h"
#include "Settings/AppSettings.h"
#include "Settings/SettingsManager.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QGeoCoordinate>
#include <QList>

QGC_LOGGING_CATEGORY(MissionServiceLog, "MissionServiceLog")

MissionService::MissionService(QObject* parent)
    : QObject(parent)
    , m_ptahMissionGenerator(nullptr)
    , m_uploadService(nullptr)
    , m_vehicleService(nullptr)
    , m_totalTripods(0)
    , m_installedTripods(0)
    , m_explodeButtonEnabled(false)
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
                                   const QString& description,
                                   double frontDistance,
                                   bool payloadDropMode,
                                   int loiterTimeSeconds,
                                   int bendHeight,
                                   double payloadDropHeight,
                                   int servoDelaySeconds,
                                   double observationDistance)
{
    // GenCall8: MissionService::generateMission() - Start mission generation
    qCDebug(MissionServiceLog) << "GenCall8: MissionService::generateMission() - Starting mission generation";
    qCDebug(MissionServiceLog) << "Starting mission generation:" 
                              << "Type:" << missionType
                              << "Area Size:" << areaSize
                              << "Altitude:" << altitude
                              << "Speed:" << speed
                              << "Description:" << description
                              << "Front Distance:" << frontDistance
                              << "Payload Drop Mode:" << payloadDropMode
                              << "Loiter Time:" << loiterTimeSeconds << "seconds"
                              << "Bend Height:" << bendHeight << "m"
                              << "Payload Drop Height:" << payloadDropHeight << "m"
                              << "Servo Delay:" << servoDelaySeconds << "seconds";

    // GenCall9: Store current mission parameters
    qCDebug(MissionServiceLog) << "GenCall9: Storing current mission parameters";
    m_currentMissionType = missionType;
    m_currentAreaSize = areaSize;
    m_currentAltitude = altitude;
    m_currentSpeed = speed;
    m_currentDescription = description;
    
    // Calculate total tripods for demining operations
    if (payloadDropMode) {
        // For demining operations, calculate tripods based on area size
        // Each waypoint represents a tripod installation point
        const double stepDistance = 4.5; // meters between bulbs and drones
        m_totalTripods = static_cast<int>(areaSize / stepDistance);
        m_installedTripods = 0;
        m_vehicleTripodCount.clear();
        m_explodeButtonEnabled = false;
        
        qCDebug(MissionServiceLog) << "Demining mode: Total tripods to install:" << m_totalTripods;
        qCDebug(MissionServiceLog) << "Step distance:" << stepDistance << "m, Area size:" << areaSize << "m";
    } else {
        // Reset tripod tracking for non-demining missions
        resetTripodTracking();
    }

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
    
    // Clear existing mission items from vehicle to prevent old orange lines
    qCDebug(MissionServiceLog) << "Clearing existing mission items from vehicle to prevent old orange lines";
    if (activeVehicle->missionManager()) {
        activeVehicle->missionManager()->removeAll();
        qCDebug(MissionServiceLog) << "Mission clear command sent to vehicle";
        
        // Use QTimer to delay waypoint generation until mission clear completes
        QTimer::singleShot(1000, this, [this, activeVehicle, vehicleCoord, frontDistance, payloadDropMode, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds, observationDistance]() {
            qCDebug(MissionServiceLog) << "Mission clear delay completed, generating new waypoints";
            generateWaypointsForActiveVehicle(activeVehicle, vehicleCoord, frontDistance, payloadDropMode, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds, observationDistance);
        });
    } else {
        // If no mission manager, generate waypoints immediately
        generateWaypointsForActiveVehicle(activeVehicle, vehicleCoord, frontDistance, payloadDropMode, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds, observationDistance);
    }
}

void MissionService::generateWaypointsForActiveVehicle(Vehicle* vehicle, const QGeoCoordinate& vehicleCoord, double frontDistanceMeters, bool payloadDropMode, int loiterTimeSeconds, int bendHeight, double payloadDropHeight, int servoDelaySeconds, double observationDistance)
{
    // GenCall12: MissionService::generateWaypointsForActiveVehicle() - Generate waypoints for active vehicle only
    qCDebug(MissionServiceLog) << "GenCall12: MissionService::generateWaypointsForActiveVehicle() - Generating waypoints for active vehicle only";
    qCDebug(MissionServiceLog) << "Vehicle ID:" << vehicle->id() << "Coordinates:" << vehicleCoord.toString() << "Front distance:" << frontDistanceMeters << "m";
    
    // Generate waypoints from the vehicle's position with front offset
    generateWaypointsFromPosition(vehicleCoord, m_currentAreaSize, m_currentAltitude, frontDistanceMeters, payloadDropMode, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds, observationDistance);
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
                                                   double frontDistanceMeters,
                                                   bool payloadDropMode,
                                                   int loiterTimeSeconds,
                                                   int bendHeight,
                                                   double payloadDropHeight,
                                                   int servoDelaySeconds,
                                                   double observationDistance)
{
    // GenCall19: MissionService::generateWaypointsFromPosition() - Generate waypoints for single vehicle
    qCDebug(MissionServiceLog) << "GenCall19: MissionService::generateWaypointsFromPosition() - Generating waypoints for single vehicle";
    if (!m_ptahMissionGenerator) {
        emit missionGenerationCompleted(false, "Mission generator not available");
        return;
    }
    
    // Get MultiVehicleManager instance for use throughout the function
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    
    // GenCall20: Get vehicle heading and calculate point in front of vehicle for waypoint generation
    qCDebug(MissionServiceLog) << "GenCall20: Getting vehicle heading and calculating point in front of vehicle";
    
    // Get the active vehicle to retrieve its heading
    double vehicleHeading = 0.0; // Default to north if no vehicle available
    if (multiVehicleManager && multiVehicleManager->activeVehicle()) {
        Vehicle* activeVehicle = multiVehicleManager->activeVehicle();
        if (activeVehicle && activeVehicle->heading()) {
            vehicleHeading = activeVehicle->heading()->rawValue().toDouble();
            qCDebug(MissionServiceLog) << "Vehicle heading:" << vehicleHeading << "degrees";
        }
    }
    
    QGeoCoordinate referencePoint = m_ptahMissionGenerator->calculateNewCoordinates(vehiclePosition, vehicleHeading, frontDistanceMeters);
    if (!referencePoint.isValid()) {
        emit missionGenerationCompleted(false, "Could not calculate front position");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Vehicle position:" << vehiclePosition.toString();
    qCDebug(MissionServiceLog) << "Front reference point:" << referencePoint.toString();
    qCDebug(MissionServiceLog) << "Reference point values - Lat:" << referencePoint.latitude() << "Lng:" << referencePoint.longitude() << "Alt:" << referencePoint.altitude();
    
    // Validate reference point - if it's in ocean, try alternative positions
    if (!m_ptahMissionGenerator->isValidLandCoordinate(referencePoint)) {
        qCWarning(MissionServiceLog) << "Reference point is in ocean, trying alternative positions";
        qCDebug(MissionServiceLog) << "Reference point coordinates:" << referencePoint.latitude() << "," << referencePoint.longitude();
        
        // Try different directions to find a land-based reference point
        bool foundLandReference = false;
        for (int dir = 0; dir < 8 && !foundLandReference; dir++) {
            double alternativeBearing = dir * 45.0; // Try every 45 degrees
            QGeoCoordinate alternativeRef = m_ptahMissionGenerator->calculateNewCoordinates(vehiclePosition, alternativeBearing, frontDistanceMeters);
            
            qDebug() << "Trying reference point at bearing" << alternativeBearing << ":" << alternativeRef.toString();
            
            if (alternativeRef.isValid() && m_ptahMissionGenerator->isValidLandCoordinate(alternativeRef)) {
                referencePoint = alternativeRef;
                foundLandReference = true;
                qCDebug(MissionServiceLog) << "Found land-based reference point at bearing" << alternativeBearing << ":" << referencePoint.toString();
            } else {
                qDebug() << "Reference point at bearing" << alternativeBearing << "failed validation";
            }
        }
        
        // If still no land reference found, try different distances
        if (!foundLandReference) {
            qCWarning(MissionServiceLog) << "No land reference found at 10m, trying different distances";
            for (double distance = 5.0; distance <= 50.0 && !foundLandReference; distance += 5.0) {
                for (int dir = 0; dir < 8 && !foundLandReference; dir++) {
                    double alternativeBearing = dir * 45.0;
                    QGeoCoordinate alternativeRef = m_ptahMissionGenerator->calculateNewCoordinates(vehiclePosition, alternativeBearing, distance);
                    
                    if (alternativeRef.isValid() && m_ptahMissionGenerator->isValidLandCoordinate(alternativeRef)) {
                        referencePoint = alternativeRef;
                        foundLandReference = true;
                        qCDebug(MissionServiceLog) << "Found land-based reference point at bearing" << alternativeBearing << "distance" << distance << "m:" << referencePoint.toString();
                    }
                }
            }
        }
        
        // If still no land reference found, use the vehicle position itself
        if (!foundLandReference) {
            qCWarning(MissionServiceLog) << "Could not find land-based reference point, using vehicle position";
            referencePoint = vehiclePosition;
        }
    }
    
    qCDebug(MissionServiceLog) << "Final reference point:" << referencePoint.toString();
    
    // GenCall21: Use vehicle heading + 90 degrees for waypoint generation (perpendicular row pattern)
    qCDebug(MissionServiceLog) << "GenCall21: Using vehicle heading + 90 degrees for waypoint generation";
    double bearing = vehicleHeading + 90.0; // Add 90 degrees to vehicle's heading for perpendicular row pattern
    
    // Normalize bearing to 0-360 range
    while (bearing >= 360.0) bearing -= 360.0;
    while (bearing < 0.0) bearing += 360.0;
    
    qCDebug(MissionServiceLog) << "Using perpendicular bearing:" << bearing << "degrees (90° offset from vehicle heading:" << vehicleHeading << "degrees)";
    
    // GenCall22: Generate waypoints using PtahMissionGenerator
    qCDebug(MissionServiceLog) << "GenCall22: Generating waypoints using PtahMissionGenerator";
    QList<QGeoCoordinate> waypoints;
    
    if (payloadDropMode) {
        // Use payload drop waypoint generation
        qCDebug(MissionServiceLog) << "Using payload drop waypoint generation";
        waypoints = m_ptahMissionGenerator->generatePayloadDropWaypoints(
            referencePoint, bearing, 3.0, static_cast<double>(areaSize), loiterTimeSeconds);
    } else {
        // Use standard waypoint generation
        qCDebug(MissionServiceLog) << "Using standard waypoint generation";
        waypoints = m_ptahMissionGenerator->generateCoordinatesInBothDirections(
            referencePoint, bearing, 3.0, static_cast<double>(areaSize));
    }
    
    qCDebug(MissionServiceLog) << "Generated" << waypoints.size() << "waypoints";
    
    // Log ALL waypoints for debugging
    for (int i = 0; i < waypoints.size(); i++) {
        qCDebug(MissionServiceLog) << "Waypoint" << i << ":" << waypoints[i].toString();
    }
    
    if (waypoints.isEmpty()) {
        qCWarning(MissionServiceLog) << "No waypoints generated! This will cause empty missions.";
        emit missionGenerationCompleted(false, "No waypoints generated");
        return;
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
    
    // Get all connected vehicles (reuse the multiVehicleManager from earlier)
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
        
        qCDebug(MissionServiceLog) << "=== NON ID 1 DRONE DISTRIBUTION ===";
        qCDebug(MissionServiceLog) << "Drone index:" << i << "Vehicle ID:" << vehicle->id() << "at altitude" << vehicleAltitude;
        qCDebug(MissionServiceLog) << "Waypoints assigned:" << vehicleWaypoints.size();
        qCDebug(MissionServiceLog) << "Mission sequence: TAKEOFF ->" << vehicleWaypoints.size() << "WAYPOINTS -> LAND";
        
        // Log first few waypoints for this drone
        int logCount = vehicleWaypoints.size() < 3 ? vehicleWaypoints.size() : 3;
        for (int j = 0; j < logCount; j++) {
            qCDebug(MissionServiceLog) << "  Waypoint" << j << "for Vehicle ID" << vehicle->id() << ":" << vehicleWaypoints[j].toString();
        }
        if (vehicleWaypoints.size() > 3) {
            qCDebug(MissionServiceLog) << "  ... and" << (vehicleWaypoints.size() - 3) << "more waypoints";
        }
        
        // Set altitude for all waypoints
        for (QGeoCoordinate& coord : vehicleWaypoints) {
            coord.setAltitude(vehicleAltitude);
        }
        
        // Upload complete mission (takeoff + waypoints + land) to this specific vehicle
        qCDebug(MissionServiceLog) << "Calling uploadMissionToVehicle for vehicle ID" << vehicle->id() << "with" << vehicleWaypoints.size() << "waypoints at altitude" << vehicleAltitude;
        
        if (payloadDropMode) {
            // Upload payload drop mission with servo commands and loiter
            qCDebug(MissionServiceLog) << "Uploading payload drop mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadPayloadDropMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds);
        } else {
            // Upload standard mission
            qCDebug(MissionServiceLog) << "Uploading standard mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude);
        }
    }
    
    // ID1 drone is now included in regular waypoint distribution - no special loiter mission needed
}

void MissionService::onMissionUploadCompleted(bool success, const QString& message)
{
    // GenCall47: MissionService::onMissionUploadCompleted() - Final completion callback
    qCDebug(MissionServiceLog) << "Mission upload completed:" << success << message;
    
    // GenCall48: Emit final mission generation completed signal
    emit missionGenerationCompleted(success, message);
}

void MissionService::generateMissionFromDrawnArea(const QList<QGeoCoordinate>& drawnCoordinates,
                                                 const QString& missionType,
                                                 int altitude,
                                                 double speed,
                                                 const QString& description,
                                                 double frontDistance,
                                                 bool payloadDropMode,
                                                 int loiterTimeSeconds,
                                                 int bendHeight,
                                                 double payloadDropHeight,
                                                 int servoDelaySeconds,
                                                 double observationDistance)
{
    // GenCall70: MissionService::generateMissionFromDrawnArea() - Generate mission from AreaPlanEditor drawn coordinates
    qCDebug(MissionServiceLog) << "GenCall70: MissionService::generateMissionFromDrawnArea() - Generating mission from drawn area coordinates";
    qCDebug(MissionServiceLog) << "Drawn coordinates count:" << drawnCoordinates.size();
    qCDebug(MissionServiceLog) << "Mission type:" << missionType << "Altitude:" << altitude << "Speed:" << speed;
    qCDebug(MissionServiceLog) << "Description:" << description;
    
    if (drawnCoordinates.isEmpty()) {
        qCWarning(MissionServiceLog) << "No drawn coordinates provided";
        emit missionGenerationCompleted(false, "No drawn area coordinates provided");
        return;
    }
    
    // Store current mission parameters
    m_currentMissionType = missionType;
    m_currentAltitude = altitude;
    m_currentSpeed = speed;
    m_currentDescription = description;
    
    // Calculate total tripods for demining operations
    if (payloadDropMode) {
        const double stepDistance = 4.5; // meters between bulbs and drones
        m_totalTripods = static_cast<int>(drawnCoordinates.size());
        m_installedTripods = 0;
        m_vehicleTripodCount.clear();
        m_explodeButtonEnabled = false;
        
        qCDebug(MissionServiceLog) << "Demining mode: Total tripods to install:" << m_totalTripods;
    } else {
        resetTripodTracking();
    }
    
    // Emit mission generation started signal
    emit missionGenerationStarted();
    
    // Get the active vehicle from MultiVehicleManager
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    Vehicle* activeVehicle = multiVehicleManager ? multiVehicleManager->activeVehicle() : nullptr;
    
    if (!activeVehicle) {
        qCWarning(MissionServiceLog) << "No active vehicle found";
        emit missionGenerationCompleted(false, "No active vehicle connected");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Active vehicle found:" << activeVehicle->id();
    
    // Use the drawn coordinates directly as waypoints
    QList<QGeoCoordinate> waypoints = drawnCoordinates;
    
    // Set altitude for all waypoints
    for (QGeoCoordinate& coord : waypoints) {
        coord.setAltitude(altitude);
    }
    
    qCDebug(MissionServiceLog) << "Using" << waypoints.size() << "drawn coordinates as waypoints";
    
    // Log waypoint pattern for debugging (2,3,4,2,3,4...)
    qCDebug(MissionServiceLog) << "Waypoint pattern: 2,3,4,2,3,4... (waypoint sequence numbers, all drones)";
    for (int i = 0; i < waypoints.size(); i++) {
        int waypointNumber = 2 + (i % 3); // Pattern: 2,3,4,2,3,4...
        qCDebug(MissionServiceLog) << "Waypoint" << (i + 1) << "(pattern#" << waypointNumber << "):" << waypoints[i].toString();
    }
    
    if (waypoints.isEmpty()) {
        qCWarning(MissionServiceLog) << "No waypoints generated from drawn coordinates";
        emit missionGenerationCompleted(false, "No waypoints generated from drawn coordinates");
        return;
    }
    
    // Emit waypoints generated signal
    emit waypointsGenerated(waypoints);
    
    // Distribute waypoints among NON ID 1 drones (ID 1 gets special loiter mission)
    QmlObjectListModel* vehiclesModel = multiVehicleManager->vehicles();
    QList<Vehicle*> nonId1Vehicles;
    
    // Include NON ID 1 vehicles in waypoint distribution
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() != 1) {
            nonId1Vehicles.append(vehicle);
        }
    }
    
    if (nonId1Vehicles.isEmpty()) {
        emit missionGenerationCompleted(false, "No non-ID1 vehicles found");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Found" << nonId1Vehicles.size() << "non-ID1 vehicles for waypoint distribution";
    
    // Distribute waypoints among NON ID 1 vehicles
    QList<QList<QGeoCoordinate>> distributedWaypoints = m_ptahMissionGenerator->distributeWaypointsAmongDrones(waypoints, nonId1Vehicles.size());
    
    // Upload missions with different altitudes for each NON ID 1 drone
    for (int i = 0; i < nonId1Vehicles.size() && i < distributedWaypoints.size(); i++) {
        Vehicle* vehicle = nonId1Vehicles[i];
        QList<QGeoCoordinate> vehicleWaypoints = distributedWaypoints[i];
        
        // Set different altitude for each drone to avoid collisions
        int vehicleAltitude = altitude + (i * 10); // 10m altitude difference per drone
        
        qCDebug(MissionServiceLog) << "=== NON ID 1 DRONE DISTRIBUTION ===";
        qCDebug(MissionServiceLog) << "Drone index:" << i << "Vehicle ID:" << vehicle->id() << "at altitude" << vehicleAltitude;
        qCDebug(MissionServiceLog) << "Waypoints assigned:" << vehicleWaypoints.size();
        
        // Set altitude for all waypoints
        for (QGeoCoordinate& coord : vehicleWaypoints) {
            coord.setAltitude(vehicleAltitude);
        }
        
        // Upload complete mission (takeoff + waypoints + land) to this specific vehicle
        if (payloadDropMode) {
            qCDebug(MissionServiceLog) << "Uploading payload drop mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadPayloadDropMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds);
        } else {
            qCDebug(MissionServiceLog) << "Uploading standard mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude);
        }
    }
    
    // Create special observation mission for drone ID 1
    Vehicle* droneId1 = nullptr;
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() == 1) {
            droneId1 = vehicle;
            break;
        }
    }
    
    if (droneId1) {
        // Calculate mission center for yaw control
        QGeoCoordinate missionCenter = m_ptahMissionGenerator->calculateMiddlePoint(waypoints);
        
        if (missionCenter.isValid() && !waypoints.isEmpty()) {
            qCDebug(MissionServiceLog) << "Drone ID 1 mission center:" << missionCenter.toString();
            qCDebug(MissionServiceLog) << "Drone ID 1 observation distance:" << observationDistance << "meters";
            
            // Calculate observation position at configurable distance from mission center
            QGeoCoordinate observationPosition = m_ptahMissionGenerator->calculateObservationPosition(missionCenter, observationDistance, altitude);
            
            if (observationPosition.isValid()) {
                qCDebug(MissionServiceLog) << "Drone ID 1 observation position:" << observationPosition.toString();
                m_uploadService->uploadLoiterMissionToVehicle(droneId1, observationPosition, altitude + 50, missionCenter);
            } else {
                qCWarning(MissionServiceLog) << "Could not calculate valid observation position for drone ID 1";
            }
        } else {
            qCWarning(MissionServiceLog) << "Could not calculate mission center or no waypoints available";
        }
    } else {
        qCDebug(MissionServiceLog) << "Drone ID 1 not found - skipping observation mission";
    }
}

void MissionService::processMissionGeneration(const QString& missionType, 
                                            int areaSize, 
                                            int altitude, 
                                            double speed, 
                                            const QString& description,
                                            double frontDistance)
{
    // This method is kept for compatibility but delegates to generateMission
    generateMission(missionType, areaSize, altitude, speed, description, frontDistance);
}

// Tripod tracking methods for demining operations
void MissionService::reportTripodInstalled(int vehicleId)
{
    qCDebug(MissionServiceLog) << "GenCall60: Tripod installed by vehicle" << vehicleId;
    
    // Increment tripod count for this vehicle
    m_vehicleTripodCount[vehicleId]++;
    m_installedTripods++;
    
    qCDebug(MissionServiceLog) << "Vehicle" << vehicleId << "tripod count:" << m_vehicleTripodCount[vehicleId];
    qCDebug(MissionServiceLog) << "Total installed tripods:" << m_installedTripods << "of" << m_totalTripods;
    
    // Emit tripod installed signal
    emit tripodInstalled(vehicleId, m_vehicleTripodCount[vehicleId], m_totalTripods);
    
    // Check if all tripods are installed
    if (m_installedTripods >= m_totalTripods) {
        qCDebug(MissionServiceLog) << "All tripods installed - enabling explode button";
        m_explodeButtonEnabled = true;
        emit allTripodsInstalled();
        emit explodeButtonEnabled(true);
    }
}

bool MissionService::isExplodeButtonEnabled() const
{
    // Always enable explode button when Vehicle ID 1 is active
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    Vehicle* activeVehicle = multiVehicleManager ? multiVehicleManager->activeVehicle() : nullptr;
    
    if (activeVehicle && activeVehicle->id() == 1) {
        return true;
    }
    
    return false;
}

void MissionService::executeExplode()
{
    qCDebug(MissionServiceLog) << "GenCall61: Executing explode command";
    
    // Get active vehicle
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    Vehicle* activeVehicle = multiVehicleManager ? multiVehicleManager->activeVehicle() : nullptr;
    
    if (activeVehicle && activeVehicle->id() == 1) {
        // Send relay HIGH command directly
        activeVehicle->sendCommand(
            1, // Component ID
            181, // MAV_CMD_DO_SET_RELAY
            true, // showError
            0, // param1: Relay number (0 for Relay1)
            1.0, // param2: Value (1=HIGH)
            0, 0, 0, 0, 0 // param3-7: unused
        );
        
        qCDebug(MissionServiceLog) << "Relay HIGH command sent for Vehicle ID 1";
    } else {
        qCWarning(MissionServiceLog) << "Explode command only works with Vehicle ID 1";
    }
    
    // Emit demining success signal
    emit deminingSuccess();
    
    qCDebug(MissionServiceLog) << "Explode command completed";
}

void MissionService::resetTripodTracking()
{
    qCDebug(MissionServiceLog) << "GenCall62: Resetting tripod tracking";
    
    m_totalTripods = 0;
    m_installedTripods = 0;
    m_vehicleTripodCount.clear();
    m_explodeButtonEnabled = false;
    
    emit explodeButtonEnabled(false);
    
    qCDebug(MissionServiceLog) << "Tripod tracking reset";
}

// Test method for development/debugging
void MissionService::testMarkAllPayloadsInstalled()
{
    qCDebug(MissionServiceLog) << "GenCall70: TEST - Marking all payloads as installed";
    
    // Get all connected vehicles
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        qCWarning(MissionServiceLog) << "MultiVehicleManager not available for test";
        return;
    }
    
    QmlObjectListModel* vehiclesModel = multiVehicleManager->vehicles();
    if (!vehiclesModel) {
        qCWarning(MissionServiceLog) << "Vehicles model not available for test";
        return;
    }
    
    qCDebug(MissionServiceLog) << "Found" << vehiclesModel->count() << "vehicles for test";
    
    // Mark tripod installed for each vehicle (except ID 1)
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() != 1) { // Skip vehicle ID 1 (observation drone)
            qCDebug(MissionServiceLog) << "TEST - Marking tripod installed for vehicle" << vehicle->id();
            reportTripodInstalled(vehicle->id());
        }
    }
    
    qCDebug(MissionServiceLog) << "TEST - All payloads marked as installed";
}

// Water avoidance settings (read from global AppSettings)
bool MissionService::waterAvoidanceEnabled() const
{
    return SettingsManager::instance()->appSettings()->waterAvoidanceEnabled()->rawValue().toBool();
}

void MissionService::generateSpecialMissionForVehicle1(const QGeoCoordinate& areaCenter,
                                                     int altitude,
                                                     double speed,
                                                     const QString& description,
                                                     int loiterTimeSeconds,
                                                     double observationDistance)
{
    qCDebug(MissionServiceLog) << "GenCall1: generateSpecialMissionForVehicle1() - Generating special observation mission for Vehicle ID 1";
    
    // Emit mission generation started signal
    emit missionGenerationStarted();
    
    // Find Vehicle ID 1
    Vehicle* vehicle1 = nullptr;
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    if (multiVehicleManager) {
        QmlObjectListModel* vehiclesModel = multiVehicleManager->vehicles();
        for (int i = 0; i < vehiclesModel->count(); i++) {
            Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
            if (vehicle && vehicle->id() == 1) {
                vehicle1 = vehicle;
                break;
            }
        }
    }
    
    if (!vehicle1) {
        qCWarning(MissionServiceLog) << "GenCall2: Vehicle ID 1 not found";
        emit missionGenerationCompleted(false, "Vehicle ID 1 not found");
        return;
    }
    
    qCDebug(MissionServiceLog) << "GenCall3: Found Vehicle ID 1, generating special observation mission";
    
    // Calculate safe observation position (offset from area center)
    QGeoCoordinate observationPosition = areaCenter;
    observationPosition.setAltitude(altitude + 20); // 20m higher than other vehicles for safety
    
    // Create special mission for Vehicle ID 1: Takeoff -> Loiter -> Land
    QList<QGeoCoordinate> specialWaypoints;
    
    // 1. Takeoff position (same as observation position)
    specialWaypoints.append(observationPosition);
    
    // 2. Loiter position (slightly offset for observation)
    QGeoCoordinate loiterPosition = observationPosition;
    loiterPosition.setLatitude(loiterPosition.latitude() + 0.0001); // Small offset
    specialWaypoints.append(loiterPosition);
    
    // 3. Land position (back to takeoff position)
    specialWaypoints.append(observationPosition);
    
    qCDebug(MissionServiceLog) << "GenCall4: Generated special mission waypoints for Vehicle ID 1:";
    for (int i = 0; i < specialWaypoints.size(); i++) {
        qCDebug(MissionServiceLog) << "  Waypoint" << (i+1) << ":" << specialWaypoints[i].toString();
    }
    
    // Upload special mission to Vehicle ID 1
    if (m_uploadService) {
        qCDebug(MissionServiceLog) << "GenCall5: Uploading special mission to Vehicle ID 1";
        
        // Upload special loiter mission for observation
        m_uploadService->uploadLoiterMissionToVehicle(vehicle1, observationPosition, 
                                                     altitude + 20, areaCenter);
        
        qCDebug(MissionServiceLog) << "GenCall6: Special loiter mission uploaded to Vehicle ID 1";
        emit missionGenerationCompleted(true, "Special observation mission generated for Vehicle ID 1");
    } else {
        qCWarning(MissionServiceLog) << "GenCall8: Mission upload service not available";
        emit missionGenerationCompleted(false, "Mission upload service not available");
    }
}

void MissionService::generateMissionFromDrawnAreaWithPreDistribution(const QList<QGeoCoordinate>& drawnCoordinates,
                                                                   const QString& missionType,
                                                                   int altitude,
                                                                   double speed,
                                                                   const QString& description,
                                                                   double frontDistance,
                                                                   bool payloadDropMode,
                                                                   int loiterTimeSeconds,
                                                                   int bendHeight,
                                                                   double payloadDropHeight,
                                                                   int servoDelaySeconds,
                                                                   double observationDistance)
{
    qCDebug(MissionServiceLog) << "GenCall1: generateMissionFromDrawnAreaWithPreDistribution() - Using pre-distributed waypoints";
    
    // Emit mission generation started signal
    emit missionGenerationStarted();
    
    // Copy waypoints and set altitude
    QList<QGeoCoordinate> waypoints = drawnCoordinates;
    for (QGeoCoordinate& coord : waypoints) {
        coord.setAltitude(altitude);
    }
    
    qCDebug(MissionServiceLog) << "Using" << waypoints.size() << "pre-distributed waypoints (NO redistribution)";
    
    // Log waypoint pattern for debugging (2,3,4,2,3,4...)
    qCDebug(MissionServiceLog) << "Pre-distributed waypoint pattern: 2,3,4,2,3,4... (waypoint sequence numbers, respecting existing distribution)";
    for (int i = 0; i < waypoints.size(); i++) {
        int waypointNumber = 2 + (i % 3); // Pattern: 2,3,4,2,3,4...
        qCDebug(MissionServiceLog) << "Pre-distributed waypoint" << (i + 1) << "(pattern#" << waypointNumber << "):" << waypoints[i].toString();
    }
    
    if (waypoints.isEmpty()) {
        qCWarning(MissionServiceLog) << "No pre-distributed waypoints provided";
        emit missionGenerationCompleted(false, "No pre-distributed waypoints provided");
        return;
    }
    
    // Get available vehicles
    MultiVehicleManager* multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        qCWarning(MissionServiceLog) << "MultiVehicleManager not available";
        emit missionGenerationCompleted(false, "MultiVehicleManager not available");
        return;
    }
    
    QmlObjectListModel* vehiclesModel = multiVehicleManager->vehicles();
    QList<Vehicle*> nonId1Vehicles;
    
    // Include NON ID 1 vehicles in waypoint distribution (ID 1 gets special loiter mission)
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() != 1) {
            nonId1Vehicles.append(vehicle);
        }
    }
    
    if (nonId1Vehicles.isEmpty()) {
        emit missionGenerationCompleted(false, "No non-ID1 vehicles found");
        return;
    }
    
    qCDebug(MissionServiceLog) << "Found" << nonId1Vehicles.size() << "non-ID1 vehicles for pre-distributed mission";
    
    // IMPORTANT: Do NOT redistribute waypoints - use them as-is with pre-distribution
    // The waypoints are already assigned to specific drones in the 1,2,1,2,1,2... pattern (user-facing)
    
    // Upload missions with different altitudes for each NON ID 1 drone
    // We'll distribute waypoints evenly among available NON ID 1 vehicles while respecting the pattern
    int waypointsPerVehicle = waypoints.size() / nonId1Vehicles.size();
    int remainingWaypoints = waypoints.size() % nonId1Vehicles.size();
    
    qCDebug(MissionServiceLog) << "Pre-distribution: Each NON ID 1 vehicle gets" << waypointsPerVehicle << "waypoints, with" << remainingWaypoints << "extra";
    
    int waypointIndex = 0;
    for (int i = 0; i < nonId1Vehicles.size(); i++) {
        Vehicle* vehicle = nonId1Vehicles[i];
        
        // Calculate how many waypoints this vehicle gets
        int vehicleWaypointCount = waypointsPerVehicle;
        if (i < remainingWaypoints) {
            vehicleWaypointCount++; // Give extra waypoints to first few vehicles
        }
        
        // Extract waypoints for this vehicle
        QList<QGeoCoordinate> vehicleWaypoints;
        for (int j = 0; j < vehicleWaypointCount && waypointIndex < waypoints.size(); j++) {
            vehicleWaypoints.append(waypoints[waypointIndex]);
            waypointIndex++;
        }
        
        // Set different altitude for each drone to avoid collisions
        int vehicleAltitude = altitude + (i * 10); // 10m altitude difference per drone
        
        qCDebug(MissionServiceLog) << "=== PRE-DISTRIBUTED NON ID 1 MISSION ===";
        qCDebug(MissionServiceLog) << "Vehicle ID:" << vehicle->id() << "at altitude" << vehicleAltitude;
        qCDebug(MissionServiceLog) << "Pre-distributed waypoints assigned:" << vehicleWaypoints.size();
        
        // Set altitude for all waypoints
        for (QGeoCoordinate& coord : vehicleWaypoints) {
            coord.setAltitude(vehicleAltitude);
        }
        
        // Upload complete mission (takeoff + waypoints + land) to this specific vehicle
        if (payloadDropMode) {
            qCDebug(MissionServiceLog) << "Uploading pre-distributed payload drop mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadPayloadDropMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude, loiterTimeSeconds, bendHeight, payloadDropHeight, servoDelaySeconds);
        } else {
            qCDebug(MissionServiceLog) << "Uploading pre-distributed standard mission for vehicle ID" << vehicle->id();
            m_uploadService->uploadMissionToVehicle(vehicle, vehicleWaypoints, vehicleAltitude);
        }
    }
    
    // Create special observation mission for drone ID 1
    Vehicle* droneId1 = nullptr;
    for (int i = 0; i < vehiclesModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehiclesModel->get(i));
        if (vehicle && vehicle->id() == 1) {
            droneId1 = vehicle;
            break;
        }
    }
    
    if (droneId1) {
        // Calculate mission center for yaw control
        QGeoCoordinate missionCenter = m_ptahMissionGenerator->calculateMiddlePoint(waypoints);
        
        if (missionCenter.isValid() && !waypoints.isEmpty()) {
            qCDebug(MissionServiceLog) << "Drone ID 1 mission center:" << missionCenter.toString();
            qCDebug(MissionServiceLog) << "Drone ID 1 observation distance:" << observationDistance << "meters";
            
            // Calculate observation position at configurable distance from mission center
            QGeoCoordinate observationPosition = m_ptahMissionGenerator->calculateObservationPosition(missionCenter, observationDistance, altitude);
            
            if (observationPosition.isValid()) {
                qCDebug(MissionServiceLog) << "Drone ID 1 observation position:" << observationPosition.toString();
                m_uploadService->uploadLoiterMissionToVehicle(droneId1, observationPosition, altitude + 50, missionCenter);
            } else {
                qCWarning(MissionServiceLog) << "Could not calculate valid observation position for drone ID 1";
            }
        } else {
            qCWarning(MissionServiceLog) << "Could not calculate mission center or no waypoints available";
        }
    } else {
        qCWarning(MissionServiceLog) << "Drone ID 1 not found for special observation mission";
    }
    
    qCDebug(MissionServiceLog) << "Pre-distributed mission generation completed with" << waypoints.size() << "waypoints";
    emit missionGenerationCompleted(true, "Pre-distributed mission generated successfully");
}
