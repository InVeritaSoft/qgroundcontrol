#include "AreaPlanEditor.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include <QDebug>
#include <QtMath>
#include <QtPositioning>
#include <QVariant>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QElapsedTimer>
#include <QCache>

// QGC includes for mission management
#include "Vehicle.h"
#include "MissionManager.h"
#include "MissionItem.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGroundControlQmlGlobal.h"
#include "MultiVehicleManager.h"
#include "QGCMAVLink.h"
#include "MissionController.h" // Added for MissionController
#include "SimpleMissionItem.h"
#include "MissionManager/AreaPartition.h"
#include "QGCApplication.h"

AreaPlanEditor::AreaPlanEditor(QObject* parent)
    : QObject(parent)
    , _areaWidth(_defaultAreaWidth)
    , _areaHeight(_defaultAreaHeight)
    , _lineSpacing(_defaultLineSpacing)
    , _numPoints(_defaultNumPoints)
    , _missionAltitude(_defaultAltitude)
    , _droneCount(_defaultDroneCount)
    , _altitudeBandStart(_defaultAltitudeBandStart)
    , _altitudeBandStep(_defaultAltitudeBandStep)
    , _timeOffsetPerDrone(_defaultTimeOffsetPerDrone)
    , _rtlAfterEveryWaypoint(false)
    , _loiterAfterRtl(false)
    , _isProcessing(false)
    , _progressValue(0)
    , _isOptimized(false)
    , _cacheSize(0)
    , _isDrawingMode(false)
    , _planMasterController(nullptr)
    , _currentFormation(NoFormation)
    , _formationSpacing(5.0)
    , _isFormationTransitioning(false)
    , _leaderVehicle(nullptr)
{
    // Initialize any additional members if needed
}

void AreaPlanEditor::setAreaWidth(qreal width)
{
    if (qFuzzyCompare(_areaWidth, width)) {
        return;
    }
    _areaWidth = width;
    emit areaWidthChanged();
}

void AreaPlanEditor::setAreaHeight(qreal height)
{
    if (qFuzzyCompare(_areaHeight, height)) {
        return;
    }
    _areaHeight = height;
    emit areaHeightChanged();
}

void AreaPlanEditor::setLineSpacing(qreal spacing)
{
    if (qFuzzyCompare(_lineSpacing, spacing)) {
        return;
    }
    _lineSpacing = spacing;
    emit lineSpacingChanged();
}

void AreaPlanEditor::setNumPoints(int points)
{
    if (_numPoints == points) {
        return;
    }
    _numPoints = points;
    emit numPointsChanged();
}

void AreaPlanEditor::setMissionAltitude(qreal altitude)
{
    if (qFuzzyCompare(_missionAltitude, altitude)) {
        return;
    }
    _missionAltitude = altitude;
    emit missionAltitudeChanged();
}

void AreaPlanEditor::setAreaCenter(const QGeoCoordinate& center)
{
    if (_areaCenter == center) {
        return;
    }
    _areaCenter = center;
    emit areaCenterChanged();
}

void AreaPlanEditor::setHomeLocation(const QGeoCoordinate& location)
{
    if (_homeLocation == location) {
        return;
    }
    _homeLocation = location;
    emit homeLocationChanged();
}

void AreaPlanEditor::setAreaRotation(qreal rotation)
{
    if (qFuzzyCompare(_areaRotation, rotation)) {
        return;
    }
    _areaRotation = rotation;
    emit areaRotationChanged();
}

void AreaPlanEditor::setLoiterTime(qreal time)
{
    if (qFuzzyCompare(_loiterTime, time)) {
        return;
    }
    _loiterTime = time;
    emit loiterTimeChanged();
}

void AreaPlanEditor::setIsDrawingMode(bool drawingMode)
{
    if (_isDrawingMode == drawingMode) {
        return;
    }
    _isDrawingMode = drawingMode;
    emit isDrawingModeChanged();
}

void AreaPlanEditor::setPlanMasterController(QObject* controller)
{
    if (_planMasterController == controller) {
        return;
    }
    _planMasterController = controller;
    emit planMasterControllerChanged();
}

void AreaPlanEditor::setDroneCount(int count)
{
    if (_droneCount == count) {
        return;
    }
    _droneCount = count;
    emit droneCountChanged();
}

