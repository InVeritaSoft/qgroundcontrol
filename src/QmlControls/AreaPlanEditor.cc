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
#include <QDateTime>

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
    _planMasterController = controller;
    emit planMasterControllerChanged();
}

void AreaPlanEditor::setLandAtTargetReturn(bool enabled)
{
    if (_landAtTargetReturn == enabled) return;
    _landAtTargetReturn = enabled;
    emit landAtTargetReturnChanged();
}

void AreaPlanEditor::setDroneCount(int count)
{
    // Clamp to minimum of 1
    int clamped = count < 1 ? 1 : count;
    if (_droneCount == clamped) {
        return;
    }
    _droneCount = clamped;
    emit droneCountChanged();
}

void AreaPlanEditor::setAltitudeBandStart(qreal startMeters)
{
    // Clamp to >= 0
    qreal clamped = startMeters < 0.0 ? 0.0 : startMeters;
    if (qFuzzyCompare(_altitudeBandStart, clamped)) {
        return;
    }
    _altitudeBandStart = clamped;
    emit altitudeBandStartChanged();
}

void AreaPlanEditor::setAltitudeBandStep(qreal stepMeters)
{
    // Require > 0, otherwise reset to default positive step
    qreal value = stepMeters > 0.0 ? stepMeters : _defaultAltitudeBandStep;
    if (qFuzzyCompare(_altitudeBandStep, value)) {
        return;
    }
    _altitudeBandStep = value;
    emit altitudeBandStepChanged();
}

void AreaPlanEditor::setTimeOffsetPerDrone(qreal seconds)
{
    // Clamp to \u003e= 0
    qreal clamped = seconds < 0.0 ? 0.0 : seconds;
    if (qFuzzyCompare(_timeOffsetPerDrone, clamped)) {
        return;
    }
    _timeOffsetPerDrone = clamped;
    emit timeOffsetPerDroneChanged();
}

void AreaPlanEditor::setPerTargetSeparationS(qreal seconds)
{
    qreal clamped = seconds < 0.0 ? 0.0 : seconds;
    if (qFuzzyCompare(_perTargetSeparationS, clamped)) return;
    _perTargetSeparationS = clamped;
    emit perTargetSeparationSChanged();
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

void AreaPlanEditor::setTargetHoldTimeS(qreal seconds)
{
    qreal clamped = seconds < 0 ? 0 : seconds;
    if (qFuzzyCompare(_targetHoldTimeS, clamped)) return;
    _targetHoldTimeS = clamped;
    emit targetHoldTimeSChanged();
}

void AreaPlanEditor::setHomeTurnaroundWaitS(qreal seconds)
{
    qreal clamped = seconds < 0 ? 0 : seconds;
    if (qFuzzyCompare(_homeTurnaroundWaitS, clamped)) return;
    _homeTurnaroundWaitS = clamped;
    emit homeTurnaroundWaitSChanged();
}

void AreaPlanEditor::setPayloadReleaseEnabled(bool enabled)
{
    if (_payloadReleaseEnabled == enabled) return;
    _payloadReleaseEnabled = enabled;
    emit payloadReleaseEnabledChanged();
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

    auto clamp = [](qreal v, qreal lo, qreal hi){ return v < lo ? lo : (v > hi ? hi : v); };

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

            // Clamp to bounds in local space (safety against FP drift)
            x = clamp(x, -halfW, halfW);
            y = clamp(y, -halfH, halfH);

            // Apply rotation around center
            const QPointF r = rotateXY(x, y);

// Convert to geo coordinate
            QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
            // Set altitude as mission altitude for downstream consumers/tests
            wp.setAltitude(_missionAltitude);
            waypoints.append(QVariant::fromValue(wp));
        }
    }

    return waypoints;
}

