#include "AreaPlanEditor.h"
#include "PlanMasterController.h"
#include "MissionController.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"

#include <QtMath>
#include <QDebug>
#include <QLoggingCategory>
#include <QDateTime>

Q_LOGGING_CATEGORY(AreaPlanEditorLog, "AreaPlanEditorLog")

AreaPlanEditor::AreaPlanEditor(QObject* parent)
    : QObject(parent)
{
    // Initialize with default values
    _areaWidth = 100.0;
    _areaHeight = 100.0;
    _lineSpacing = 10.0;
    _numPoints = 5;
    _missionAltitude = 50.0;
    _droneCount = 1;
    _altitudeBandStart = 0.0;
    _altitudeBandStep = 10.0;
}

void AreaPlanEditor::setIsDrawingMode(bool drawingMode)
{
    if (_isDrawingMode == drawingMode) {
        return;
    }
    _isDrawingMode = drawingMode;
    emit isDrawingModeChanged();
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

void AreaPlanEditor::setAreaRotation(qreal rotation)
{
    if (qFuzzyCompare(_areaRotation, rotation)) {
        return;
    }
    _areaRotation = rotation;
    emit areaRotationChanged();
}

void AreaPlanEditor::setDroneCount(int count)
{
    if (_droneCount == count) {
        return;
    }
    _droneCount = count;
    emit droneCountChanged();
}

void AreaPlanEditor::setAltitudeBandStart(qreal start)
{
    if (qFuzzyCompare(_altitudeBandStart, start)) {
        return;
    }
    _altitudeBandStart = start;
    emit altitudeBandStartChanged();
}

void AreaPlanEditor::setAltitudeBandStep(qreal step)
{
    if (qFuzzyCompare(_altitudeBandStep, step)) {
        return;
    }
    _altitudeBandStep = step;
    emit altitudeBandStepChanged();
}

void AreaPlanEditor::setPlanMasterController(PlanMasterController* controller)
{
    if (_planMasterController == controller) {
        return;
    }
    _planMasterController = controller;
    emit planMasterControllerChanged();
}

void AreaPlanEditor::moveAreaNorth()
{
    if (!_areaCenter.isValid()) {
        return;
    }
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _moveStepMeters, 0.0);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaSouth()
{
    if (!_areaCenter.isValid()) {
        return;
    }
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _moveStepMeters, 180.0);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaEast()
{
    if (!_areaCenter.isValid()) {
        return;
    }
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _moveStepMeters, 90.0);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::moveAreaWest()
{
    if (!_areaCenter.isValid()) {
        return;
    }
    QGeoCoordinate newCenter = calculateOffsetCoordinate(_areaCenter, _moveStepMeters, 270.0);
    setAreaCenter(newCenter);
}

void AreaPlanEditor::rotateAreaClockwise()
{
    setAreaRotation(_areaRotation + _rotationStepDegrees);
}

void AreaPlanEditor::rotateAreaCounterClockwise()
{
    setAreaRotation(_areaRotation - _rotationStepDegrees);
}

void AreaPlanEditor::centerArea()
{
    if (!_planMasterController || !_planMasterController->controllerVehicle()) {
        return;
    }
    
    QGeoCoordinate vehiclePosition = _planMasterController->controllerVehicle()->coordinate();
    if (vehiclePosition.isValid()) {
        setAreaCenter(vehiclePosition);
    }
}

QVariantList AreaPlanEditor::generateWaypoints()
{
    QVariantList waypoints;
    
    // Basic validation
    if (!_areaCenter.isValid() || _areaWidth <= 0 || _areaHeight <= 0 || 
        _numPoints <= 0 || _lineSpacing <= 0) {
        return waypoints;
    }
    
    // Compute number of grid lines along height (north-south axis before rotation)
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    
    // Calculate half dimensions for centering
    const qreal halfWidth = _areaWidth * 0.5;
    const qreal halfHeight = _areaHeight * 0.5;
    
    // Rotation transformation - CRITICAL: Rotation is applied here
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosTheta = qCos(theta);
    const qreal sinTheta = qSin(theta);
    
    // Generate waypoints for each line
    for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
        // Calculate line position (north-south offset from center)
        const qreal lineOffset = (lineIndex * _lineSpacing) - halfHeight;
        
        // Generate points along this line
        for (int pointIndex = 0; pointIndex < _numPoints; ++pointIndex) {
            // Calculate point position along line (east-west offset from center)
            const qreal pointOffset = (pointIndex * (_areaWidth / qMax(1, _numPoints - 1))) - halfWidth;
            
            // Apply rotation transformation - THIS IS WHERE ROTATION HAPPENS
            const qreal rotatedX = pointOffset * cosTheta - lineOffset * sinTheta;
            const qreal rotatedY = pointOffset * sinTheta + lineOffset * cosTheta;
            
            // Convert to geographic coordinate
            QGeoCoordinate waypoint = calculateOffsetCoordinate(
                _areaCenter, 
                qAbs(rotatedY), 
                rotatedY >= 0 ? 0.0 : 180.0
            );
            waypoint = calculateOffsetCoordinate(
                waypoint, 
                qAbs(rotatedX), 
                rotatedX >= 0 ? 90.0 : 270.0
            );
            waypoint.setAltitude(_missionAltitude);
            
            waypoints.append(QVariant::fromValue(waypoint));
        }
    }
    
    return waypoints;
}