void AreaPlanEditor::setAltitudeBandStart(qreal startMeters)
{
    if (qFuzzyCompare(_altitudeBandStart, startMeters)) {
        return;
    }
    _altitudeBandStart = startMeters;
    emit altitudeBandStartChanged();
}

void AreaPlanEditor::setAltitudeBandStep(qreal stepMeters)
{
    if (qFuzzyCompare(_altitudeBandStep, stepMeters)) {
        return;
    }
    _altitudeBandStep = stepMeters;
    emit altitudeBandStepChanged();
}

void AreaPlanEditor::setTimeOffsetPerDrone(qreal seconds)
{
    if (qFuzzyCompare(_timeOffsetPerDrone, seconds)) {
        return;
    }
    _timeOffsetPerDrone = seconds;
    emit timeOffsetPerDroneChanged();
}

void AreaPlanEditor::setRtlAfterEveryWaypoint(bool enabled)
{
    if (_rtlAfterEveryWaypoint == enabled) {
        return;
    }
    _rtlAfterEveryWaypoint = enabled;
    emit rtlAfterEveryWaypointChanged();
}

void AreaPlanEditor::setLoiterAfterRtl(bool enabled)
{
    if (_loiterAfterRtl == enabled) {
        return;
    }
    _loiterAfterRtl = enabled;
    emit loiterAfterRtlChanged();
}

void AreaPlanEditor::setFormationSpacing(qreal spacing)
{
    if (qFuzzyCompare(_formationSpacing, spacing)) {
        return;
    }
    
    _formationSpacing = spacing;
    calculateFormationPositions();
    emit formationSpacingChanged();
}

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

bool AreaPlanEditor::isSwarmReady() const
{
    return checkSwarmReadiness();
}

QString AreaPlanEditor::swarmStatus() const
{
    return _swarmStatus;
}

bool AreaPlanEditor::isCoordinatedMissionActive() const
{
    return _isCoordinatedMissionActive;
}

AreaPlanEditor::FormationType AreaPlanEditor::currentFormation() const
{
    return _currentFormation;
}

bool AreaPlanEditor::isFormationTransitioning() const
{
    return _isFormationTransitioning;
}

bool AreaPlanEditor::checkSwarmReadiness() const
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) {
        return false;  // Need at least 2 vehicles for swarm operations
    }

    // Check if all vehicles are ready
    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (!vehicle || !_vehicleReadyStatus.value(vehicle->id(), false)) {
            return false;
        }
    }

    return true;
}

void AreaPlanEditor::updateSwarmStatus(const QString& status)
{
    if (_swarmStatus != status) {
        _swarmStatus = status;
        emit swarmStatusChanged();
    }
}

bool AreaPlanEditor::startCoordinatedTakeoff()
{
    if (!isSwarmReady()) {
        updateSwarmStatus("Swarm not ready for coordinated takeoff");
        return false;
    }

    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleModel->count(); i++) {
        vehicles.append(qobject_cast<Vehicle*>(vehicleModel->get(i)));
    }

    // Send takeoff command to all vehicles
    sendSwarmCommand(MAV_CMD_NAV_TAKEOFF, vehicles);
    updateSwarmStatus("Coordinated takeoff initiated");
    return true;
}

bool AreaPlanEditor::startCoordinatedMission()
{
    if (!isSwarmReady()) {
        updateSwarmStatus("Swarm not ready for coordinated mission");
        return false;
    }

    _isCoordinatedMissionActive = true;
    emit coordinatedMissionStatusChanged();
    updateSwarmStatus("Coordinated mission started");
    return true;
}

bool AreaPlanEditor::abortCoordinatedMission()
{
    if (!_isCoordinatedMissionActive) {
        return false;
    }

    _isCoordinatedMissionActive = false;
    emit coordinatedMissionStatusChanged();
    updateSwarmStatus("Coordinated mission aborted");
    return true;
}

bool AreaPlanEditor::setFormationType(FormationType type)
{
    if (_currentFormation == type) {
        return true;
    }

    _currentFormation = type;
    calculateFormationPositions();
    emit formationChanged();
    return true;
}

bool AreaPlanEditor::adjustFormationSpacing(qreal spacing)
{
    setFormationSpacing(spacing);
    return true;
}