QList<QVariant> AreaPlanEditor::computePartitionStripes() const
{
    QList<QVariant> stripes;
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0) {
        return stripes;
    }
    // Number of stripes equals lineCount along height before rotation
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const double cx = 0.0;
    const double cy = 0.0;
    // Build local stripes, then map endpoints to geo using areaCenter and rotation already applied in helper
    const auto lines = AreaPlan::splitIntoStripes(cx, cy,
                                                  static_cast<double>(_areaWidth),
                                                  static_cast<double>(_areaHeight),
                                                  lineCount,
                                                  /*alongShortAxis=*/true,
                                                  static_cast<double>(_areaRotation));
    // Convert to geo coordinates: treat local meters (x east, y north) relative to areaCenter
    auto toGeo = [&](double x, double y) {
        // translate by dy north, then dx east
        QGeoCoordinate tmp = calculateOffsetCoordinate(_areaCenter, std::abs(y), y >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, std::abs(x), x >= 0 ? 90.0 : 270.0);
    };
    for (const auto& ln : lines) {
        QVariantMap m;
        const QGeoCoordinate a = toGeo(ln.a.x, ln.a.y);
        const QGeoCoordinate b = toGeo(ln.b.x, ln.b.y);
        m["a"] = QVariant::fromValue(a);
        m["b"] = QVariant::fromValue(b);
        stripes.append(m);
    }
    return stripes;
}

QList<QVariant> AreaPlanEditor::computeRoundRobinAssignments() const
{
    QList<QVariant> assignments;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        QVariantMap m;
        QVariantList idx;
        for (int i : rr[static_cast<size_t>(d)]) idx.append(i);
        m["droneIndex"] = d;
        m["lineIndices"] = idx;
        assignments.append(m);
    }
    return assignments;
}

QList<QVariant> AreaPlanEditor::computeDroneAssignments() const
{
    QList<QVariant> assignments;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        QVariantMap m;
        QVariantList idx;
        for (int i : rr[static_cast<size_t>(d)]) idx.append(i);
        m["droneIndex"] = d;
        m["altitudeOffsetM"] = _altitudeBandStart + d * _altitudeBandStep;
        m["timeOffsetS"] = d * _timeOffsetPerDrone;
        m["lineIndices"] = idx;
        assignments.append(m);
    }
    return assignments;
}

QMap<QString, QVariant> AreaPlanEditor::computePerDroneCounts() const
{
    QMap<QString, QVariant> counts;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    QVariantList per;
    int sum = 0;
    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        const int c = static_cast<int>(rr[static_cast<size_t>(d)].size());
        QVariantMap m; m["droneIndex"] = d; m["lineCount"] = c; per.append(m); sum += c;
    }
    counts["perDrone"] = per;
    counts["totalLines"] = sum;
    counts["expectedTotalWaypoints"] = sum * _numPoints;
    return counts;
}

QList<QVariant> AreaPlanEditor::computePerDroneWaypointPreview() const
{
    QList<QVariant> preview;
    // Guard
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) {
        return preview;
    }
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);

    // Precompute rotated coordinates for each line index and point index similar to generateWaypoints
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    auto rotateXY = [&](qreal x, qreal y) { return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); };
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };

    auto lineYAt = [&](int li) {
        if (lineCount == 1) return 0.0;
        return -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
    };
    auto pointXAt = [&](int pi) {
        if (_numPoints == 1) return 0.0;
        return -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
    };

    for (int d = 0; d < static_cast<int>(rr.size()); ++d) {
        const auto& indices = rr[static_cast<size_t>(d)];
        QVariantMap group;
        group["droneIndex"] = d;
        group["altitudeOffsetM"] = _altitudeBandStart + d * _altitudeBandStep;
        group["timeOffsetS"] = d * _timeOffsetPerDrone;
        QVariantList wps;
        for (int li : indices) {
            const qreal y = (lineCount == 1) ? 0.0 : qBound(-halfH, lineYAt(li), halfH);
            for (int pi = 0; pi < _numPoints; ++pi) {
                const qreal x = (_numPoints == 1) ? 0.0 : qBound(-halfW, pointXAt(pi), halfW);
                const QPointF r = rotateXY(x, y);
                QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
                // Altitude per-drone band offset
                wp.setAltitude(_missionAltitude + (_altitudeBandStart + d * _altitudeBandStep));
                wps.append(QVariant::fromValue(wp));
            }
        }
        group["waypoints"] = wps;
        preview.append(group);
    }
    return preview;
}

