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

/**
 * @file AreaPlanEditor_Properties.cc
 * @brief Property setters and getters for AreaPlanEditor
 * 
 * This file contains all the property setter and getter methods for
 * the AreaPlanEditor class. These methods handle the QML property
 * bindings and ensure proper signal emission when values change.
 */

// Area dimension properties
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
    // Clamp to minimum of 1 to ensure at least one waypoint per line
    int clamped = points < 1 ? 1 : points;
    if (_numPoints == clamped) {
        return;
    }
    _numPoints = clamped;
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

void AreaPlanEditor::setLandAtTargetReturn(bool enabled)
{
    if (_landAtTargetReturn == enabled) {
        return;
    }
    _landAtTargetReturn = enabled;
    emit landAtTargetReturnChanged();
}

// Multi-drone properties
void AreaPlanEditor::setDroneCount(int count)
{
    if (droneCount() == count) {
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

void AreaPlanEditor::setPerTargetSeparationS(qreal seconds)
{
    if (qFuzzyCompare(_perTargetSeparationS, seconds)) {
        return;
    }
    _perTargetSeparationS = seconds;
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
    if (qFuzzyCompare(_targetHoldTimeS, seconds)) {
        return;
    }
    _targetHoldTimeS = seconds;
    emit targetHoldTimeSChanged();
}

void AreaPlanEditor::setHomeTurnaroundWaitS(qreal seconds)
{
    if (qFuzzyCompare(_homeTurnaroundWaitS, seconds)) {
        return;
    }
    _homeTurnaroundWaitS = seconds;
    emit homeTurnaroundWaitSChanged();
}

void AreaPlanEditor::setPayloadReleaseEnabled(bool enabled)
{
    if (_payloadReleaseEnabled == enabled) {
        return;
    }
    _payloadReleaseEnabled = enabled;
    emit payloadReleaseEnabledChanged();
}

void AreaPlanEditor::setTakeoffHeight(qreal height)
{
    if (qFuzzyCompare(_takeoffHeight, height)) {
        return;
    }
    _takeoffHeight = height;
    emit takeoffHeightChanged();
}

void AreaPlanEditor::setFormationSpacing(qreal spacing)
{
    // Enforce at least 1m spacing between vehicles in formation
    qreal clamped = spacing < 1.0 ? 1.0 : spacing;
    if (qFuzzyCompare(_formationSpacing, clamped)) {
        return;
    }
    
    _formationSpacing = clamped;
    calculateFormationPositions();
    emit formationSpacingChanged();
}

void AreaPlanEditor::setDrawingPresent(bool present)
{
    if (_drawingPresent == present) return;
    _drawingPresent = present;
    emit drawingPresentChanged(present);
}

void AreaPlanEditor::setProgressOperation(const QString& operation)
{
    if (_currentOperation == operation) {
        return;
    }
    _currentOperation = operation;
    emit currentOperationChanged();
}

void AreaPlanEditor::setCacheSize(int size)
{
    if (_cacheSize == size) {
        return;
    }
    _cacheSize = size;
    emit cacheSizeChanged();
}