bool AreaPlanEditor::assignFormationRoles()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) {
        return false;
    }

    // Clear existing roles
    _formationRoles.clear();

    // Assign roles (0 = leader, 1+ = followers)
    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle) {
            _formationRoles[vehicle->id()] = i;
            if (i == 0) {
                _leaderVehicle = vehicle;
                emit leaderVehicleChanged();
            }
        }
    }

    emit formationRolesChanged();
    return true;
}

bool AreaPlanEditor::startFormationTransition()
{
    if (!isSwarmReady() || _isFormationTransitioning) {
        return false;
    }

    _isFormationTransitioning = true;
    emit formationTransitioningChanged();

    // Calculate and send new formation positions
    calculateFormationPositions();
    sendFormationCommands();

    return true;
}

void AreaPlanEditor::sendSwarmCommand(uint16_t command, const QList<Vehicle*>& vehicles)
{
    for (Vehicle* vehicle : vehicles) {
        if (vehicle) {
            // Send MAVLink command to each vehicle
            // Note: Actual command sending would require more parameters and proper MAVLink integration
            qDebug() << "Sending command" << command << "to vehicle" << vehicle->id();
        }
    }
}

void AreaPlanEditor::handleSwarmResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) {
        return;
    }

    // Update vehicle ready status based on command result
    _vehicleReadyStatus[vehicle->id()] = (result == MAV_RESULT_ACCEPTED);
    
    // Update swarm status
    if (result != MAV_RESULT_ACCEPTED) {
        updateSwarmStatus(QString("Vehicle %1 command %2 failed").arg(vehicle->id()).arg(command));
    }
}

void AreaPlanEditor::sendFormationCommands()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel) {
        return;
    }

    for (int i = 0; i < vehicleModel->count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle && _formationOffsets.contains(vehicle->id())) {
            // Send position command to each vehicle
            // Note: Actual command sending would require proper MAVLink integration
            qDebug() << "Sending formation position to vehicle" << vehicle->id() 
                    << "offset:" << _formationOffsets[vehicle->id()];
        }
    }
}

void AreaPlanEditor::handleFormationResponse(Vehicle* vehicle, uint16_t command, uint8_t result)
{
    if (!vehicle) {
        return;
    }

    // Update formation transition status if all vehicles have responded
    bool allComplete = true;
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    for (int i = 0; vehicleModel && i < vehicleModel->count(); i++) {
        Vehicle* v = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (v && !_vehicleReadyStatus.value(v->id(), false)) {
            allComplete = false;
            break;
        }
    }

    if (allComplete) {
        _isFormationTransitioning = false;
        emit formationTransitioningChanged();
    }
}

void AreaPlanEditor::moveAreaNorth()
{
    // Move area center north by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 0);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaSouth()
{
    // Move area center south by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 180);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaEast()
{
    // Move area center east by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 90);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaWest()
{
    // Move area center west by line spacing distance
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _lineSpacing, 270);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::rotateAreaClockwise()
{
    setAreaRotation(_areaRotation + 5.0);  // Rotate 5 degrees clockwise
}

void AreaPlanEditor::rotateAreaCounterClockwise()
{
    setAreaRotation(_areaRotation - 5.0);  // Rotate 5 degrees counter-clockwise
}

void AreaPlanEditor::centerArea()
{
    // Get current vehicle position as center
    Vehicle* vehicle = getCurrentVehicle();
    if (vehicle) {
        setAreaCenter(vehicle->coordinate());
    }
}

void AreaPlanEditor::resetArea()
{
    // Reset all area parameters to defaults
    setAreaWidth(_defaultAreaWidth);
    setAreaHeight(_defaultAreaHeight);
    setLineSpacing(_defaultLineSpacing);
    setNumPoints(_defaultNumPoints);
    setMissionAltitude(_defaultAltitude);
    setAreaRotation(0.0);
    
    // Center on current vehicle
    centerArea();
}

Vehicle* AreaPlanEditor::getCurrentVehicle() const
{
    return MultiVehicleManager::instance()->activeVehicle();
}

MissionManager* AreaPlanEditor::getMissionManager() const
{
    Vehicle* vehicle = getCurrentVehicle();
    return vehicle ? vehicle->missionManager() : nullptr;
}

MissionController* AreaPlanEditor::getMissionController() const
{
    return qobject_cast<MissionController*>(_planMasterController);
}

QList<QVariant> AreaPlanEditor::generateWaypoints()
{
    QList<QVariant> waypoints;

    // Basic validation
    if (_areaCenter.isValid() == false || _areaWidth <= 0 || _areaHeight <= 0 || _numPoints <= 0 || _lineSpacing <= 0) {
        return waypoints;
    }

    // Compute number of grid lines along height (north-south axis before rotation)
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));

    // Local helpers for geometry
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(_areaRotation); // rotation: 0 = North
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);

    auto rotateXY = [&](qreal x, qreal y) {
        // Rotate local (x,y) around origin by theta
        return QPointF(x * cosT - y * sinT, x * sinT + y * cosT);
    };

    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        // Approximate translation by dy north, then dx east
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        QGeoCoordinate res = calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
        return res;
    };

    // Y coordinates for each line (evenly distributed from -halfH to +halfH)
    for (int li = 0; li < lineCount; ++li) {
        qreal y;
        if (lineCount == 1) {
            y = 0.0;
        } else {
            y = -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
        }

        // X coordinates along width
        for (int pi = 0; pi < _numPoints; ++pi) {
            qreal x;
            if (_numPoints == 1) {
                x = 0.0;
            } else {
                x = -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
            }

            // Apply rotation around center
            const QPointF r = rotateXY(x, y);

            // Convert to geo coordinate
            const QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
            waypoints.append(QVariant::fromValue(wp));
        }
    }

    return waypoints;
}