QList<QVariant> AreaPlanEditor::generatePerDroneWaypoints(int droneIndex) const
{
    QList<QVariant> waypoints;
    if (droneIndex < 0 || droneIndex >= _droneCount) return waypoints;
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) return waypoints;

    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);

    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    auto rotateXY = [&](qreal x, qreal y) { return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); };
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };
    auto lineYAt = [&](int li) {
        if (lineCount == 1) return 0.0;
        return -halfH + (static_cast<qreal>(li) * (_areaHeight / (lineCount - 1)));
    };
    auto pointXAt = [&](int pi) {
        if (_numPoints == 1) return 0.0;
        return -halfW + (static_cast<qreal>(pi) * (_areaWidth / (_numPoints - 1)));
    };

    const double altOffset = _altitudeBandStart + droneIndex * _altitudeBandStep;

    const auto& indices = rr[static_cast<size_t>(droneIndex)];
    for (int li : indices) {
        const qreal y = (lineCount == 1) ? 0.0 : qBound(-halfH, lineYAt(li), halfH);
        for (int pi = 0; pi < _numPoints; ++pi) {
            const qreal x = (_numPoints == 1) ? 0.0 : qBound(-halfW, pointXAt(pi), halfW);
            const QPointF r = rotateXY(x, y);
            QGeoCoordinate wp = offsetByXY(_areaCenter, r.x(), r.y());
            wp.setAltitude(_missionAltitude + altOffset);
            waypoints.append(QVariant::fromValue(wp));
        }
    }

    return waypoints;
}

void AreaPlanEditor::insertGripperRelease(MissionController* mission, const QGeoCoordinate& atCoord)
{
    if (!mission || !_payloadReleaseEnabled) return;
    VisualMissionItem* doItem = mission->insertSimpleMissionItem(atCoord, -1, false);
    if (SimpleMissionItem* sm = qobject_cast<SimpleMissionItem*>(doItem)) {
        sm->setCommand(MAV_CMD_DO_GRIPPER);
        // Param1=gripper instance (0), Param2=action (0=release)
        sm->missionItem().setParam1(0);
        sm->missionItem().setParam2(0);
    }
}

void AreaPlanEditor::addPerDroneToMission(int droneIndex)
{
    MissionController* mission = getMissionController();
    if (!mission) {
        qWarning() << "AreaPlanEditor::addPerDroneToMission: MissionController not set";
        return;
    }
    // Ensure mission has MissionSettings
    if (mission->visualItems()->count() == 0) {
        // Trigger init by inserting and removing a dummy to create settings if needed
        mission->insertSimpleMissionItem(_areaCenter, -1, false);
        // Remove the inserted item, keep settings
        if (mission->visualItems()->count() > 1) {
            mission->removeVisualItem(1);
        }
    }

    const QVariantList wps = generatePerDroneWaypoints(droneIndex);
    for (int idx = 0; idx < wps.size(); ++idx) {
        QGeoCoordinate c = wps[idx].value<QGeoCoordinate>();
        // Slotting to avoid conflicts at start and between cycles
        qreal startDelay = (idx == 0) ? (droneIndex * _timeOffsetPerDrone) : _perTargetSeparationS;
        if (startDelay > 0.0) {
            VisualMissionItem* hold = mission->insertSimpleMissionItem(_homeLocation.isValid() ? _homeLocation : c, -1, false);
            if (SimpleMissionItem* h = qobject_cast<SimpleMissionItem*>(hold)) {
                h->setCommand(MAV_CMD_NAV_LOITER_TIME);
                h->missionItem().setParam1(startDelay);
                if (h->specifiesAltitude()) {
                    h->altitude()->setRawValue(_missionAltitude);
                }
            }
        }
        if (_landAtTargetReturn) {
            // Land at target
            mission->insertLandItem(c, -1, false);
            // Optional: Payload release command
            insertGripperRelease(mission, c);
            // Hold on target for configured time
            if (_targetHoldTimeS > 0) {
                VisualMissionItem* hold = mission->insertSimpleMissionItem(c, -1, false);
                if (SimpleMissionItem* h = qobject_cast<SimpleMissionItem*>(hold)) {
                    h->setCommand(MAV_CMD_NAV_LOITER_TIME);
                    h->missionItem().setParam1(_targetHoldTimeS);
                    if (h->specifiesAltitude()) {
                        h->altitude()->setRawValue(_missionAltitude);
                    }
                }
            }
            // Takeoff from target back to altitude
            VisualMissionItem* tkItem = mission->insertSimpleMissionItem(c, -1, false);
            if (SimpleMissionItem* tk = qobject_cast<SimpleMissionItem*>(tkItem)) {
                tk->setCommand(MAV_CMD_NAV_TAKEOFF);
                if (tk->specifiesAltitude()) {
                    tk->altitude()->setRawValue(_missionAltitude);
                }
            }
            // Return and land at home
            mission->insertLandItem(QGeoCoordinate(), -1, false);
            // Loiter at home for turnaround (use configured wait)
            VisualMissionItem* loiterItem = mission->insertSimpleMissionItem(_homeLocation.isValid() ? _homeLocation : c, -1, false);
            if (SimpleMissionItem* loiter = qobject_cast<SimpleMissionItem*>(loiterItem)) {
                loiter->setCommand(MAV_CMD_NAV_LOITER_TIME);
                loiter->missionItem().setParam1(_homeTurnaroundWaitS > 0 ? _homeTurnaroundWaitS : _loiterTime);
                if (loiter->specifiesAltitude()) {
                    loiter->altitude()->setRawValue(_missionAltitude);
                }
            }
        } else {
            // Insert waypoint transit only
            VisualMissionItem* vmi = mission->insertSimpleMissionItem(c, -1, false);
            if (SimpleMissionItem* smi = qobject_cast<SimpleMissionItem*>(vmi)) {
                if (smi->specifiesAltitude()) {
                    smi->altitude()->setRawValue(c.altitude());
                }
            }
            // Policy: RTL after every waypoint
            if (_rtlAfterEveryWaypoint) {
                mission->insertLandItem(QGeoCoordinate(), -1, false);
                if (_loiterAfterRtl) {
                    VisualMissionItem* lItem = mission->insertSimpleMissionItem(c, -1, false);
                    if (SimpleMissionItem* loiter = qobject_cast<SimpleMissionItem*>(lItem)) {
                        loiter->setCommand(MAV_CMD_NAV_LOITER_TIME);
                        loiter->missionItem().setParam1(_loiterTime);
                        if (loiter->specifiesAltitude()) {
                            loiter->altitude()->setRawValue(_missionAltitude);
                        }
                    }
                }
            }
        }
    }
}