QVariantList AreaPlanEditor::computePerDroneWaypointPreview() const
{
    QVariantList preview;
    
    // Guard conditions
    if (_areaWidth <= 0 || _areaHeight <= 0 || _lineSpacing <= 0 || _numPoints <= 0) {
        return preview;
    }
    
    const int lineCount = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    
    // Simple round-robin assignment for now
    QList<QList<int>> roundRobin;
    for (int i = 0; i < _droneCount; ++i) {
        QList<int> droneLines;
        for (int lineIdx = i; lineIdx < lineCount; lineIdx += _droneCount) {
            droneLines.append(lineIdx);
        }
        roundRobin.append(droneLines);
    }
    
    // Precompute rotated coordinates for each line and point
    const qreal halfW = _areaWidth * 0.5;
    const qreal halfH = _areaHeight * 0.5;
    
    // Rotation transformation - CRITICAL: Same rotation logic as single-drone
    const qreal theta = qDegreesToRadians(-_areaRotation);
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);
    
    // Lambda function for rotation transformation
    auto rotateXY = [&](qreal x, qreal y) { 
        return QPointF(x * cosT - y * sinT, x * sinT + y * cosT); 
    };
    
    auto offsetByXY = [&](const QGeoCoordinate& c, qreal dx_m, qreal dy_m) {
        QGeoCoordinate tmp = calculateOffsetCoordinate(c, qAbs(dy_m), dy_m >= 0 ? 0.0 : 180.0);
        return calculateOffsetCoordinate(tmp, qAbs(dx_m), dx_m >= 0 ? 90.0 : 270.0);
    };
    
    // Generate preview for each drone
    for (int droneIndex = 0; droneIndex < _droneCount; ++droneIndex) {
        QVariantMap droneData;
        droneData["droneIndex"] = droneIndex;
        droneData["altitudeOffsetM"] = _altitudeBandStart + (droneIndex * _altitudeBandStep);
        droneData["timeOffsetS"] = droneIndex * _timeOffsetPerDrone;
        
        QVariantList waypoints;
        const auto& assignedLines = roundRobin[droneIndex];
        
        for (int lineIdx : assignedLines) {
            const qreal lineOffset = (lineIdx * _lineSpacing) - halfH;
            
            for (int pointIdx = 0; pointIdx < _numPoints; ++pointIdx) {
                const qreal pointOffset = (pointIdx * (_areaWidth / qMax(1, _numPoints - 1))) - halfW;
                
                // Apply rotation transformation - ROTATION APPLIED HERE TOO
                const auto rotated = rotateXY(pointOffset, lineOffset);
                
                QGeoCoordinate wp = offsetByXY(_areaCenter, rotated.x(), rotated.y());
                wp.setAltitude(_missionAltitude + droneData["altitudeOffsetM"].toReal());
                waypoints.append(QVariant::fromValue(wp));
            }
        }
        
        droneData["waypoints"] = waypoints;
        preview.append(droneData);
    }
    
    return preview;
}