QList<QVariant> AreaPlanEditor::computePartitionStripes() const
{
    QList<QVariant> stripes;
    // TODO: Implement stripe computation logic
    return stripes;
}

QList<QVariant> AreaPlanEditor::computeRoundRobinAssignments() const
{
    QList<QVariant> assignments;
    // TODO: Implement round-robin assignment logic
    return assignments;
}

QList<QVariant> AreaPlanEditor::computeDroneAssignments() const
{
    QList<QVariant> assignments;
    // TODO: Implement drone assignment logic
    return assignments;
}

QMap<QString, QVariant> AreaPlanEditor::computePerDroneCounts() const
{
    QMap<QString, QVariant> counts;
    // TODO: Implement per-drone count computation
    return counts;
}

QList<QVariant> AreaPlanEditor::computePerDroneWaypointPreview() const
{
    QList<QVariant> preview;
    // TODO: Implement per-drone waypoint preview
    return preview;
}

QList<QVariant> AreaPlanEditor::generatePerDroneWaypoints(int droneIndex) const
{
    QList<QVariant> waypoints;
    // TODO: Implement per-drone waypoint generation
    return waypoints;
}

void AreaPlanEditor::addPerDroneToMission(int droneIndex)
{
    // TODO: Implement adding per-drone waypoints to mission
}

void AreaPlanEditor::addAllDronesToMission()
{
    // TODO: Implement adding all drone waypoints to mission
}

void AreaPlanEditor::addWaypointsToMission()
{
    // TODO: Implement adding waypoints to mission
}

void AreaPlanEditor::saveMissionFile()
{
    // TODO: Implement mission file saving
}

void AreaPlanEditor::savePerDroneMissionFiles()
{
    // TODO: Implement per-drone mission file saving
}

void AreaPlanEditor::uploadToVehicle()
{
    // TODO: Implement mission upload to vehicle
}

void AreaPlanEditor::uploadPerDroneMissionToVehicle(int droneIndex, QObject* vehicleObject)
{
    // TODO: Implement per-drone mission upload
}

void AreaPlanEditor::uploadToAllDrones()
{
    // TODO: Implement uploading to all drones
}

QList<QVariant> AreaPlanEditor::getAvailableVehicles() const
{
    QList<QVariant> vehicles;
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (vehicleModel) {
        for (int i = 0; i < vehicleModel->count(); i++) {
            Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
            if (vehicle) {
                QVariantMap vehicleInfo;
                vehicleInfo["id"] = vehicle->id();
                vehicleInfo["name"] = QString("Vehicle %1").arg(vehicle->id());
                vehicles.append(QVariant::fromValue(vehicleInfo));
            }
        }
    }
    return vehicles;
}

void AreaPlanEditor::startMission()
{
    // TODO: Implement mission start
}