void AreaPlanEditor::addAllDronesToMission()
{
    for (int d = 0; d < _droneCount; ++d) {
        addPerDroneToMission(d);
    }
}

void AreaPlanEditor::addWaypointsToMission()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        qWarning() << "AreaPlanEditor::addWaypointsToMission: MissionController not set";
        return;
    }
    // Ensure mission has MissionSettings initialized
    if (mission->visualItems()->count() == 0) {
        VisualMissionItem* tmp = mission->insertSimpleMissionItem(_areaCenter, -1, false);
        if (mission->visualItems()->count() > 1) {
            mission->removeVisualItem(1);
        }
        Q_UNUSED(tmp);
    }

    // If advanced business-flow is enabled or multi-drone planning is in use,
    // delegate to per-drone insertion which handles LAND/LOITER/TAKEOFF/returns.
    const bool advanced = (_droneCount > 1) || _landAtTargetReturn || _payloadReleaseEnabled ||
                          _rtlAfterEveryWaypoint || _loiterAfterRtl ||
                          (_timeOffsetPerDrone > 0) || (_perTargetSeparationS > 0);
    if (advanced) {
        for (int d = 0; d < _droneCount; ++d) {
            addPerDroneToMission(d);
        }
        return;
    }

    // Simple single-drone insertion: straight waypoints only
    const QVariantList wps = generateWaypoints();
    for (const QVariant& v : wps) {
        QGeoCoordinate c = v.value<QGeoCoordinate>();
        VisualMissionItem* vmi = mission->insertSimpleMissionItem(c, -1, false);
        if (SimpleMissionItem* smi = qobject_cast<SimpleMissionItem*>(vmi)) {
            if (smi->specifiesAltitude()) {
                smi->altitude()->setRawValue(c.altitude());
            }
        }
    }
}

void AreaPlanEditor::saveMissionFile()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    // Compose a simple CSV filename in working directory with timestamp
    const QString filename = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss'_mission.csv'");
    saveMissionToFile(mission, filename);
    updateStatus(QString("Mission saved: %1").arg(filename));
}

