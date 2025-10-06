#include "MissionUploadService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "MissionManager/MissionItem.h"
#include "MissionManager/MissionManager.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "QGCMAVLink.h"
#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
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

QString MissionUploadService::getHumanReadableCommandDescription(MAV_CMD command, const QList<double>& params, int sequenceNumber)
{
    QString description;
    
    switch (command) {
        case MAV_CMD_NAV_WAYPOINT:
            if (params.size() >= 7) {
                description = QString("Heading to waypoint at %1, %2 (altitude: %3m)")
                    .arg(params[4], 0, 'f', 6)
                    .arg(params[5], 0, 'f', 6)
                    .arg(params[6], 0, 'f', 1);
            } else {
                description = "Heading to waypoint";
            }
            break;
            
        case MAV_CMD_NAV_TAKEOFF:
            if (params.size() >= 7) {
                description = QString("Taking off to %1m altitude")
                    .arg(params[6], 0, 'f', 1);
            } else {
                description = "Taking off";
            }
            break;
            
        case MAV_CMD_NAV_LAND:
            description = "Landing at current position";
            break;
            
        case MAV_CMD_NAV_LOITER_TIME:
            if (params.size() >= 2) {
                description = QString("Loitering for %1 seconds")
                    .arg(params[1], 0, 'f', 0);
            } else {
                description = "Loitering";
            }
            break;
            
        case MAV_CMD_DO_SET_SERVO:
            if (params.size() >= 2) {
                int servoId = static_cast<int>(params[0]);
                int servoValue = static_cast<int>(params[1]);
                
                if (servoId == 10) {
                    if (servoValue >= 2000) {
                        description = "Releasing gripper (servo 10 = 2400)";
                    } else if (servoValue <= 600) {
                        description = "Closing gripper (servo 10 = 400)";
                    } else {
                        description = QString("Setting gripper servo 10 to %1").arg(servoValue);
                    }
                } else {
                    description = QString("Setting servo %1 to %2").arg(servoId).arg(servoValue);
                }
            } else {
                description = "Setting servo";
            }
            break;
            
        case MAV_CMD_NAV_DELAY:
            if (params.size() >= 1) {
                description = QString("Waiting for %1 seconds")
                    .arg(params[0], 0, 'f', 0);
            } else {
                description = "Waiting";
            }
            break;
            
        case MAV_CMD_DO_SET_RELAY:
            if (params.size() >= 2) {
                int relayId = static_cast<int>(params[0]);
                int relayValue = static_cast<int>(params[1]);
                if (relayValue > 0) {
                    description = QString("Activating relay %1").arg(relayId);
                } else {
                    description = QString("Deactivating relay %1").arg(relayId);
                }
            } else {
                description = "Setting relay";
            }
            break;
            
        case MAV_CMD_DO_AUX_FUNCTION:
            if (params.size() >= 1) {
                int auxFunction = static_cast<int>(params[0]);
                if (auxFunction == 153) {
                    description = "Arming/Disarming system";
                } else {
                    description = QString("Auxiliary function %1").arg(auxFunction);
                }
            } else {
                description = "Auxiliary function";
            }
            break;
            
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            description = "Returning to launch position";
            break;
            
        case MAV_CMD_NAV_LOITER_UNLIM:
            description = "Loitering indefinitely";
            break;
            
        case MAV_CMD_NAV_LOITER_TURNS:
            if (params.size() >= 1) {
                description = QString("Loitering for %1 turns")
                    .arg(params[0], 0, 'f', 0);
            } else {
                description = "Loitering for turns";
            }
            break;
            
        default:
            // Handle unknown commands
            if (command >= 200 && command <= 255) {
                description = QString("Custom command %1").arg(static_cast<int>(command));
            } else {
                description = QString("Unknown command %1").arg(static_cast<int>(command));
            }
            break;
    }
    
    return description;
}

