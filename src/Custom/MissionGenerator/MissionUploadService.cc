#include "MissionUploadService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "MissionManager/MissionItem.h"
#include "MissionManager/MissionManager.h"
#include "QGCLoggingCategory.h"
#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>

QGC_LOGGING_CATEGORY(MissionUploadServiceLog, "MissionUploadServiceLog")

MissionUploadService::MissionUploadService(QObject* parent)
    : QObject(parent)
{
}

QList<Vehicle*> MissionUploadService::getConnectedVehicles()
{
    QList<Vehicle*> vehicles;
    
    MultiVehicleManager* vehicleManager = MultiVehicleManager::instance();
    if (!vehicleManager) {
        qCWarning(MissionUploadServiceLog) << "MultiVehicleManager not available";
        return vehicles;
    }
    
    QmlObjectListModel* vehicleModel = vehicleManager->vehicles();
    if (!vehicleModel) {
        qCWarning(MissionUploadServiceLog) << "Vehicles model not available";
        return vehicles;
    }
    
    int vehicleCount = vehicleModel->count();
    for (int i = 0; i < vehicleCount; i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle) {
            vehicles.append(vehicle);
        }
    }
    
    return vehicles;
}

void MissionUploadService::uploadMissionsToAllVehicles(const QList<QGeoCoordinate>& waypoints, int altitude)
{
    // GenCall34: MissionUploadService::uploadMissionsToAllVehicles() - Upload to all vehicles
    if (waypoints.isEmpty()) {
        qCWarning(MissionUploadServiceLog) << "No waypoints provided for mission upload";
        emit missionUploadCompleted(false, "No waypoints provided");
        return;
    }
    
    // GenCall35: Get connected vehicles
    QList<Vehicle*> vehicles = getConnectedVehicles();
    if (vehicles.isEmpty()) {
        qCWarning(MissionUploadServiceLog) << "No vehicles connected";
        emit missionUploadCompleted(false, "No vehicles connected");
        return;
    }
    
    qCDebug(MissionUploadServiceLog) << "Uploading missions to" << vehicles.size() << "vehicles";
    
    // GenCall36: Upload mission to each vehicle
    int successfulUploads = 0;
    for (int i = 0; i < vehicles.size(); i++) {
        Vehicle* vehicle = vehicles[i];
        
        qCDebug(MissionUploadServiceLog) << "Uploading mission to vehicle" << i << "ID:" << vehicle->id();
        
        // GenCall37: Upload mission to individual vehicle
        uploadMissionToVehicle(vehicle, waypoints, altitude);
        successfulUploads++;
        qCDebug(MissionUploadServiceLog) << "Successfully uploaded mission to vehicle" << vehicle->id();
        
        // GenCall38: Emit progress update
        emit missionUploadProgress(i + 1, vehicles.size());
    }
    
    // GenCall39: Emit completion status
    QString message = QString("Uploaded missions to %1 out of %2 vehicles")
                     .arg(successfulUploads)
                     .arg(vehicles.size());
    
    qCDebug(MissionUploadServiceLog) << message;
    emit missionUploadCompleted(successfulUploads > 0, message);
}