void AreaPlanEditor::updateStatus(const QString& message)
{
    emit statusChanged(message);
}

void AreaPlanEditor::testCompleteWorkflow()
{
    // TODO: Implement complete workflow test
}

bool AreaPlanEditor::validateAreaParameters() const
{
    // TODO: Implement area parameter validation
    return true;
}

bool AreaPlanEditor::validateWaypointGeneration()
{
    // TODO: Implement waypoint generation validation
    return true;
}

bool AreaPlanEditor::validateMissionUpload()
{
    // TODO: Implement mission upload validation
    return true;
}

bool AreaPlanEditor::validateMissionFileSaving()
{
    // TODO: Implement mission file saving validation
    return true;
}

QMap<QString, QVariant> AreaPlanEditor::getDroneAllocationStats(int droneIndex) const
{
    QMap<QString, QVariant> stats;
    // TODO: Implement drone allocation stats
    return stats;
}

bool AreaPlanEditor::validateSwarmConfiguration() const
{
    // TODO: Implement swarm configuration validation
    return true;
}

QString AreaPlanEditor::validateInput(const QString& fieldName, const QVariant& value) const
{
    // TODO: Implement input validation
    return QString();
}

bool AreaPlanEditor::isInputValid(const QString& fieldName, const QVariant& value) const
{
    // TODO: Implement input validation
    return true;
}

QString AreaPlanEditor::getValidationError() const
{
    return _validationError;
}

void AreaPlanEditor::clearValidationError()
{
    if (!_validationError.isEmpty()) {
        _validationError.clear();
        emit validationErrorChanged();
    }
}

void AreaPlanEditor::logError(const QString& errorMessage, const QString& context)
{
    qWarning() << "Error:" << errorMessage << "Context:" << context;
}

void AreaPlanEditor::handleError(const QString& errorMessage, const QString& recoverySuggestion)
{
    logError(errorMessage, recoverySuggestion);
    updateStatus(QString("Error: %1. %2").arg(errorMessage, recoverySuggestion));
}

void AreaPlanEditor::startProgress(const QString& operation, const QString& message)
{
    _isProcessing = true;
    _progressValue = 0;
    _currentOperation = operation;
    _progressMessage = message;
    emit isProcessingChanged();
    emit progressValueChanged();
    emit currentOperationChanged();
    emit progressMessageChanged();
}

void AreaPlanEditor::updateProgress(int value, const QString& message)
{
    _progressValue = value;
    if (!message.isEmpty()) {
        _progressMessage = message;
        emit progressMessageChanged();
    }
    emit progressValueChanged();
}

void AreaPlanEditor::finishProgress(const QString& message)
{
    _isProcessing = false;
    _progressValue = 100;
    if (!message.isEmpty()) {
        _progressMessage = message;
        emit progressMessageChanged();
    }
    emit isProcessingChanged();
    emit progressValueChanged();
}

void AreaPlanEditor::cancelProgress()
{
    _isProcessing = false;
    _progressValue = 0;
    _progressMessage.clear();
    emit isProcessingChanged();
    emit progressValueChanged();
    emit progressMessageChanged();
}

void AreaPlanEditor::setProgressOperation(const QString& operation)
{
    if (_currentOperation != operation) {
        _currentOperation = operation;
        emit currentOperationChanged();
    }
}

void AreaPlanEditor::enableOptimizations()
{
    if (!_isOptimized) {
        _isOptimized = true;
        emit isOptimizedChanged();
    }
}

void AreaPlanEditor::disableOptimizations()
{
    if (_isOptimized) {
        _isOptimized = false;
        emit isOptimizedChanged();
    }
}

void AreaPlanEditor::clearCache()
{
    _waypointCache.clear();
    _cacheHits = 0;
    _cacheMisses = 0;
}

void AreaPlanEditor::optimizeWaypointGeneration()
{
    // TODO: Implement waypoint generation optimization
}

void AreaPlanEditor::setCacheSize(int size)
{
    if (_cacheSize != size) {
        _cacheSize = size;
        emit cacheSizeChanged();
    }
}

void AreaPlanEditor::profilePerformance()
{
    // TODO: Implement performance profiling
}