void AreaPlanEditor::clearMission()
{
    MissionController* mission = getMissionController();
    if (!mission) { handleError("MissionController not set", QString()); return; }
    // Try direct call if API exists
    bool invoked = QMetaObject::invokeMethod(mission, "removeAll", Qt::DirectConnection);
    if (!invoked) {
        // Fallback: remove everything except MissionSettings (index 0)
        if (mission->visualItems()) {
            // remove from end to start to maintain indices
            for (int i = mission->visualItems()->count() - 1; i >= 1; --i) {
                mission->removeVisualItem(i);
            }
        }
    }
    updateStatus("Mission items cleared");
}

void AreaPlanEditor::savePerDroneMissionFiles()
{
    if (_droneCount < 1) return;
    // Save a CSV per drone using generated waypoints
    const QString ts = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss");
    for (int d = 0; d < _droneCount; ++d) {
        const QVariantList wps = generatePerDroneWaypoints(d);
        const QString filename = QString("%1_drone%2.csv").arg(ts).arg(d);
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            handleError(QString("Could not open %1 for writing").arg(filename), QString());
            continue;
        }
        QTextStream ts(&f);
        ts << "lat,lon,alt\n";
        for (const QVariant& v : wps) {
            const QGeoCoordinate c = v.value<QGeoCoordinate>();
            ts << QString::number(c.latitude(), 'f', 7) << ","
               << QString::number(c.longitude(), 'f', 7) << ","
               << QString::number(c.altitude(), 'f', 2) << "\n";
        }
        f.close();
    }
    updateStatus("Per-drone mission CSV files saved");
}

void AreaPlanEditor::uploadToVehicle()
{
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    // Direct C++ call: MissionController::sendToVehicle is not a QML-invokable method
    mission->sendToVehicle();
    updateStatus("Upload to active vehicle initiated");
}

void AreaPlanEditor::uploadPerDroneMissionToVehicle(int droneIndex, QObject* vehicleObject)
{
    // Insert per-drone waypoints to the mission and request upload
    addPerDroneToMission(droneIndex);
    uploadToVehicle();
    emit missionUploaded(droneIndex, vehicleObject);
}

void AreaPlanEditor::uploadToAllDrones()
{
    // Add waypoints for all drones to the single mission sequence, then upload
    addAllDronesToMission();
    uploadToVehicle();
}

void AreaPlanEditor::armVehicle(QObject* vehicleObject, bool arm)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    bool ok = QMetaObject::invokeMethod(v, "setArmed", Qt::DirectConnection, Q_ARG(bool, arm));
    if (!ok) {
        ok = QMetaObject::invokeMethod(v, "armDisarm", Qt::DirectConnection, Q_ARG(bool, arm));
    }
    updateStatus(ok ? QString("Vehicle %1 %2").arg(v->id()).arg(arm?"armed":"disarmed")
                    : QString("Vehicle %1 arm/disarm command failed").arg(v->id()));
}

void AreaPlanEditor::takeoffVehicle(QObject* vehicleObject, qreal altitude)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    // Try common guided takeoff method names
    bool ok = QMetaObject::invokeMethod(v, "guidedTakeoff", Qt::DirectConnection, Q_ARG(double, static_cast<double>(altitude)));
    if (!ok) {
        ok = QMetaObject::invokeMethod(v, "guidedModeTakeoff", Qt::DirectConnection, Q_ARG(double, static_cast<double>(altitude)));
    }
    if (!ok) {
        // Fallback: set flight mode to Takeoff if available
        ok = QMetaObject::invokeMethod(v, "setFlightMode", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("Takeoff")));
    }
    updateStatus(ok ? QString("Vehicle %1 takeoff requested").arg(v->id())
                    : QString("Vehicle %1 takeoff command failed").arg(v->id()));
}

void AreaPlanEditor::landVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    // Try direct land command first if exposed
    bool ok = QMetaObject::invokeMethod(v, "land", Qt::DirectConnection);
    if (!ok) {
        // Try setting flight mode to Land
        ok = QMetaObject::invokeMethod(v, "setFlightMode", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("Land")));
    }
    if (!ok) {
        // Some stacks may use "RTL" or command variants; as last resort, try returnToLaunch
        ok = QMetaObject::invokeMethod(v, "returnToLaunch", Qt::DirectConnection);
    }
    updateStatus(ok ? QString("Vehicle %1 land requested").arg(v->id())
                    : QString("Vehicle %1 land command failed").arg(v->id()));
}