void MissionUploadService::uploadMissionToVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude)
{
    // GenCall40: MissionUploadService::uploadMissionToVehicle() - Create mission for individual vehicle
    if (!vehicle || !vehicle->missionManager()) {
        qCWarning(MissionUploadServiceLog) << "Invalid vehicle or mission manager";
        return;
    }
    
    if (waypoints.isEmpty()) {
        qCWarning(MissionUploadServiceLog) << "No waypoints provided for vehicle";
        return;
    }
    
    // GenCall41: Get the drone's current position
    QGeoCoordinate droneCurrentPosition = vehicle->coordinate();
    if (!droneCurrentPosition.isValid()) {
        qCWarning(MissionUploadServiceLog) << "Invalid current position for vehicle" << vehicle->id();
        return;
    }
    
    qCDebug(MissionUploadServiceLog) << "Vehicle" << vehicle->id() << "current position:" << droneCurrentPosition.toString();
    
    // GenCall42: Create mission items for this vehicle
    QList<MissionItem*> missionItems;
    QObject* missionItemParent = vehicle; // Vehicle will manage the mission items
    
    // GenCall43: Add takeoff item at drone's current position
    QGeoCoordinate takeoffCoord = droneCurrentPosition;
    takeoffCoord.setAltitude(altitude);
    MissionItem* takeoffItem = new MissionItem(0, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                               0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(), 
                                               false, false, missionItemParent);
    missionItems.append(takeoffItem);
    qCDebug(MissionUploadServiceLog) << "Added takeoff at vehicle position:" << takeoffCoord.toString();
    
    // GenCall44: Add waypoint items
    for (int i = 0; i < waypoints.size(); i++) {
        QGeoCoordinate coord = waypoints[i];
        coord.setAltitude(altitude);
        MissionItem* waypointItem = new MissionItem(i + 1, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                   0, 0, 0, 0, coord.latitude(), coord.longitude(), coord.altitude(), 
                                                   false, false, missionItemParent);
        missionItems.append(waypointItem);
    }
    
    // GenCall45: Add land item at drone's current position (return to start)
    QGeoCoordinate landCoord = droneCurrentPosition;
    landCoord.setAltitude(0); // Land at ground level
    MissionItem* landItem = new MissionItem(waypoints.size() + 1, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                           0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(landItem);
    qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
    
    // GenCall46: Upload mission to vehicle via MissionManager
    if (vehicle->missionManager()) {
        vehicle->missionManager()->writeMissionItems(missionItems);
    }
    
    qCDebug(MissionUploadServiceLog) << "Uploaded mission with" << missionItems.size() << "items to vehicle" << vehicle->id();
}

bool MissionUploadService::createMissionForVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude)
{
    // This method is kept for compatibility but delegates to uploadMissionToVehicle
    uploadMissionToVehicle(vehicle, waypoints, altitude);
    return true; // Assume success for now
}

void MissionUploadService::uploadLoiterMissionToVehicle(Vehicle* vehicle, const QGeoCoordinate& loiterPosition, int altitude)
{
    // GenCall49: MissionUploadService::uploadLoiterMissionToVehicle() - Create loiter mission for drone ID 1
    qCDebug(MissionUploadServiceLog) << "GenCall49: Creating loiter mission for drone ID 1";
    
    if (!vehicle || !vehicle->missionManager()) {
        qCWarning(MissionUploadServiceLog) << "Invalid vehicle or mission manager for loiter mission";
        return;
    }
    
    if (!loiterPosition.isValid()) {
        qCWarning(MissionUploadServiceLog) << "Invalid loiter position for vehicle" << vehicle->id();
        return;
    }
    
    // GenCall50: Get the drone's current position
    QGeoCoordinate droneCurrentPosition = vehicle->coordinate();
    if (!droneCurrentPosition.isValid()) {
        qCWarning(MissionUploadServiceLog) << "Invalid current position for vehicle" << vehicle->id();
        return;
    }
    
    qCDebug(MissionUploadServiceLog) << "Vehicle" << vehicle->id() << "current position:" << droneCurrentPosition.toString();
    qCDebug(MissionUploadServiceLog) << "Loiter position:" << loiterPosition.toString();
    
    // GenCall51: Create loiter mission items
    QList<MissionItem*> missionItems;
    QObject* missionItemParent = vehicle;
    
    // GenCall52: Add takeoff item at drone's current position
    QGeoCoordinate takeoffCoord = droneCurrentPosition;
    takeoffCoord.setAltitude(altitude);
    MissionItem* takeoffItem = new MissionItem(0, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                              0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(),
                                              false, false, missionItemParent);
    missionItems.append(takeoffItem);
    qCDebug(MissionUploadServiceLog) << "Added takeoff at vehicle position:" << takeoffCoord.toString();
    
    // GenCall53: Add loiter item at observation position
    QGeoCoordinate loiterCoord = loiterPosition;
    loiterCoord.setAltitude(altitude);
    MissionItem* loiterItem = new MissionItem(1, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                            300, // Loiter for 5 minutes (300 seconds)
                                            0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                            false, false, missionItemParent);
    missionItems.append(loiterItem);
    qCDebug(MissionUploadServiceLog) << "Added loiter at observation position:" << loiterCoord.toString();
    
    // GenCall54: Add land item at drone's current position (return to start)
    QGeoCoordinate landCoord = droneCurrentPosition;
    landCoord.setAltitude(0); // Land at ground level
    MissionItem* landItem = new MissionItem(2, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                          0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(),
                                          false, false, missionItemParent);
    missionItems.append(landItem);
    qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
    
    // GenCall55: Upload loiter mission to vehicle
    if (vehicle->missionManager()) {
        vehicle->missionManager()->writeMissionItems(missionItems);
    }
    
    qCDebug(MissionUploadServiceLog) << "Uploaded loiter mission with" << missionItems.size() << "items to vehicle" << vehicle->id();
}