QString MissionUploadService::getShortMissionDescription(const QString& fullDescription)
{
    // Convert long descriptions to short, concise versions
    if (fullDescription.contains("Home position")) {
        return "Home";
    } else if (fullDescription.contains("Releasing gripper")) {
        return "Release Gripper";
    } else if (fullDescription.contains("Closing gripper")) {
        return "Close Gripper";
    } else if (fullDescription.contains("Waiting for")) {
        return "Wait";
    } else if (fullDescription.contains("Taking off")) {
        return "Takeoff";
    } else if (fullDescription.contains("Heading to waypoint")) {
        // Extract waypoint number using simple string operations
        int wpIndex = fullDescription.indexOf("waypoint ");
        if (wpIndex != -1) {
            QString remaining = fullDescription.mid(wpIndex + 9); // Skip "waypoint "
            int spaceIndex = remaining.indexOf(' ');
            if (spaceIndex != -1) {
                QString wpNumber = remaining.left(spaceIndex);
                return QString("WP%1").arg(wpNumber);
            }
        }
        return "Waypoint";
    } else if (fullDescription.contains("Loitering for")) {
        return "Loiter";
    } else if (fullDescription.contains("Returning to mission altitude")) {
        return "Return to Alt";
    } else if (fullDescription.contains("Returning to launch")) {
        return "RTL";
    } else if (fullDescription.contains("Landing at")) {
        return "Land";
    } else if (fullDescription.contains("Mission created with")) {
        return "Mission Complete";
    }
    
    // Default: return first 20 characters if no pattern matches
    return fullDescription.left(20) + (fullDescription.length() > 20 ? "..." : "");
}

void MissionUploadService::logMissionItem(const QString& vehicleId, int sequenceNumber, MAV_CMD command, const QList<double>& params, const QString& description)
{
    qCDebug(MissionUploadServiceLog) << QString("Vehicle %1 - Item %2: %3")
        .arg(vehicleId)
        .arg(sequenceNumber)
        .arg(description);
}

void MissionUploadService::logMissionItemToVehicle(Vehicle* vehicle, int sequenceNumber, const QString& description)
{
    if (!vehicle) {
        qCWarning(MissionUploadServiceLog) << "Cannot log mission item: vehicle is null";
        return;
    }
    
    // Create concise mission item description with Vehicle ID
    QString shortDescription = getShortMissionDescription(description);
    QString message = QString("Vehicle %1 - Item %2: %3").arg(vehicle->id()).arg(sequenceNumber).arg(shortDescription);
    
    // Log to debug console with Vehicle ID and short description
    qCDebug(MissionUploadServiceLog) << message;
    
    // TODO: Implement proper vehicle logs integration when access to status text handler is available
    // For now, this provides the human-readable logging functionality in debug output
}