void AreaPlanEditor::startMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    bool ok = QMetaObject::invokeMethod(v, "startMission", Qt::DirectConnection);
    if (!ok) {
        ok = QMetaObject::invokeMethod(v, "setFlightMode", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("Mission")));
    }
    updateStatus(ok ? QString("Vehicle %1 mission start requested").arg(v->id())
                    : QString("Vehicle %1 mission start failed").arg(v->id()));
}

void AreaPlanEditor::pauseMissionOnVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    bool ok = QMetaObject::invokeMethod(v, "pauseMission", Qt::DirectConnection);
    if (!ok) {
        ok = QMetaObject::invokeMethod(v, "setFlightMode", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("Hold")));
    }
    updateStatus(ok ? QString("Vehicle %1 mission pause requested").arg(v->id())
                    : QString("Vehicle %1 mission pause failed").arg(v->id()));
}

void AreaPlanEditor::rtlVehicle(QObject* vehicleObject)
{
    Vehicle* v = qobject_cast<Vehicle*>(vehicleObject);
    if (!v) { handleError("Invalid vehicle object", QString()); return; }
    bool ok = QMetaObject::invokeMethod(v, "setFlightMode", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("Return")));
    if (!ok) {
        ok = QMetaObject::invokeMethod(v, "returnToLaunch", Qt::DirectConnection);
    }
    updateStatus(ok ? QString("Vehicle %1 RTL requested").arg(v->id())
                    : QString("Vehicle %1 RTL command failed").arg(v->id()));
}

QVariantMap AreaPlanEditor::getVehicleStatus(QObject* vehicleObject) const
{
    QVariantMap m;
    const Vehicle* v = qobject_cast<const Vehicle*>(vehicleObject);
    if (!v) return m;
    m["id"] = v->id();
    m["armed"] = v->property("armed");
    m["flightMode"] = v->property("flightMode");
    m["connectionLost"] = v->property("connectionLost");
    m["linkName"] = v->property("activeLinkName");
    m["altitudeRelative"] = v->property("altitudeRelative");
    // Battery group (if available)
    m["batteryPercent"] = v->property("batteryPercent");
    return m;
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
    MissionController* mission = getMissionController();
    if (!mission) {
        handleError("MissionController not set", "Open Plan view to initialize controller");
        return;
    }
    bool invoked = QMetaObject::invokeMethod(mission, "startMission", Qt::DirectConnection);
    if (!invoked) {
        invoked = QMetaObject::invokeMethod(mission, "startMission", Qt::QueuedConnection);
    }
    updateStatus(invoked ? "Mission start requested" : "Start method not available on MissionController");
}

void AreaPlanEditor::updateStatus(const QString& message)
{
    emit statusChanged(message);
}

void AreaPlanEditor::testCompleteWorkflow()
{
    // Simple end-to-end: validate, generate, insert, save, upload
    if (!validateAreaParameters()) {
        handleError("Invalid area parameters", _validationError);
        return;
    }
    if (!validateWaypointGeneration()) {
        handleError("Waypoint generation failed", QString());
        return;
    }
    addWaypointsToMission();
    saveMissionFile();
    uploadToVehicle();
}

bool AreaPlanEditor::validateAreaParameters()
{
    auto setError = [this](const QString& err){ _validationError = err; emit validationErrorChanged(); };
    if (!_areaCenter.isValid()) { setError("Area center is invalid"); return false; }
    if (!qIsFinite(_areaWidth) || _areaWidth <= 0) { setError("Area width must be > 0"); return false; }
    if (!qIsFinite(_areaHeight) || _areaHeight <= 0) { setError("Area height must be > 0"); return false; }
    if (!qIsFinite(_lineSpacing) || _lineSpacing <= 0) { setError("Line spacing must be > 0"); return false; }
    if (_numPoints <= 0) { setError("Points per line must be > 0"); return false; }
    if (!qIsFinite(_missionAltitude)) { setError("Mission altitude invalid"); return false; }
    if (_droneCount < 1) { setError("Drone count must be >= 1"); return false; }
    if (_altitudeBandStep <= 0) { setError("Altitude band step must be > 0"); return false; }
    // Clear previous error
    if (!_validationError.isEmpty()) { _validationError.clear(); emit validationErrorChanged(); }
    return true;
}