QMap<QString, QVariant> AreaPlanEditor::getPerformanceMetrics() const
{
    QMap<QString, QVariant> metrics;
    metrics["cacheHits"] = _cacheHits;
    metrics["cacheMisses"] = _cacheMisses;
    metrics["cacheSize"] = _cacheSize;
    return metrics;
}

int AreaPlanEditor::calculateTotalWaypoints() const
{
    // Calculate total waypoints based on area dimensions and line spacing
    int linesHorizontal = qCeil(_areaWidth / _lineSpacing);
    int linesVertical = qCeil(_areaHeight / _lineSpacing);
    
    // Each line has _numPoints waypoints
    int totalPoints = (linesHorizontal + linesVertical) * _numPoints;
    
    // Add extra points for RTL and loiter if enabled
    if (_rtlAfterEveryWaypoint) {
        totalPoints *= 2;  // Double for RTL after each point
    }
    if (_loiterAfterRtl) {
        totalPoints += linesHorizontal + linesVertical;  // Add loiter points
    }
    
    return totalPoints;
}

int AreaPlanEditor::calculateFlightTime() const
{
    // Approximate flight time calculation
    const qreal averageSpeed = 5.0;  // meters per second
    const qreal turnTime = 5.0;      // seconds per turn
    
    // Calculate total distance
    qreal totalDistance = _areaWidth * qCeil(_areaHeight / _lineSpacing);  // Total survey distance
    
    // Calculate number of turns
    int numTurns = qCeil(_areaHeight / _lineSpacing);
    
    // Basic flight time = distance/speed + turns*turnTime
    int flightTime = qCeil(totalDistance / averageSpeed + numTurns * turnTime);
    
    // Add loiter time if enabled
    if (_loiterAfterRtl) {
        flightTime += calculateTotalWaypoints() * _loiterTime;
    }
    
    // Add RTL time if enabled
    if (_rtlAfterEveryWaypoint) {
        // Rough estimate: 2x the height for each RTL
        qreal rtlDistance = 2 * _missionAltitude * calculateTotalWaypoints();
        flightTime += qCeil(rtlDistance / averageSpeed);
    }
    
    return flightTime;
}

void AreaPlanEditor::calculateFormationPositions()
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    QList<Vehicle*> vehicles;
    for (int i = 0; i < vehicleModel->count(); i++) {
        vehicles.append(qobject_cast<Vehicle*>(vehicleModel->get(i)));
    }
    if (vehicles.isEmpty()) return;
    
    _formationOffsets.clear();
    
    // Initialize variables before switch statement
    const int totalVehicles = vehicles.size();
    const int gridSize = qCeil(qSqrt(totalVehicles));
    const qreal spacing = _formationSpacing;
    
    // Clear any existing offsets
    _formationOffsets.clear();
    
    switch (_currentFormation) {
        case FormationType::NoFormation:
            break;
        case FormationType::VFormation: {
            // V formation with leader at front
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    // Leader at front
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    // Followers in V shape
                    bool isLeft = (role % 2 == 1);
                    int position = (role + 1) / 2;
                    double angle = isLeft ? 30.0 : -30.0;  // 30-degree V shape
                    double distance = position * _formationSpacing;
                    
                    // Calculate offset using trigonometry
                    double dx = distance * qSin(qDegreesToRadians(angle));
                    double dy = -distance * qCos(qDegreesToRadians(angle));
                    
                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }
            
        case FormationType::LineFormation: {
            // Line formation with equal spacing
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    double offset = role * _formationSpacing;
                    _formationOffsets[vehicle->id()] = calculateOffsetCoordinate(QGeoCoordinate(), 0.0, offset);
                }
            }
            break;
        }
            
        case FormationType::CircleFormation: {
            // Circle formation with equal angular spacing
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    double angle = (360.0 * role) / totalVehicles;
                    double dx = _formationSpacing * qSin(qDegreesToRadians(angle));
                    double dy = _formationSpacing * qCos(qDegreesToRadians(angle));
                    
                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }
            
        case FormationType::GridFormation: {
            // Grid formation with equal spacing
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    int row = role / gridSize;
                    int col = role % gridSize;
                    double dx = col * _formationSpacing;
                    double dy = row * _formationSpacing;
                    
                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dy, 0.0);
                    offset = calculateOffsetCoordinate(offset, dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }
            
        default:
            break;
    }
    
    // Notify that formation positions have been updated
    emit formationPositionsChanged();
}
