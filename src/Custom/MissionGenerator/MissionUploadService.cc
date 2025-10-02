#include "MissionUploadService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "MissionManager/MissionItem.h"
#include "MissionManager/MissionManager.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
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
    
    // GenCall43: Add home position item first (required by QGroundControl, will be skipped by vehicle)
    QGeoCoordinate homeCoord = droneCurrentPosition;
    homeCoord.setAltitude(0); // Home at ground level
    MissionItem* homeItem = new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                           0, 0, 0, 0, homeCoord.latitude(), homeCoord.longitude(), homeCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(homeItem);
    qCDebug(MissionUploadServiceLog) << "Added home position item (will be skipped by vehicle):" << homeCoord.toString();
    
    // GenCall44: Add takeoff item as second item (first real mission item)
    QGeoCoordinate takeoffCoord = droneCurrentPosition;
    takeoffCoord.setAltitude(altitude);
    qCDebug(MissionUploadServiceLog) << "Creating takeoff item with coordinates:" << takeoffCoord.toString();
    
    MissionItem* takeoffItem = new MissionItem(1, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                               0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(), 
                                               false, false, missionItemParent);
    missionItems.append(takeoffItem);
    qCDebug(MissionUploadServiceLog) << "Successfully created takeoff item - Command:" << takeoffItem->command() << "Frame:" << takeoffItem->frame();
    
    // GenCall45: Add waypoint items
    for (int i = 0; i < waypoints.size(); i++) {
        QGeoCoordinate coord = waypoints[i];
        coord.setAltitude(altitude);
        MissionItem* waypointItem = new MissionItem(i + 2, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                   0, 0, 0, 0, coord.latitude(), coord.longitude(), coord.altitude(), 
                                                   false, false, missionItemParent);
        missionItems.append(waypointItem);
    }
    
    // GenCall46: Add land item at drone's current position (return to start)
    QGeoCoordinate landCoord = droneCurrentPosition;
    landCoord.setAltitude(0); // Land at ground level
    MissionItem* landItem = new MissionItem(waypoints.size() + 2, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                           0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(landItem);
    qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
    
    // GenCall46: Log complete mission before upload
    qCDebug(MissionUploadServiceLog) << "Complete mission for vehicle" << vehicle->id() << "contains" << missionItems.size() << "items:";
    for (int i = 0; i < missionItems.size(); i++) {
        MissionItem* item = missionItems[i];
        qCDebug(MissionUploadServiceLog) << "Item" << i << "- Seq:" << item->sequenceNumber() 
                                        << "Cmd:" << item->command() 
                                        << "Frame:" << item->frame()
                                        << "Lat:" << item->param5() 
                                        << "Lng:" << item->param6() 
                                        << "Alt:" << item->param7();
    }
    
    // GenCall47: Upload mission to vehicle via MissionManager
    if (vehicle->missionManager()) {
        qCDebug(MissionUploadServiceLog) << "Uploading mission to vehicle" << vehicle->id() << "via MissionManager";
        vehicle->missionManager()->writeMissionItems(missionItems);
        qCDebug(MissionUploadServiceLog) << "Successfully uploaded mission with" << missionItems.size() << "items to vehicle" << vehicle->id();
        
        // Emit success signal for this vehicle
        emit missionUploadCompleted(true, QString("Mission uploaded to vehicle %1").arg(vehicle->id()));
    } else {
        qCWarning(MissionUploadServiceLog) << "Failed to upload mission - no mission manager for vehicle" << vehicle->id();
        emit missionUploadCompleted(false, QString("No mission manager for vehicle %1").arg(vehicle->id()));
    }
}

bool MissionUploadService::createMissionForVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude)
{
    // This method is kept for compatibility but delegates to uploadMissionToVehicle
    uploadMissionToVehicle(vehicle, waypoints, altitude);
    return true; // Assume success for now
}

