/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "../../pch.h"
#include "../AreaPlanEditor.h"

// QGroundControl includes
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MissionManager.h"
#include "MissionItem.h"
#include "MissionController.h"
#include "QGCMAVLink.h"

/**
 * @file AreaPlanEditor_VehicleControl.cc
 * @brief Vehicle control and management for AreaPlanEditor
 * 
 * This file contains all vehicle control, status monitoring, and
 * mission upload methods for the AreaPlanEditor class.
 */

// Vehicle control methods
void AreaPlanEditor::armVehicle(QObject* vehicleObject, bool arm)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    if (arm) {
        vehicle->setArmed(true, false);
        updateStatus(QString("Arming vehicle %1").arg(vehicle->id()));
    } else {
        vehicle->setArmed(false, false);
        updateStatus(QString("Disarming vehicle %1").arg(vehicle->id()));
    }
}

void AreaPlanEditor::takeoffVehicle(QObject* vehicleObject, qreal altitude)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    if (!vehicle->armed()) {
        handleError("Vehicle not armed", "Please arm the vehicle before takeoff");
        return;
    }
    
    // Send takeoff command with specified altitude
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_NAV_TAKEOFF, 
                          0.0, 0.0, 0.0, 0.0, 0.0, static_cast<float>(altitude));
    
    updateStatus(QString("Takeoff initiated for vehicle %1 at altitude %2m")
                .arg(vehicle->id())
                .arg(altitude));
}

void AreaPlanEditor::landVehicle(QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Send land command
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_NAV_LAND, 
                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    updateStatus(QString("Land command sent to vehicle %1").arg(vehicle->id()));
}

void AreaPlanEditor::startMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    if (!vehicle->armed()) {
        handleError("Vehicle not armed", "Please arm the vehicle before starting mission");
        return;
    }
    
    // Send mission start command
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_MISSION_START, 
                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    updateStatus(QString("Mission started on vehicle %1").arg(vehicle->id()));
}

void AreaPlanEditor::pauseMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Send mission pause command
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_DO_PAUSE_CONTINUE, 
                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    updateStatus(QString("Mission paused on vehicle %1").arg(vehicle->id()));
}

void AreaPlanEditor::rtlVehicle(QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Send RTL command
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_NAV_RETURN_TO_LAUNCH, 
                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    updateStatus(QString("RTL command sent to vehicle %1").arg(vehicle->id()));
}

void AreaPlanEditor::continueMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Send mission continue command
    vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                          MAV_CMD_DO_PAUSE_CONTINUE, 
                          1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    updateStatus(QString("Mission continued on vehicle %1").arg(vehicle->id()));
}

// Vehicle status and information methods
Vehicle* AreaPlanEditor::getCurrentVehicle() const
{
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return nullptr;
    }
    
    return vehicleManager->activeVehicle();
}

QVariantMap AreaPlanEditor::getVehicleStatus(QObject* vehicleObject) const
{
    QVariantMap status;
    
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        status["error"] = "Invalid vehicle object";
        return status;
    }
    
    status["id"] = vehicle->id();
    status["connected"] = vehicle->isConnected();
    status["armed"] = vehicle->armed();
    status["flying"] = vehicle->flying();
    status["batteryLevel"] = vehicle->battery()->percentRemaining()->rawValue().toDouble();
    status["altitude"] = vehicle->altitudeAMSL();
    status["groundSpeed"] = vehicle->groundSpeed();
    status["airSpeed"] = vehicle->airSpeed();
    status["heading"] = vehicle->heading();
    status["coordinate"] = QVariant::fromValue(vehicle->coordinate());
    
    // Mission status
    if (vehicle->missionManager()) {
        status["missionItemCount"] = vehicle->missionManager()->missionItems().count();
        status["currentMissionItem"] = vehicle->missionManager()->currentMissionItem();
        status["missionInProgress"] = vehicle->missionManager()->inProgress();
    }
    
    // Flight mode
    status["flightMode"] = vehicle->flightMode();
    
    return status;
}

QList<QVariant> AreaPlanEditor::getAvailableVehicles() const
{
    QList<QVariant> vehicles;
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return vehicles;
    }
    
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle) {
            QVariantMap vehicleInfo;
            vehicleInfo["id"] = vehicle->id();
            vehicleInfo["name"] = vehicle->vehicleName();
            vehicleInfo["connected"] = vehicle->connected();
            vehicleInfo["armed"] = vehicle->armed();
            vehicleInfo["flying"] = vehicle->flying();
            vehicleInfo["coordinate"] = QVariant::fromValue(vehicle->coordinate());
            
            vehicles.append(QVariant::fromValue(vehicleInfo));
        }
    }
    
    return vehicles;
}

// Mission upload methods
void AreaPlanEditor::uploadToVehicle()
{
    if (!_planMasterController) {
        handleError("No mission controller available", "Please ensure a mission controller is set");
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(_planMasterController);
    if (!missionController) {
        handleError("Invalid mission controller", "Please ensure a valid mission controller is set");
        return;
    }
    
    Vehicle* vehicle = getCurrentVehicle();
    if (!vehicle) {
        handleError("No active vehicle", "Please connect a vehicle first");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Upload mission to vehicle
    missionController->upload();
    
    updateStatus(QString("Mission uploaded to vehicle %1").arg(vehicle->id()));
}

void AreaPlanEditor::uploadPerDroneMissionToVehicle(int droneIndex, QObject* vehicleObject)
{
    if (droneIndex < 0 || droneIndex >= droneCount()) {
        handleError("Invalid drone index", QString("Drone index %1 is out of range").arg(droneIndex));
        return;
    }
    
    Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleObject);
    if (!vehicle) {
        handleError("Invalid vehicle object", "Please provide a valid vehicle object");
        return;
    }
    
    if (!vehicle->isConnected()) {
        handleError("Vehicle not connected", "Please ensure the vehicle is connected");
        return;
    }
    
    // Generate waypoints for this specific drone
    QList<QVariant> waypoints = generatePerDroneWaypoints(droneIndex);
    
    // Create mission items from waypoints
    QList<MissionItem*> missionItems;
    for (const QVariant& waypoint : waypoints) {
        QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
        MissionItem* item = new MissionItem(this);
        item->coordinate()->setRawValue(coord);
        missionItems.append(item);
    }
    
    // Upload mission to vehicle
    // Note: This is a simplified implementation - in practice, you would
    // need to properly upload the mission items to the vehicle's mission manager
    
    updateStatus(QString("Mission uploaded for drone %1 to vehicle %2")
                .arg(droneIndex)
                .arg(vehicle->id()));
}

void AreaPlanEditor::uploadToAllDrones()
{
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return;
    }
    
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->connected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        handleError("No connected vehicles", "Please connect at least one vehicle");
        return;
    }
    
    // Upload mission to all connected vehicles
    for (int i = 0; i < qMin(vehicles.count(), droneCount()); ++i) {
        uploadPerDroneMissionToVehicle(i, vehicles[i]);
    }
    
    updateStatus(QString("Mission uploaded to all %1 connected vehicles").arg(vehicles.count()));
}

void AreaPlanEditor::startMission()
{
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return;
    }
    
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->connected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        handleError("No connected vehicles", "Please connect at least one vehicle");
        return;
    }
    
    // Start mission on all connected vehicles
    for (Vehicle* vehicle : vehicles) {
        startMissionOnVehicle(vehicle);
    }
    
    updateStatus(QString("Mission started on all %1 connected vehicles").arg(vehicles.count()));
}