void AreaPlanEditor::addWaypointsToMission()
{
    if (!_planMasterController || !_planMasterController->missionController()) {
        qWarning() << "AreaPlanEditor: No mission controller available";
        return;
    }
    
    QVariantList waypoints = generateWaypoints();
    if (waypoints.isEmpty()) {
        qWarning() << "AreaPlanEditor: No waypoints generated";
        return;
    }
    
    qCDebug(AreaPlanEditorLog) << "AreaPlanEditor: Adding" << waypoints.size() << "waypoints to mission";
    
    // Clear existing mission items first (optional - you can comment this out if you want to append)
    // _planMasterController->missionController()->removeAll();
    
    // Add waypoints to mission
    for (const QVariant& waypointVar : waypoints) {
        QGeoCoordinate waypoint = waypointVar.value<QGeoCoordinate>();
        if (waypoint.isValid()) {
            _planMasterController->missionController()->insertSimpleMissionItem(
                waypoint,
                _planMasterController->missionController()->visualItems()->count(),
                true
            );
        }
    }
    
    qCDebug(AreaPlanEditorLog) << "AreaPlanEditor: Successfully added waypoints to mission";
    emit waypointsAddedToMission(waypoints.size());
}

void AreaPlanEditor::saveMissionFile()
{
    if (!_planMasterController) {
        qWarning() << "AreaPlanEditor: No plan master controller available";
        return;
    }
    
    qCDebug(AreaPlanEditorLog) << "AreaPlanEditor: Saving mission file";
    
    // Generate a filename with timestamp
    QString filename = QString("area_mission_%1.plan").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    
    // Use the existing save functionality from plan master controller
    _planMasterController->saveToFile(filename);
    
    qCDebug(AreaPlanEditorLog) << "AreaPlanEditor: Mission file saved as" << filename;
    emit missionSaved();
}

QGeoCoordinate AreaPlanEditor::calculateOffsetCoordinate(const QGeoCoordinate& coordinate, qreal distanceMeters, qreal bearingDegrees) const
{
    if (!coordinate.isValid()) {
        return QGeoCoordinate();
    }
    
    // Convert bearing to radians
    qreal bearingRadians = qDegreesToRadians(bearingDegrees);
    
    // Earth's radius in meters
    const qreal earthRadius = 6371000.0;
    
    // Convert latitude and longitude to radians
    qreal latRadians = qDegreesToRadians(coordinate.latitude());
    qreal lonRadians = qDegreesToRadians(coordinate.longitude());
    
    // Calculate new latitude
    qreal newLatRadians = qAsin(qSin(latRadians) * qCos(distanceMeters / earthRadius) +
                               qCos(latRadians) * qSin(distanceMeters / earthRadius) * qCos(bearingRadians));
    
    // Calculate new longitude
    qreal newLonRadians = lonRadians + qAtan2(qSin(bearingRadians) * qSin(distanceMeters / earthRadius) * qCos(latRadians),
                                             qCos(distanceMeters / earthRadius) - qSin(latRadians) * qSin(newLatRadians));
    
    // Convert back to degrees
    qreal newLat = qRadiansToDegrees(newLatRadians);
    qreal newLon = qRadiansToDegrees(newLonRadians);
    
    // Normalize longitude to [-180, 180]
    while (newLon > 180.0) newLon -= 360.0;
    while (newLon < -180.0) newLon += 360.0;
    
    return QGeoCoordinate(newLat, newLon, coordinate.altitude());
}
