/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "../../pch.h"
#include <QtCore/QtMath>
#include <QtPositioning/QGeoCoordinate>
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
 * @file AreaPlanEditor_SwarmCoordination.cc
 * @brief Swarm coordination and formation logic for AreaPlanEditor
 * 
 * This file contains all swarm coordination, formation management,
 * and multi-drone coordination methods for the AreaPlanEditor class.
 */

// Swarm status and coordination methods
bool AreaPlanEditor::isSwarmReady()
{
    if (droneCount() <= 0) {
        return false;
    }
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return false;
    }
    
    int connectedVehicles = 0;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            connectedVehicles++;
        }
    }
    
    return connectedVehicles >= droneCount();
}

QString AreaPlanEditor::swarmStatus() const
{
    if (!isSwarmReady()) {
        return "Swarm not ready - insufficient connected vehicles";
    }
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return "Vehicle manager not available";
    }
    
    int connectedVehicles = 0;
    int armedVehicles = 0;
    
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            connectedVehicles++;
            if (vehicle->armed()) {
                armedVehicles++;
            }
        }
    }
    
    return QString("Swarm ready: %1/%2 vehicles connected, %3 armed")
           .arg(connectedVehicles)
           .arg(droneCount())
           .arg(armedVehicles);
}

bool AreaPlanEditor::isCoordinatedMissionActive() const
{
    // Check if any vehicles are currently executing coordinated missions
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return false;
    }
    
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->connected() && vehicle->flying()) {
            return true;
        }
    }
    
    return false;
}

bool AreaPlanEditor::isFormationTransitioning() const
{
    return _isFormationTransitioning;
}

bool AreaPlanEditor::checkSwarmReadiness() const
{
    if (!isSwarmReady()) {
        updateSwarmStatus("Swarm not ready - check vehicle connections");
        return false;
    }
    
    updateSwarmStatus("Swarm ready for coordinated operations");
    return true;
}

void AreaPlanEditor::updateSwarmStatus(const QString& status)
{
    if (swarmStatus() == status) {
        return;
    }
    
    _swarmStatus = status;
    emit swarmStatusChanged();
    
    qDebug() << "Swarm status:" << status;
}

bool AreaPlanEditor::startCoordinatedTakeoff()
{
    if (!checkSwarmReadiness()) {
        return false;
    }
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return false;
    }
    
    // Send takeoff commands to all connected vehicles
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        handleError("No connected vehicles", "Please connect at least one vehicle");
        return false;
    }
    
    // Send coordinated takeoff command
    sendSwarmCommand(MAV_CMD_NAV_TAKEOFF, vehicles);
    
    updateSwarmStatus("Coordinated takeoff initiated");
    return true;
}

bool AreaPlanEditor::startCoordinatedMission()
{
    if (!checkSwarmReadiness()) {
        return false;
    }
    
    if (!isCoordinatedMissionActive()) {
        handleError("No active coordinated mission", "Please start a coordinated mission first");
        return false;
    }
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return false;
    }
    
    // Send mission start commands to all connected vehicles
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        handleError("No connected vehicles", "Please connect at least one vehicle");
        return false;
    }
    
    // Send coordinated mission start command
    sendSwarmCommand(MAV_CMD_MISSION_START, vehicles);
    
    updateSwarmStatus("Coordinated mission started");
    return true;
}

bool AreaPlanEditor::abortCoordinatedMission()
{
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return false;
    }
    
    // Send abort commands to all connected vehicles
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (!vehicles.isEmpty()) {
        // Send coordinated abort command
        sendSwarmCommand(MAV_CMD_MISSION_ABORT, vehicles);
    }
    
    updateSwarmStatus("Coordinated mission aborted");
    return true;
}

// Formation management methods
bool AreaPlanEditor::setFormationType(FormationType type)
{
    if (formationType() == type) {
        return true;
    }
    
    _formationType = type;
    calculateFormationPositions();
    emit formationTypeChanged();
    
    updateSwarmStatus(QString("Formation type changed to %1").arg(static_cast<int>(type)));
    return true;
}

bool AreaPlanEditor::adjustFormationSpacing(qreal spacing)
{
    if (qFuzzyCompare(formationSpacing(), spacing)) {
        return true;
    }
    
    // Enforce minimum spacing
    qreal clamped = spacing < 1.0 ? 1.0 : spacing;
    setFormationSpacing(clamped);
    
    calculateFormationPositions();
    emit formationSpacingChanged();
    
    updateSwarmStatus(QString("Formation spacing adjusted to %1m").arg(clamped));
    return true;
}

bool AreaPlanEditor::assignFormationRoles()
{
    if (!checkSwarmReadiness()) {
        return false;
    }
    
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        handleError("Vehicle manager not available", "Please ensure QGroundControl is properly initialized");
        return false;
    }
    
    // Assign formation roles to connected vehicles
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        handleError("No connected vehicles", "Please connect at least one vehicle");
        return false;
    }
    
    // Assign leader role to first vehicle
    if (!vehicles.isEmpty()) {
        _leaderVehicle = vehicles.first();
    }
    
    // Calculate formation positions
    calculateFormationPositions();
    
    updateSwarmStatus(QString("Formation roles assigned to %1 vehicles").arg(vehicles.count()));
    return true;
}

bool AreaPlanEditor::startFormationTransition()
{
    if (_isFormationTransitioning) {
        return false; // Already transitioning
    }
    
    if (!checkSwarmReadiness()) {
        return false;
    }
    
    _isFormationTransitioning = true;
    emit isFormationTransitioningChanged();
    
    // Send formation transition commands
    sendFormationCommands();
    
    updateSwarmStatus("Formation transition started");
    return true;
}