bool AreaPlanEditor::validateWaypointGeneration()
{
    // Validate that generated waypoints match expected counts and lie within the rotated rectangle bounds.
    if (_areaCenter.isValid() == false || _areaWidth <= 0 || _areaHeight <= 0 || _numPoints <= 0 || _lineSpacing <= 0) {
        return false;
    }

    const QList<QVariant> wps = generateWaypoints();
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const int expectedCount = lineCount * _numPoints;
    if (wps.size() != expectedCount) {
        qWarning() << "Waypoint count mismatch" << wps.size() << "!=" << expectedCount;
        return false;
    }

    // Geometry check: approximate dx,dy in meters from center, unrotate, and assert within half width/height
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    const qreal theta = qDegreesToRadians(_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);

    auto toMeters = [&](const QGeoCoordinate& c0, const QGeoCoordinate& c1) {
        const qreal metersPerDegreeLat = 111319.9;
        const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(c0.latitude()));
        const qreal dy = (c1.latitude() - c0.latitude()) * metersPerDegreeLat; // north positive
        const qreal dx = (c1.longitude() - c0.longitude()) * metersPerDegreeLon; // east positive
        return QPointF(dx, dy);
    };

    auto unrotate = [&](const QPointF& p) {
        // Apply inverse rotation by -theta to map back to local rectangle axes
        return QPointF(p.x() *  cosT + p.y() * sinT,
                       -p.x() * sinT + p.y() * cosT);
    };

    const qreal eps = 0.25; // meters tolerance
    for (const QVariant& v : wps) {
        const QGeoCoordinate wp = v.value<QGeoCoordinate>();
        if (!wp.isValid()) {
            qWarning() << "Invalid waypoint coordinate";
            return false;
        }
        const QPointF dxy = toMeters(_areaCenter, wp);
        const QPointF local = unrotate(dxy);
        if (qAbs(local.x()) > halfW + eps || qAbs(local.y()) > halfH + eps) {
            qWarning() << "Waypoint out of bounds" << local << "halfW/H" << halfW << halfH;
            return false;
        }
    }

    return true;
}

bool AreaPlanEditor::validateMissionUpload()
{
    Vehicle* v = getCurrentVehicle();
    MissionController* mission = getMissionController();
    if (!v) { _validationError = "No active vehicle"; emit validationErrorChanged(); return false; }
    if (!mission) { _validationError = "MissionController not set"; emit validationErrorChanged(); return false; }
    if (mission->visualItems()->count() <= 1) { // only MissionSettings
        _validationError = "No mission items to upload"; emit validationErrorChanged(); return false; }
    _validationError.clear(); emit validationErrorChanged();
    return true;
}

bool AreaPlanEditor::validateMissionFileSaving()
{
    // Basic check: controller exists and has items
    MissionController* mission = getMissionController();
    if (!mission) return false;
    return mission->visualItems()->count() > 0;
}

QMap<QString, QVariant> AreaPlanEditor::getDroneAllocationStats(int droneIndex) const
{
    QMap<QString, QVariant> stats;
    if (droneIndex < 0 || droneIndex >= _droneCount) return stats;
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    const auto rr = AreaPlan::assignStripesRoundRobin(_droneCount, lineCount);
    const auto& assigned = rr[static_cast<size_t>(droneIndex)];
    const int lines = static_cast<int>(assigned.size());
    const int wpCount = lines * _numPoints;
    stats["droneIndex"] = droneIndex;
    stats["lineCount"] = lines;
    stats["waypointCount"] = wpCount;
    stats["altitudeOffsetM"] = _altitudeBandStart + droneIndex * _altitudeBandStep;
    stats["timeOffsetS"] = droneIndex * _timeOffsetPerDrone;
    QVariantList li; for (int i : assigned) li.append(i); stats["lineIndices"] = li;
    return stats;
}

bool AreaPlanEditor::validateSwarmConfiguration() const
{
    QmlObjectListModel* vehicleModel = MultiVehicleManager::instance()->vehicles();
    if (!vehicleModel || vehicleModel->count() < 2) return false;
    // At least leader + one follower and roles assigned
    for (int i = 0; i < vehicleModel->count(); ++i) {
        Vehicle* v = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (!v) return false;
        if (!_formationRoles.contains(v->id())) return false;
    }
    return true;
}