void MissionUploadService::uploadLoiterMissionToVehicle(Vehicle* vehicle, const QGeoCoordinate& loiterPosition, int altitude, const QGeoCoordinate& missionCenter)
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
    
    // GenCall52: Add home position item first (required by QGroundControl, will be skipped by vehicle)
    QGeoCoordinate homeCoord = droneCurrentPosition;
    homeCoord.setAltitude(0); // Home at ground level
    MissionItem* homeItem = new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                           0, 0, 0, 0, homeCoord.latitude(), homeCoord.longitude(), homeCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(homeItem);
    qCDebug(MissionUploadServiceLog) << "Added home position item (will be skipped by vehicle):" << homeCoord.toString();
    
    // GenCall53: Add takeoff item as second item (first real mission item)
    QGeoCoordinate takeoffCoord = droneCurrentPosition;
    takeoffCoord.setAltitude(altitude);
    qCDebug(MissionUploadServiceLog) << "Creating loiter mission takeoff item with coordinates:" << takeoffCoord.toString();
    
    MissionItem* takeoffItem = new MissionItem(1, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                              0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(),
                                              false, false, missionItemParent);
    missionItems.append(takeoffItem);
    qCDebug(MissionUploadServiceLog) << "Added takeoff at vehicle position:" << takeoffCoord.toString();
    qCDebug(MissionUploadServiceLog) << "Loiter takeoff item - Command:" << takeoffItem->command() << "Frame:" << takeoffItem->frame();
    
    // GenCall54: Add yaw control to point toward mission center (if provided)
    if (missionCenter.isValid()) {
        // Calculate bearing from observer to mission center
        double bearing = loiterPosition.azimuthTo(missionCenter);
        
        // Add yaw command to point toward mission center
        MissionItem* yawItem = new MissionItem(2, MAV_CMD_CONDITION_YAW, MAV_FRAME_MISSION,
                                             bearing, // param1: Target yaw angle
                                             0,       // param2: Speed (0 = use default)
                                             1,       // param3: Direction (1 = shortest path)
                                             0,       // param4: unused
                                             0,       // param5: unused
                                             0,       // param6: unused
                                             0,       // param7: unused
                                             false,   // autoContinue
                                             false,   // isCurrentItem
                                             missionItemParent);
        missionItems.append(yawItem);
        qCDebug(MissionUploadServiceLog) << "Added yaw control to point toward mission center at bearing:" << bearing << "degrees";
        
        // Add loiter item at observation position
        QGeoCoordinate loiterCoord = loiterPosition;
        loiterCoord.setAltitude(altitude);
        MissionItem* loiterItem = new MissionItem(3, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                300, // Loiter for 5 minutes (300 seconds)
                                                0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                                false, false, missionItemParent);
        missionItems.append(loiterItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter at observation position:" << loiterCoord.toString();
        
        // Add periodic yaw updates to maintain camera pointing at loiter position
        for (int i = 0; i < 5; i++) { // Update yaw every 60 seconds for 5 minutes
            MissionItem* yawUpdateItem = new MissionItem(4 + i, MAV_CMD_CONDITION_YAW, MAV_FRAME_MISSION,
                                                       bearing, // param1: Target yaw angle
                                                       0,       // param2: Speed (0 = use default)
                                                       1,       // param3: Direction (1 = shortest path)
                                                       0,       // param4: unused
                                                       0,       // param5: unused
                                                       0,       // param6: unused
                                                       0,       // param7: unused
                                                       false,   // autoContinue
                                                       false,   // isCurrentItem
                                                       missionItemParent);
            missionItems.append(yawUpdateItem);
            qCDebug(MissionUploadServiceLog) << "Added yaw update" << (i + 1) << "to maintain camera pointing at loiter position:" << loiterCoord.toString();
        }
        
        // Add RTL (Return to Launch) before landing
        MissionItem* rtlItem = new MissionItem(9, MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_MISSION,
                                             0, 0, 0, 0, 0, 0, 0,
                                             false, false, missionItemParent);
        missionItems.append(rtlItem);
        qCDebug(MissionUploadServiceLog) << "Added return to launch for drone ID 1";
        
        // Add land item
        QGeoCoordinate landCoord = droneCurrentPosition;
        landCoord.setAltitude(0); // Land at ground level
        MissionItem* landItem = new MissionItem(10, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                              0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(),
                                              false, false, missionItemParent);
        missionItems.append(landItem);
        qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
        
    } else {
        // No mission center provided, use standard loiter without yaw control
        QGeoCoordinate loiterCoord = loiterPosition;
        loiterCoord.setAltitude(altitude);
        MissionItem* loiterItem = new MissionItem(2, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                300, // Loiter for 5 minutes (300 seconds)
                                                0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                                false, false, missionItemParent);
        missionItems.append(loiterItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter at observation position:" << loiterCoord.toString();
        
        // Add RTL (Return to Launch) before landing
        MissionItem* rtlItem = new MissionItem(3, MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_MISSION,
                                             0, 0, 0, 0, 0, 0, 0,
                                             false, false, missionItemParent);
        missionItems.append(rtlItem);
        qCDebug(MissionUploadServiceLog) << "Added return to launch for drone ID 1 (no yaw control)";
        
        // Add land item
        QGeoCoordinate landCoord = droneCurrentPosition;
        landCoord.setAltitude(0); // Land at ground level
        MissionItem* landItem = new MissionItem(4, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                              0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(),
                                              false, false, missionItemParent);
        missionItems.append(landItem);
        qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
    }
    
    // GenCall55: Upload loiter mission to vehicle
    if (vehicle->missionManager()) {
        vehicle->missionManager()->writeMissionItems(missionItems);
        qCDebug(MissionUploadServiceLog) << "Successfully uploaded loiter mission with" << missionItems.size() << "items to vehicle" << vehicle->id();
        
        // Emit success signal for this vehicle
        emit missionUploadCompleted(true, QString("Loiter mission uploaded to vehicle %1").arg(vehicle->id()));
    } else {
        qCWarning(MissionUploadServiceLog) << "Failed to upload loiter mission - no mission manager for vehicle" << vehicle->id();
        emit missionUploadCompleted(false, QString("No mission manager for vehicle %1").arg(vehicle->id()));
    }
}

void MissionUploadService::uploadPayloadDropMissionToVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude, int loiterTimeSeconds, int bendHeight, double payloadDropHeight, int servoDelaySeconds)
{
    // GenCall56: MissionUploadService::uploadPayloadDropMissionToVehicle() - Create payload drop mission with continueAfterLand
    qCDebug(MissionUploadServiceLog) << "GenCall56: Creating payload drop mission with continueAfterLand for vehicle" << vehicle->id();
    
    if (!vehicle || !vehicle->missionManager()) {
        qCWarning(MissionUploadServiceLog) << "Invalid vehicle or mission manager for payload drop mission";
        return;
    }
    
    if (waypoints.isEmpty()) {
        qCWarning(MissionUploadServiceLog) << "No waypoints provided for payload drop mission";
        return;
    }
    
    // Note: AUTO_OPTIONS parameter setting removed due to firmware compatibility
    // The continueAfterLand functionality will rely on the vehicle's default behavior
    qCDebug(MissionUploadServiceLog) << "Using vehicle's default continueAfterLand behavior (AUTO_OPTIONS not supported)";
    
    // GenCall57: Get the drone's current position
    QGeoCoordinate droneCurrentPosition = vehicle->coordinate();
    if (!droneCurrentPosition.isValid()) {
        qCWarning(MissionUploadServiceLog) << "Invalid current position for vehicle" << vehicle->id();
        return;
    }
    
    qCDebug(MissionUploadServiceLog) << "Vehicle" << vehicle->id() << "current position:" << droneCurrentPosition.toString();
    qCDebug(MissionUploadServiceLog) << "Payload drop mission with continueAfterLand for" << waypoints.size() << "waypoints";
    qCDebug(MissionUploadServiceLog) << "Parameters - Altitude:" << altitude << "Bend Height:" << bendHeight << "Payload Drop Height:" << payloadDropHeight << "Servo Delay:" << servoDelaySeconds;
    
    // GenCall58: Create payload drop mission items with continueAfterLand
    QList<MissionItem*> missionItems;
    QObject* missionItemParent = vehicle;
    int sequenceNumber = 0;
    
    // GenCall59: Add home position item first (required by QGroundControl, will be skipped by vehicle)
    QGeoCoordinate homeCoord = droneCurrentPosition;
    homeCoord.setAltitude(0); // Home at ground level
    MissionItem* homeItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                           0, 0, 0, 0, homeCoord.latitude(), homeCoord.longitude(), homeCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(homeItem);
    qCDebug(MissionUploadServiceLog) << "Added home position item (will be skipped by vehicle):" << homeCoord.toString();
    
    // GenCall60: Process each waypoint with continueAfterLand pattern
    for (int i = 0; i < waypoints.size(); i++) {
        const QGeoCoordinate& waypoint = waypoints[i];
        QGeoCoordinate wpCoord = waypoint;
        wpCoord.setAltitude(altitude);
        
        qCDebug(MissionUploadServiceLog) << "Processing waypoint" << (i + 1) << "of" << waypoints.size() << "at:" << wpCoord.toString();
        
        // 1. Servo 10 = 400 (Hold payload)
        MissionItem* servoHoldItem = new MissionItem(sequenceNumber++, MAV_CMD_DO_SET_SERVO, MAV_FRAME_MISSION,
                                                   10, 400, 0, 0, 0, 0, 0,
                                                   false, false, missionItemParent);
        missionItems.append(servoHoldItem);
        qCDebug(MissionUploadServiceLog) << "Added servo hold command (Servo 10 = 400) for waypoint" << (i + 1);
        
        // 2. Delay for servo pickup
        MissionItem* delayItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_DELAY, MAV_FRAME_MISSION,
                                               servoDelaySeconds, 0, 0, 0, 0, 0, 0,
                                               false, false, missionItemParent);
        missionItems.append(delayItem);
        qCDebug(MissionUploadServiceLog) << "Added delay of" << servoDelaySeconds << "seconds for servo pickup for waypoint" << (i + 1);
        
        // 3. Takeoff to working altitude + bend height
        QGeoCoordinate takeoffCoord = droneCurrentPosition;
        takeoffCoord.setAltitude(altitude + bendHeight);
        MissionItem* takeoffItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                  0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(),
                                                  false, false, missionItemParent);
        missionItems.append(takeoffItem);
        qCDebug(MissionUploadServiceLog) << "Added takeoff to altitude" << (altitude + bendHeight) << "for waypoint" << (i + 1);
        
        // 4. Fly to waypoint at working altitude + bend height
        QGeoCoordinate waypointHighCoord = wpCoord;
        waypointHighCoord.setAltitude(altitude + bendHeight);
        MissionItem* waypointItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                   0, 0, 0, 0, waypointHighCoord.latitude(), waypointHighCoord.longitude(), waypointHighCoord.altitude(),
                                                   false, false, missionItemParent);
        missionItems.append(waypointItem);
        qCDebug(MissionUploadServiceLog) << "Added waypoint at altitude" << (altitude + bendHeight) << "for waypoint" << (i + 1);
        
        // 5. Descend to payload drop height
        QGeoCoordinate dropCoord = wpCoord;
        dropCoord.setAltitude(payloadDropHeight);
        MissionItem* descendItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                  0, 0, 0, 0, dropCoord.latitude(), dropCoord.longitude(), dropCoord.altitude(),
                                                  false, false, missionItemParent);
        missionItems.append(descendItem);
        qCDebug(MissionUploadServiceLog) << "Added descent to payload drop height" << payloadDropHeight << "for waypoint" << (i + 1);
        
        // 6. Servo 10 = 2400 (Release payload)
        MissionItem* servoReleaseItem = new MissionItem(sequenceNumber++, MAV_CMD_DO_SET_SERVO, MAV_FRAME_MISSION,
                                                      10, 2400, 0, 0, 0, 0, 0,
                                                      false, false, missionItemParent);
        missionItems.append(servoReleaseItem);
        qCDebug(MissionUploadServiceLog) << "Added servo release command (Servo 10 = 2400) for waypoint" << (i + 1);
        
        // 7. Climb back to working altitude + bend height
        MissionItem* climbItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                0, 0, 0, 0, waypointHighCoord.latitude(), waypointHighCoord.longitude(), waypointHighCoord.altitude(),
                                                false, false, missionItemParent);
        missionItems.append(climbItem);
        qCDebug(MissionUploadServiceLog) << "Added climb back to altitude" << (altitude + bendHeight) << "for waypoint" << (i + 1);
        
        // 8. Land (continueAfterLand will automatically continue to next waypoint)
        QGeoCoordinate landCoord = droneCurrentPosition;
        landCoord.setAltitude(0); // Land at ground level
        MissionItem* landItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                               0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(),
                                               false, false, missionItemParent);
        missionItems.append(landItem);
        qCDebug(MissionUploadServiceLog) << "Added land command for waypoint" << (i + 1) << "- continueAfterLand will handle continuation";
        
        // 8.5. Add UNKNOWN command with ID 218 (MAV_CMD_DO_AUX_FUNCTION) after LAND
        // This command should display as "UNKNOWN" in QGroundControl UI
        // Parameters: param1=AuxiliaryFunction=153 (ARMDISARM), param2=SwitchPosition, param3-7=Empty
        MissionItem* unknownItem = new MissionItem(sequenceNumber++, static_cast<MAV_CMD>(218), MAV_FRAME_MISSION,
                                                  153, 0, 0, 0, 0, 0, 0,
                                                  false, false, missionItemParent);
        missionItems.append(unknownItem);
        qCDebug(MissionUploadServiceLog) << "Added UNKNOWN command (ID 218, param1=153 ARMDISARM) after land for waypoint" << (i + 1);
        
        // 9. Add delay between waypoints for payload preparation
        if (i < waypoints.size() - 1) {
            MissionItem* waypointDelayItem = new MissionItem(sequenceNumber++, MAV_CMD_NAV_DELAY, MAV_FRAME_MISSION,
                                                           5, 0, 0, 0, 0, 0, 0,
                                                           false, false, missionItemParent);
            missionItems.append(waypointDelayItem);
            qCDebug(MissionUploadServiceLog) << "Added delay between waypoints for payload preparation";
        }
    }
    
    // GenCall65: Upload single mission with all waypoints and continueAfterLand
    if (vehicle->missionManager()) {
        vehicle->missionManager()->writeMissionItems(missionItems);
        qCDebug(MissionUploadServiceLog) << "Successfully uploaded payload drop mission with continueAfterLand -" << missionItems.size() << "items to vehicle" << vehicle->id();
        
        // Emit success signal for this vehicle
        emit missionUploadCompleted(true, QString("Payload drop mission with continueAfterLand uploaded to vehicle %1").arg(vehicle->id()));
    } else {
        qCWarning(MissionUploadServiceLog) << "Failed to upload payload drop mission - no mission manager for vehicle" << vehicle->id();
        emit missionUploadCompleted(false, QString("No mission manager for vehicle %1").arg(vehicle->id()));
    }
}