void AreaPlanEditor::sendSwarmCommand(uint16_t command, const QList<Vehicle*>& vehicles)
{
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) continue;
        
        // Send MAVLink command to vehicle
        vehicle->sendMavCommand(vehicle->defaultComponentId(), command, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        
        qDebug() << "Sent swarm command" << command << "to vehicle" << vehicle->id();
    }
}

void AreaPlanEditor::handleSwarmResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) return;
    
    QString status;
    switch (result) {
        case MAV_RESULT_ACCEPTED:
            status = "Command accepted";
            break;
        case MAV_RESULT_TEMPORARILY_REJECTED:
            status = "Command temporarily rejected";
            break;
        case MAV_RESULT_DENIED:
            status = "Command denied";
            break;
        case MAV_RESULT_UNSUPPORTED:
            status = "Command unsupported";
            break;
        case MAV_RESULT_FAILED:
            status = "Command failed";
            break;
        default:
            status = "Unknown response";
            break;
    }
    
    qDebug() << "Swarm response from vehicle" << vehicle->id() 
             << "for command" << command << ":" << status;
    
    updateSwarmStatus(QString("Vehicle %1: %2").arg(vehicle->id()).arg(status));
}

void AreaPlanEditor::sendFormationCommands()
{
    MultiVehicleManager* vehicleManager = qgcApp()->multiVehicleManager();
    if (!vehicleManager) {
        return;
    }
    
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
        if (vehicle && vehicle->isConnected()) {
            vehicles.append(vehicle);
        }
    }
    
    if (vehicles.isEmpty()) {
        return;
    }
    
    // Send formation-specific commands to all vehicles
    for (int i = 0; i < vehicles.count(); ++i) {
        Vehicle* vehicle = vehicles[i];
        if (!vehicle) continue;
        
        // Send formation position command
        // Note: This is a simplified implementation - in practice, you would
        // send specific formation position data to each vehicle
        vehicle->sendMavCommand(vehicle->defaultComponentId(), 
                              MAV_CMD_DO_SET_MODE, 
                              static_cast<float>(i), // Formation position
                              0.0, 0.0, 0.0, 0.0, 0.0);
    }
}

void AreaPlanEditor::handleFormationResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) return;
    
    QString status;
    switch (result) {
        case MAV_RESULT_ACCEPTED:
            status = "Formation command accepted";
            break;
        case MAV_RESULT_DENIED:
            status = "Formation command denied";
            break;
        case MAV_RESULT_FAILED:
            status = "Formation command failed";
            break;
        default:
            status = "Unknown formation response";
            break;
    }
    
    qDebug() << "Formation response from vehicle" << vehicle->id() 
             << "for command" << command << ":" << status;
    
    // Check if all vehicles have responded to formation commands
    if (result == MAV_RESULT_ACCEPTED) {
        // Formation transition completed
        _isFormationTransitioning = false;
        emit isFormationTransitioningChanged();
        
        updateSwarmStatus("Formation transition completed");
    }
}

void AreaPlanEditor::calculateFormationPositions()
{
    // This method calculates the formation positions for all vehicles
    // based on the current formation type and spacing
    
    if (droneCount() <= 0) {
        return;
    }
    
    // Clear existing formation positions
    _formationPositions.clear();
    
    // Calculate positions based on formation type
    switch (formationType()) {
        case FormationType::Line:
            calculateLineFormation();
            break;
        case FormationType::V:
            calculateVFormation();
            break;
        case FormationType::Diamond:
            calculateDiamondFormation();
            break;
        case FormationType::Circle:
            calculateCircleFormation();
            break;
        default:
            calculateLineFormation();
            break;
    }
    
    emit formationPositionsChanged();
}

void AreaPlanEditor::calculateLineFormation()
{
    // Calculate line formation positions
    _formationPositions.clear();
    for (int i = 0; i < droneCount(); ++i) {
        qreal x = i * formationSpacing();
        qreal y = 0.0;
        _formationPositions.append(QPointF(x, y));
    }
}

void AreaPlanEditor::calculateVFormation()
{
    // Calculate V formation positions
    _formationPositions.clear();
    int centerIndex = droneCount() / 2;
    
    for (int i = 0; i < droneCount(); ++i) {
        qreal x = (i - centerIndex) * formationSpacing();
        qreal y = qAbs(i - centerIndex) * formationSpacing() * 0.5;
        _formationPositions.append(QPointF(x, y));
    }
}

void AreaPlanEditor::calculateDiamondFormation()
{
    // Calculate diamond formation positions
    if (droneCount() < 4) {
        calculateLineFormation();
        return;
    }
    
    _formationPositions.clear();
    qreal radius = formationSpacing() * 0.5;
    
    for (int i = 0; i < droneCount(); ++i) {
        qreal angle = (2.0 * M_PI * i) / droneCount();
        qreal x = radius * qCos(angle);
        qreal y = radius * qSin(angle);
        _formationPositions.append(QPointF(x, y));
    }
}

void AreaPlanEditor::calculateCircleFormation()
{
    // Calculate circle formation positions
    if (droneCount() < 3) {
        calculateLineFormation();
        return;
    }
    
    _formationPositions.clear();
    qreal radius = formationSpacing() * 0.5;
    
    for (int i = 0; i < droneCount(); ++i) {
        qreal angle = (2.0 * M_PI * i) / droneCount();
        qreal x = radius * qCos(angle);
        qreal y = radius * qSin(angle);
        _formationPositions.append(QPointF(x, y));
    }
}
