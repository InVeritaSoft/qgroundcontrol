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

/**
 * @file AreaPlanEditor_Utilities.cc
 * @brief Utility functions and helpers for AreaPlanEditor
 * 
 * This file contains all utility functions, area manipulation methods,
 * and helper functions for the AreaPlanEditor class.
 */

// Area manipulation methods
void AreaPlanEditor::moveAreaNorth()
{
    if (!_areaCenter.isValid()) {
        handleError("Area center not set", "Please set a valid area center first");
        return;
    }
    
    // Move area center 100 meters north
    const qreal metersPerDegreeLat = 111319.9;
    const qreal latOffset = 100.0 / metersPerDegreeLat;
    
    QGeoCoordinate newCenter(_areaCenter.latitude() + latOffset, 
                            _areaCenter.longitude(), 
                            _areaCenter.altitude());
    
    setAreaCenter(newCenter);
    updateStatus("Area moved north");
}

void AreaPlanEditor::moveAreaSouth()
{
    if (!_areaCenter.isValid()) {
        handleError("Area center not set", "Please set a valid area center first");
        return;
    }
    
    // Move area center 100 meters south
    const qreal metersPerDegreeLat = 111319.9;
    const qreal latOffset = -100.0 / metersPerDegreeLat;
    
    QGeoCoordinate newCenter(_areaCenter.latitude() + latOffset, 
                            _areaCenter.longitude(), 
                            _areaCenter.altitude());
    
    setAreaCenter(newCenter);
    updateStatus("Area moved south");
}

void AreaPlanEditor::moveAreaEast()
{
    if (!_areaCenter.isValid()) {
        handleError("Area center not set", "Please set a valid area center first");
        return;
    }
    
    // Move area center 100 meters east
    const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(_areaCenter.latitude()));
    const qreal lonOffset = 100.0 / metersPerDegreeLon;
    
    QGeoCoordinate newCenter(_areaCenter.latitude(), 
                            _areaCenter.longitude() + lonOffset, 
                            _areaCenter.altitude());
    
    setAreaCenter(newCenter);
    updateStatus("Area moved east");
}

void AreaPlanEditor::moveAreaWest()
{
    if (!_areaCenter.isValid()) {
        handleError("Area center not set", "Please set a valid area center first");
        return;
    }
    
    // Move area center 100 meters west
    const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(_areaCenter.latitude()));
    const qreal lonOffset = -100.0 / metersPerDegreeLon;
    
    QGeoCoordinate newCenter(_areaCenter.latitude(), 
                            _areaCenter.longitude() + lonOffset, 
                            _areaCenter.altitude());
    
    setAreaCenter(newCenter);
    updateStatus("Area moved west");
}

void AreaPlanEditor::rotateAreaClockwise()
{
    qreal newRotation = _areaRotation + 15.0; // Rotate by 15 degrees
    if (newRotation >= 360.0) {
        newRotation -= 360.0;
    }
    
    setAreaRotation(newRotation);
    updateStatus(QString("Area rotated clockwise to %1 degrees").arg(newRotation));
}

void AreaPlanEditor::rotateAreaCounterClockwise()
{
    qreal newRotation = _areaRotation - 15.0; // Rotate by 15 degrees
    if (newRotation < 0.0) {
        newRotation += 360.0;
    }
    
    setAreaRotation(newRotation);
    updateStatus(QString("Area rotated counter-clockwise to %1 degrees").arg(newRotation));
}

void AreaPlanEditor::centerArea()
{
    if (!_homeLocation.isValid()) {
        handleError("Home location not set", "Please set a valid home location first");
        return;
    }
    
    setAreaCenter(_homeLocation);
    updateStatus("Area centered on home location");
}

void AreaPlanEditor::resetArea()
{
    // Reset all area parameters to default values
    setAreaWidth(1000.0);
    setAreaHeight(1000.0);
    setLineSpacing(50.0);
    setNumPoints(10);
    setMissionAltitude(100.0);
    setAreaRotation(0.0);
    setLoiterTime(5.0);
    
    // Reset multi-drone parameters
    setDroneCount(1);
    setAltitudeBandStart(0.0);
    setAltitudeBandStep(10.0);
    setTimeOffsetPerDrone(5.0);
    setPerTargetSeparationS(2.0);
    setRtlAfterEveryWaypoint(false);
    setLoiterAfterRtl(false);
    setTargetHoldTimeS(1.0);
    setHomeTurnaroundWaitS(2.0);
    setPayloadReleaseEnabled(false);
    setTakeoffHeight(10.0);
    setFormationSpacing(5.0);
    
    // Clear area center and home location
    setAreaCenter(QGeoCoordinate());
    setHomeLocation(QGeoCoordinate());
    
    // Clear any stored missions
    clearAllStoredMissions();
    
    updateStatus("Area parameters reset to defaults");
}