void MissionUploadService::exportHumanReadableMission(const QString& vehicleId, const QList<MissionItem*>& missionItems, const QString& filename)
{
    QString defaultFilename = QString("mission_vehicle_%1_%2.txt")
        .arg(vehicleId)
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    
    QString actualFilename = filename.isEmpty() ? defaultFilename : filename;
    
    QFile file(actualFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(MissionUploadServiceLog) << "Failed to open file for writing:" << actualFilename;
        return;
    }
    
    QTextStream out(&file);
    out << "=== HUMAN-READABLE MISSION FOR VEHICLE " << vehicleId << " ===" << Qt::endl;
    out << "Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << Qt::endl;
    out << "Total Mission Items: " << missionItems.size() << Qt::endl;
    out << Qt::endl;
    
    for (int i = 0; i < missionItems.size(); i++) {
        MissionItem* item = missionItems[i];
        QList<double> params = {item->param1(), item->param2(), item->param3(), item->param4(), 
                               item->param5(), item->param6(), item->param7()};
        QString description = getHumanReadableCommandDescription(static_cast<MAV_CMD>(item->command()), params, item->sequenceNumber());
        out << QString("%1. %2").arg(item->sequenceNumber()).arg(description) << Qt::endl;
    }
    
    out << Qt::endl;
    out << "=== END MISSION ===" << Qt::endl;
    
    file.close();
    qCDebug(MissionUploadServiceLog) << "Human-readable mission exported to:" << actualFilename;
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
    
    // Log human-readable mission item
    QList<double> homeParams = {0, 0, 0, 0, homeCoord.latitude(), homeCoord.longitude(), homeCoord.altitude()};
    QString homeDescription = getHumanReadableCommandDescription(MAV_CMD_NAV_WAYPOINT, homeParams, 0);
    logMissionItem(QString::number(vehicle->id()), 0, MAV_CMD_NAV_WAYPOINT, homeParams, "Home position (will be skipped)");
    logMissionItemToVehicle(vehicle, 0, "Home position (will be skipped)");
    
    // GenCall44: Add servo 10 HIGH (2400 PWM) command before takeoff
    qCDebug(MissionUploadServiceLog) << "Adding servo 10 HIGH (2400 PWM) command before takeoff";
    MissionItem* servoHighItem = new MissionItem(1, MAV_CMD_DO_SET_SERVO, MAV_FRAME_MISSION,
                                                10,    // param1: Servo number (10)
                                                2400,  // param2: PWM value (2400 = HIGH)
                                                0,     // param3: unused
                                                0,     // param4: unused
                                                0,     // param5: unused
                                                0,     // param6: unused
                                                0,     // param7: unused
                                                false, // autoContinue
                                                false, // isCurrentItem
                                                missionItemParent);
    missionItems.append(servoHighItem);
    qCDebug(MissionUploadServiceLog) << "Added servo 10 HIGH command";
    
    // Log human-readable mission item
    QList<double> servoHighParams = {10, 2400, 0, 0, 0, 0, 0};
    logMissionItem(QString::number(vehicle->id()), 1, MAV_CMD_DO_SET_SERVO, servoHighParams, "Releasing gripper (servo 10 = 2400)");
    logMissionItemToVehicle(vehicle, 1, "Releasing gripper (servo 10 = 2400)");
    
    // GenCall45: Add delay command (2 seconds)
    qCDebug(MissionUploadServiceLog) << "Adding 2 second delay after servo HIGH";
    MissionItem* delayItem = new MissionItem(2, MAV_CMD_NAV_DELAY, MAV_FRAME_MISSION,
                                           2,     // param1: Delay in seconds
                                           0,     // param2: unused
                                           0,     // param3: unused
                                           0,     // param4: unused
                                           0,     // param5: unused
                                           0,     // param6: unused
                                           0,     // param7: unused
                                           false, // autoContinue
                                           false, // isCurrentItem
                                           missionItemParent);
    missionItems.append(delayItem);
    qCDebug(MissionUploadServiceLog) << "Added 2 second delay";
    
    // Log human-readable mission item
    QList<double> delayParams = {2, 0, 0, 0, 0, 0, 0};
    logMissionItem(QString::number(vehicle->id()), 2, MAV_CMD_NAV_DELAY, delayParams, "Waiting for 2 seconds");
    logMissionItemToVehicle(vehicle, 2, "Waiting for 2 seconds");
    
    // GenCall46: Add servo 10 LOW (400 PWM) command after delay
    qCDebug(MissionUploadServiceLog) << "Adding servo 10 LOW (400 PWM) command after delay";
    MissionItem* servoLowItem = new MissionItem(3, MAV_CMD_DO_SET_SERVO, MAV_FRAME_MISSION,
                                               10,    // param1: Servo number (10)
                                               400,   // param2: PWM value (400 = LOW)
                                               0,     // param3: unused
                                               0,     // param4: unused
                                               0,     // param5: unused
                                               0,     // param6: unused
                                               0,     // param7: unused
                                               false, // autoContinue
                                               false, // isCurrentItem
                                               missionItemParent);
    missionItems.append(servoLowItem);
    qCDebug(MissionUploadServiceLog) << "Added servo 10 LOW command";
    
    // Log human-readable mission item
    QList<double> servoLowParams = {10, 400, 0, 0, 0, 0, 0};
    logMissionItem(QString::number(vehicle->id()), 3, MAV_CMD_DO_SET_SERVO, servoLowParams, "Closing gripper (servo 10 = 400)");
    logMissionItemToVehicle(vehicle, 3, "Closing gripper (servo 10 = 400)");
    
    // GenCall47: Add takeoff item as fourth item (first real mission item)
    QGeoCoordinate takeoffCoord = droneCurrentPosition;
    takeoffCoord.setAltitude(altitude);
    qCDebug(MissionUploadServiceLog) << "Creating takeoff item with coordinates:" << takeoffCoord.toString();
    
    MissionItem* takeoffItem = new MissionItem(4, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT, 
                                               0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude(), 
                                               false, false, missionItemParent);
    missionItems.append(takeoffItem);
    qCDebug(MissionUploadServiceLog) << "Successfully created takeoff item - Command:" << takeoffItem->command() << "Frame:" << takeoffItem->frame();
    
    // Log human-readable mission item
    QList<double> takeoffParams = {0, 0, 0, 0, takeoffCoord.latitude(), takeoffCoord.longitude(), takeoffCoord.altitude()};
    logMissionItem(QString::number(vehicle->id()), 4, MAV_CMD_NAV_TAKEOFF, takeoffParams, QString("Taking off to %1m altitude").arg(altitude));
    logMissionItemToVehicle(vehicle, 4, QString("Taking off to %1m altitude").arg(altitude));
    
    // GenCall48: Add waypoint items with servo control
    for (int i = 0; i < waypoints.size(); i++) {
        QGeoCoordinate coord = waypoints[i];
        coord.setAltitude(altitude);
        int waypointSeq = i + 5; // Start from sequence 5 (after home, servo high, delay, servo low, takeoff)
        
        // Add waypoint
        MissionItem* waypointItem = new MissionItem(waypointSeq, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                   0, 0, 0, 0, coord.latitude(), coord.longitude(), coord.altitude(), 
                                                   false, false, missionItemParent);
        missionItems.append(waypointItem);
        qCDebug(MissionUploadServiceLog) << "Added waypoint" << (i + 1) << "at sequence" << waypointSeq << ":" << coord.toString();
        
        // Log human-readable mission item
        QList<double> waypointParams = {0, 0, 0, 0, coord.latitude(), coord.longitude(), coord.altitude()};
        QString waypointDescription = QString("Heading to waypoint %1 at %2, %3 (altitude: %4m)")
                      .arg(i + 1)
                      .arg(coord.latitude(), 0, 'f', 6)
                      .arg(coord.longitude(), 0, 'f', 6)
                      .arg(coord.altitude(), 0, 'f', 1);
        logMissionItem(QString::number(vehicle->id()), waypointSeq, MAV_CMD_NAV_WAYPOINT, waypointParams, waypointDescription);
        logMissionItemToVehicle(vehicle, waypointSeq, waypointDescription);
        
        // Add loiter at 1.5m altitude immediately after waypoint
        QGeoCoordinate loiterCoord = coord;
        loiterCoord.setAltitude(1.5); // 1.5m altitude for loiter
        MissionItem* loiterItem = new MissionItem(waypointSeq + 1, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                 1.5, // param1: loiter time in seconds (1.5 seconds)
                                                 0,   // param2: unused
                                                 0,   // param3: unused
                                                 0,   // param4: unused
                                                 loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                                 false, false, missionItemParent);
        missionItems.append(loiterItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter at 1.5m altitude after waypoint" << (i + 1);
        
        // Log human-readable mission item
        QList<double> loiterParams = {1.5, 0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude()};
        QString loiterDescription = QString("Loitering for 1.5 seconds at waypoint %1").arg(i + 1);
        logMissionItem(QString::number(vehicle->id()), waypointSeq + 1, MAV_CMD_NAV_LOITER_TIME, loiterParams, loiterDescription);
        logMissionItemToVehicle(vehicle, waypointSeq + 1, loiterDescription);
        
        // Add servo 10 HIGH (2400 PWM) after loiter
        MissionItem* servoHighAfterLoiter = new MissionItem(waypointSeq + 2, MAV_CMD_DO_SET_SERVO, MAV_FRAME_MISSION,
                                                           10,    // param1: Servo number (10)
                                                           2400,  // param2: PWM value (2400 = HIGH)
                                                           0,     // param3: unused
                                                           0,     // param4: unused
                                                           0,     // param5: unused
                                                           0,     // param6: unused
                                                           0,     // param7: unused
                                                           false, // autoContinue
                                                           false, // isCurrentItem
                                                           missionItemParent);
        missionItems.append(servoHighAfterLoiter);
        qCDebug(MissionUploadServiceLog) << "Added servo 10 HIGH after loiter for waypoint" << (i + 1);
        
        // Log human-readable mission item
        QList<double> servoHighAfterLoiterParams = {10, 2400, 0, 0, 0, 0, 0};
        QString servoHighAfterLoiterDescription = QString("Releasing gripper at waypoint %1 (servo 10 = 2400)").arg(i + 1);
        logMissionItem(QString::number(vehicle->id()), waypointSeq + 2, MAV_CMD_DO_SET_SERVO, servoHighAfterLoiterParams, servoHighAfterLoiterDescription);
        logMissionItemToVehicle(vehicle, waypointSeq + 2, servoHighAfterLoiterDescription);
        
        // Add loiter back to mission altitude
        QGeoCoordinate backToAltCoord = coord;
        backToAltCoord.setAltitude(altitude); // Back to mission altitude
        MissionItem* backToAltItem = new MissionItem(waypointSeq + 3, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                    1.0, // param1: loiter time in seconds (1 second)
                                                    0,   // param2: unused
                                                    0,   // param3: unused
                                                    0,   // param4: unused
                                                    backToAltCoord.latitude(), backToAltCoord.longitude(), backToAltCoord.altitude(),
                                                    false, false, missionItemParent);
        missionItems.append(backToAltItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter back to mission altitude" << altitude << "m for waypoint" << (i + 1);
        
        // Log human-readable mission item
        QList<double> backToAltParams = {1.0, 0, 0, 0, backToAltCoord.latitude(), backToAltCoord.longitude(), backToAltCoord.altitude()};
        QString backToAltDescription = QString("Returning to mission altitude %1m after waypoint %2").arg(altitude).arg(i + 1);
        logMissionItem(QString::number(vehicle->id()), waypointSeq + 3, MAV_CMD_NAV_LOITER_TIME, backToAltParams, backToAltDescription);
        logMissionItemToVehicle(vehicle, waypointSeq + 3, backToAltDescription);
    }
    
    // GenCall49: Add RTL (Return to Launch) before landing
    int rtlSeq = 5 + (waypoints.size() * 4); // Calculate RTL sequence number
    MissionItem* rtlItem = new MissionItem(rtlSeq, MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_MISSION,
                                         0, 0, 0, 0, 0, 0, 0,
                                         false, false, missionItemParent);
    missionItems.append(rtlItem);
    qCDebug(MissionUploadServiceLog) << "Added return to launch at sequence" << rtlSeq;
    
    // Log human-readable mission item
    QList<double> rtlParams = {0, 0, 0, 0, 0, 0, 0};
    logMissionItem(QString::number(vehicle->id()), rtlSeq, MAV_CMD_NAV_RETURN_TO_LAUNCH, rtlParams, "Returning to launch position");
    logMissionItemToVehicle(vehicle, rtlSeq, "Returning to launch position");
    
    // GenCall50: Add land item at drone's current position (return to start)
    QGeoCoordinate landCoord = droneCurrentPosition;
    landCoord.setAltitude(0); // Land at ground level
    MissionItem* landItem = new MissionItem(rtlSeq + 1, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                           0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(), 
                                           false, false, missionItemParent);
    missionItems.append(landItem);
    qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
    
    // Log human-readable mission item
    QList<double> landParams = {0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude()};
    logMissionItem(QString::number(vehicle->id()), rtlSeq + 1, MAV_CMD_NAV_LAND, landParams, "Landing at current position");
    logMissionItemToVehicle(vehicle, rtlSeq + 1, "Landing at current position");
    
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
    
    // Log human-readable mission summary
    qCDebug(MissionUploadServiceLog) << "=== HUMAN-READABLE MISSION SUMMARY FOR VEHICLE" << vehicle->id() << "===";
    qCDebug(MissionUploadServiceLog) << "Mission contains" << missionItems.size() << "items:";
    for (int i = 0; i < missionItems.size(); i++) {
        MissionItem* item = missionItems[i];
        QList<double> params = {item->param1(), item->param2(), item->param3(), item->param4(), 
                               item->param5(), item->param6(), item->param7()};
        QString description = getHumanReadableCommandDescription(static_cast<MAV_CMD>(item->command()), params, item->sequenceNumber());
        qCDebug(MissionUploadServiceLog) << QString("  %1. %2").arg(item->sequenceNumber()).arg(description);
    }
    qCDebug(MissionUploadServiceLog) << "=== END MISSION SUMMARY ===";
    
    // Send mission summary to vehicle logs
    QString missionSummary = QString("Vehicle %1 - Mission Complete: %2 items").arg(vehicle->id()).arg(missionItems.size());
    qCDebug(MissionUploadServiceLog) << missionSummary;
    
    // TODO: Implement proper vehicle logs integration when access to status text handler is available
    // For now, this provides the human-readable logging functionality in debug output
    
    // Export human-readable mission to file
    exportHumanReadableMission(QString::number(vehicle->id()), missionItems);
    
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
        
        // Add loiter item at observation position for 120 seconds
        QGeoCoordinate loiterCoord = loiterPosition;
        loiterCoord.setAltitude(altitude);
        MissionItem* loiterItem = new MissionItem(3, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                120, // Loiter for 120 seconds (2 minutes)
                                                0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                                false, false, missionItemParent);
        missionItems.append(loiterItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter at observation position for 120 seconds:" << loiterCoord.toString();
        
        // Add relay HIGH command immediately after loiter
        MissionItem* relayHighItem = new MissionItem(4, MAV_CMD_DO_SET_RELAY, MAV_FRAME_MISSION,
                                                    0,  // param1: Relay number (0)
                                                    1,  // param2: State (1 = HIGH/ON)
                                                    0,  // param3: unused
                                                    0,  // param4: unused
                                                    0,  // param5: unused
                                                    0,  // param6: unused
                                                    0,  // param7: unused
                                                    false, // autoContinue
                                                    false, // isCurrentItem
                                                    missionItemParent);
        missionItems.append(relayHighItem);
        qCDebug(MissionUploadServiceLog) << "Added relay HIGH command after loiter";
        
        // Add RTL (Return to Launch) before landing
        MissionItem* rtlItem = new MissionItem(5, MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_MISSION,
                                             0, 0, 0, 0, 0, 0, 0,
                                             false, false, missionItemParent);
        missionItems.append(rtlItem);
        qCDebug(MissionUploadServiceLog) << "Added return to launch for drone ID 1";
        
        // Add land item
        QGeoCoordinate landCoord = droneCurrentPosition;
        landCoord.setAltitude(0); // Land at ground level
        MissionItem* landItem = new MissionItem(6, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                              0, 0, 0, 0, landCoord.latitude(), landCoord.longitude(), landCoord.altitude(),
                                              false, false, missionItemParent);
        missionItems.append(landItem);
        qCDebug(MissionUploadServiceLog) << "Added land at vehicle position:" << landCoord.toString();
        
    } else {
        // No mission center provided, use standard loiter without yaw control
        QGeoCoordinate loiterCoord = loiterPosition;
        loiterCoord.setAltitude(altitude);
        MissionItem* loiterItem = new MissionItem(2, MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                                120, // Loiter for 120 seconds (2 minutes)
                                                0, 0, 0, loiterCoord.latitude(), loiterCoord.longitude(), loiterCoord.altitude(),
                                                false, false, missionItemParent);
        missionItems.append(loiterItem);
        qCDebug(MissionUploadServiceLog) << "Added loiter at observation position for 120 seconds:" << loiterCoord.toString();
        
        // Add relay HIGH command immediately after loiter
        MissionItem* relayHighItem = new MissionItem(3, MAV_CMD_DO_SET_RELAY, MAV_FRAME_MISSION,
                                                    0,  // param1: Relay number (0)
                                                    1,  // param2: State (1 = HIGH/ON)
                                                    0,  // param3: unused
                                                    0,  // param4: unused
                                                    0,  // param5: unused
                                                    0,  // param6: unused
                                                    0,  // param7: unused
                                                    false, // autoContinue
                                                    false, // isCurrentItem
                                                    missionItemParent);
        missionItems.append(relayHighItem);
        qCDebug(MissionUploadServiceLog) << "Added relay HIGH command after loiter (no yaw control)";
        
        // Add RTL (Return to Launch) before landing
        MissionItem* rtlItem = new MissionItem(4, MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_FRAME_MISSION,
                                             0, 0, 0, 0, 0, 0, 0,
                                             false, false, missionItemParent);
        missionItems.append(rtlItem);
        qCDebug(MissionUploadServiceLog) << "Added return to launch for drone ID 1 (no yaw control)";
        
        // Add land item
        QGeoCoordinate landCoord = droneCurrentPosition;
        landCoord.setAltitude(0); // Land at ground level
        MissionItem* landItem = new MissionItem(5, MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT,
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

void MissionUploadService::clearMission(Vehicle* vehicle)
{
    qCDebug(MissionUploadServiceLog) << "GenCall1: clearMission() - Clearing mission for vehicle" << vehicle->id();
    
    if (!vehicle) {
        qCWarning(MissionUploadServiceLog) << "GenCall2: Invalid vehicle provided";
        return;
    }
    
    if (!vehicle->missionManager()) {
        qCWarning(MissionUploadServiceLog) << "GenCall3: No mission manager for vehicle" << vehicle->id();
        return;
    }
    
    // Note: Mission clearing is handled by the mission generation process
    // The new mission will overwrite the existing one
    qCDebug(MissionUploadServiceLog) << "GenCall4: Mission will be overwritten for vehicle" << vehicle->id();
}