QString AreaPlanEditor::validateInput(const QString& fieldName, const QVariant& value) const
{
    auto bad = [&](const QString& m){ return m; };
    if (fieldName == "areaWidth" || fieldName == "areaHeight" || fieldName == "lineSpacing") {
        bool ok; const qreal v = value.toDouble(&ok); if (!ok || v <= 0) return bad(fieldName + " must be > 0");
    } else if (fieldName == "numPoints") {
        bool ok; int v = value.toInt(&ok); if (!ok || v <= 0) return bad("numPoints must be > 0");
    } else if (fieldName == "missionAltitude") {
        bool ok; value.toDouble(&ok); if (!ok) return bad("missionAltitude invalid");
    } else if (fieldName == "droneCount") {
        bool ok; int v = value.toInt(&ok); if (!ok || v < 1) return bad("droneCount must be >= 1");
    } else if (fieldName == "altitudeBandStep") {
        bool ok; const qreal v = value.toDouble(&ok); if (!ok || v <= 0) return bad("altitudeBandStep must be > 0");
    }
    return QString();
}

bool AreaPlanEditor::isInputValid(const QString& fieldName, const QVariant& value) const
{
    return validateInput(fieldName, value).isEmpty();
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
    // Enable a simple cache based on key of parameters
    enableOptimizations();
    // Precompute and cache current configuration
    const QString key = QString("w=%1|h=%2|s=%3|n=%4|r=%5|clat=%6|clon=%7")
                            .arg(_areaWidth).arg(_areaHeight).arg(_lineSpacing).arg(_numPoints)
                            .arg(_areaRotation)
                            .arg(_areaCenter.latitude(), 0, 'f', 7)
                            .arg(_areaCenter.longitude(), 0, 'f', 7);
    if (_waypointCache.contains(key)) {
        _cacheHits++;
        return;
    }
    _cacheMisses++;
    QVariantList wps = const_cast<AreaPlanEditor*>(this)->generateWaypoints();
    _waypointCache.insert(key, wps);
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
    _performanceTimer.restart();
    generateWaypoints();
    qint64 elapsedMs = _performanceTimer.elapsed();
    _performanceMetrics["generateWaypoints_ms"] = elapsedMs;
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

    // Clear any existing offsets
    _formationOffsets.clear();

    const int totalVehicles = vehicles.size();
    const int gridSize = qCeil(qSqrt(totalVehicles));

    switch (_currentFormation) {
        case FormationType::NoFormation:
            // No offsets from leader
            for (Vehicle* vehicle : vehicles) {
                _formationOffsets[vehicle->id()] = QGeoCoordinate();
            }
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

                    // Calculate offset using trigonometry (x east, y north)
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
            // Line formation with equal spacing to the east (x axis)
            for (Vehicle* vehicle : vehicles) {
                int role = _formationRoles[vehicle->id()];
                if (role == 0) {
                    _formationOffsets[vehicle->id()] = QGeoCoordinate();
                } else {
                    double dx = role * _formationSpacing;
                    QGeoCoordinate offset = calculateOffsetCoordinate(QGeoCoordinate(), dx, 90.0);
                    _formationOffsets[vehicle->id()] = offset;
                }
            }
            break;
        }

        case FormationType::CircleFormation: {
            // Circle formation around leader
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
            // Grid formation filled row-major
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

    emit formationPositionsChanged();
}

// --- File save helpers -------------------------------------------------
void AreaPlanEditor::saveMissionToFile(MissionController* missionController, const QString& filename)
{
    if (!missionController) return;
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    if (missionController->visualItems()) {
        for (int i = 0; i < missionController->visualItems()->count(); ++i) {
            auto* vmi = qobject_cast<VisualMissionItem*>(missionController->visualItems()->get(i));
            if (!vmi) continue;
            const QGeoCoordinate c = vmi->coordinate();
            ts << QString::number(c.latitude(), 'f', 7) << ","
               << QString::number(c.longitude(), 'f', 7) << ","
               << QString::number(c.altitude(), 'f', 2) << "\n";
        }
    }
    f.close();
}

void AreaPlanEditor::saveMissionToFile(const QList<MissionItem*>& missionItems, const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    for (const MissionItem* mi : missionItems) {
        if (!mi) continue;
        const QGeoCoordinate c = mi->coordinate();
        ts << QString::number(c.latitude(), 'f', 7) << ","
           << QString::number(c.longitude(), 'f', 7) << ","
           << QString::number(c.altitude(), 'f', 2) << "\n";
    }
    f.close();
}