// Calculation utility methods
QGeoCoordinate AreaPlanEditor::calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const
{
    // Convert meters to degrees using approximate conversion factors
    // These factors are approximate for small distances
    const qreal metersPerDegreeLat = 111319.9;  // meters per degree of latitude
    const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(coord.latitude()));  // meters per degree of longitude

    qreal latOffset = meters * qCos(qDegreesToRadians(bearing)) / metersPerDegreeLat;
    qreal lonOffset = meters * qSin(qDegreesToRadians(bearing)) / metersPerDegreeLon;

    return QGeoCoordinate(coord.latitude() + latOffset, coord.longitude() + lonOffset);
}

int AreaPlanEditor::calculateTotalWaypoints() const
{
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) {
        return 0;
    }
    
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    return lineCount * numPoints() * droneCount();
}

int AreaPlanEditor::calculateFlightTime() const
{
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0) {
        return 0;
    }
    
    // Calculate total distance
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const qreal totalDistance = lineCount * _areaWidth;
    
    // Assume average speed of 10 m/s (36 km/h)
    const qreal averageSpeed = 10.0; // m/s
    const qreal flightTimeSeconds = totalDistance / averageSpeed;
    
    // Add time for turns and altitude changes
    const qreal turnTime = lineCount * 5.0; // 5 seconds per turn
    const qreal altitudeTime = droneCount() * 10.0; // 10 seconds per altitude change
    
    return static_cast<int>(flightTimeSeconds + turnTime + altitudeTime);
}

// Test and debugging methods
void AreaPlanEditor::testCompleteWorkflow()
{
    updateStatus("Starting complete workflow test");
    
    // Test 1: Validate area parameters
    if (!validateAreaParameters()) {
        handleError("Area parameter validation failed", "Please check area parameters");
        return;
    }
    
    // Test 2: Generate waypoints
    QList<QVariant> waypoints = generateWaypoints();
    if (waypoints.isEmpty()) {
        handleError("Waypoint generation failed", "Please check area parameters");
        return;
    }
    
    updateStatus(QString("Generated %1 waypoints").arg(waypoints.count()));
    
    // Test 3: Validate waypoint generation
    if (!validateWaypointGeneration()) {
        handleError("Waypoint generation validation failed", "Please check waypoint parameters");
        return;
    }
    
    // Test 4: Test mission upload validation
    if (!validateMissionUpload()) {
        handleError("Mission upload validation failed", "Please check vehicle connections");
        return;
    }
    
    // Test 5: Test swarm configuration
    if (!validateSwarmConfiguration()) {
        handleError("Swarm configuration validation failed", "Please check swarm parameters");
        return;
    }
    
    // Test 6: Calculate statistics
    int totalWaypoints = calculateTotalWaypoints();
    int flightTime = calculateFlightTime();
    
    updateStatus(QString("Test completed successfully: %1 waypoints, %2 seconds flight time")
                .arg(totalWaypoints)
                .arg(flightTime));
    
    qDebug() << "Complete workflow test passed";
}

// Progress and status methods
void AreaPlanEditor::startProgress(const QString& operation, const QString& message)
{
    _currentOperation = operation;
    _progressMessage = message;
    _progressValue = 0;
    _isProcessing = true;
    
    emit currentOperationChanged();
    emit progressMessageChanged();
    emit progressValueChanged();
    emit isProcessingChanged();
    
    qDebug() << "Progress started:" << operation << "-" << message;
}

void AreaPlanEditor::updateProgress(int value, const QString& message)
{
    _progressValue = qBound(0, value, 100);
    if (!message.isEmpty()) {
        _progressMessage = message;
        emit progressMessageChanged();
    }
    
    emit progressValueChanged();
    
    qDebug() << "Progress updated:" << _progressValue << "%" << _progressMessage;
}

void AreaPlanEditor::finishProgress(const QString& message)
{
    _progressValue = 100;
    if (!message.isEmpty()) {
        _progressMessage = message;
    } else {
        _progressMessage = "Operation completed";
    }
    
    _isProcessing = false;
    
    emit progressValueChanged();
    emit progressMessageChanged();
    emit isProcessingChanged();
    
    qDebug() << "Progress finished:" << _progressMessage;
}

void AreaPlanEditor::cancelProgress()
{
    _progressValue = 0;
    _progressMessage = "Operation cancelled";
    _isProcessing = false;
    
    emit progressValueChanged();
    emit progressMessageChanged();
    emit isProcessingChanged();
    
    qDebug() << "Progress cancelled";
}

void AreaPlanEditor::updateStatus(const QString& message)
{
    setStatusMessage(message);
    
    qDebug() << "Status update:" << message;
}
