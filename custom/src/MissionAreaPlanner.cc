/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionAreaPlanner.h"
#include <QDebug>
#include <QtMath>

MissionAreaPlanner::MissionAreaPlanner(QObject *parent)
    : QObject(parent)
    , _center(37.7749, -122.4194) // Default to San Francisco
{
    calculateBounds();
    generateGrid();
}

void MissionAreaPlanner::setCenter(const QGeoCoordinate &center)
{
    if (_center != center) {
        _center = center;
        calculateBounds();
        generateGrid();
        emit centerChanged();
    }
}

void MissionAreaPlanner::setWidth(double width)
{
    if (_width != width && width > 0) {
        _width = width;
        calculateBounds();
        generateGrid();
        emit widthChanged();
    }
}

void MissionAreaPlanner::setHeight(double height)
{
    if (_height != height && height > 0) {
        _height = height;
        calculateBounds();
        generateGrid();
        emit heightChanged();
    }
}

void MissionAreaPlanner::setLineSpacing(double spacing)
{
    if (_lineSpacing != spacing && spacing > 0) {
        _lineSpacing = spacing;
        generateGrid();
        emit lineSpacingChanged();
    }
}

void MissionAreaPlanner::setPointsPerLine(int points)
{
    if (_pointsPerLine != points && points > 0) {
        _pointsPerLine = points;
        generateGrid();
        emit pointsPerLineChanged();
    }
}

void MissionAreaPlanner::updateGrid()
{
    generateGrid();
}

void MissionAreaPlanner::setAreaFromCenter(const QGeoCoordinate &center, double width, double height)
{
    bool changed = false;
    
    if (_center != center) {
        _center = center;
        changed = true;
    }
    
    if (_width != width && width > 0) {
        _width = width;
        changed = true;
    }
    
    if (_height != height && height > 0) {
        _height = height;
        changed = true;
    }
    
    if (changed) {
        calculateBounds();
        generateGrid();
        emit centerChanged();
        emit widthChanged();
        emit heightChanged();
    }
}

QVariantList MissionAreaPlanner::generateMissionWaypoints() const
{
    QVariantList waypoints;
    
    for (const QVariant &pointVar : _gridPoints) {
        QGeoCoordinate coord = pointVar.value<QGeoCoordinate>();
        if (coord.isValid()) {
            QVariantMap waypoint;
            waypoint["coordinate"] = QVariant::fromValue(coord);
            waypoint["command"] = 16; // MAV_CMD_NAV_WAYPOINT
            waypoint["frame"] = 0;    // MAV_FRAME_GLOBAL
            waypoint["autocontinue"] = true;
            waypoints.append(waypoint);
        }
    }
    
    return waypoints;
}

void MissionAreaPlanner::calculateBounds()
{
    // Calculate the bounding rectangle based on center and dimensions
    // Convert meters to approximate degrees (rough approximation)
    const double metersPerLatDegree = 111320.0; // meters per degree of latitude
    const double metersPerLonDegree = metersPerLatDegree * qCos(qDegreesToRadians(_center.latitude()));
    
    double latOffset = _height / (2.0 * metersPerLatDegree);
    double lonOffset = _width / (2.0 * metersPerLonDegree);
    
    QGeoCoordinate topLeft(_center.latitude() + latOffset, _center.longitude() - lonOffset);
    QGeoCoordinate bottomRight(_center.latitude() - latOffset, _center.longitude() + lonOffset);
    
    _bounds = QGeoRectangle(topLeft, bottomRight);
    emit boundsChanged();
}

void MissionAreaPlanner::generateGrid()
{
    _gridPoints.clear();
    _gridLines.clear();
    
    if (!_center.isValid() || _width <= 0 || _height <= 0 || _lineSpacing <= 0 || _pointsPerLine <= 0) {
        emit gridPointsChanged();
        emit gridLinesChanged();
        return;
    }
    
    // Calculate grid parameters
    const double metersPerLatDegree = 111320.0;
    const double metersPerLonDegree = metersPerLatDegree * qCos(qDegreesToRadians(_center.latitude()));
    
    double latOffset = _height / (2.0 * metersPerLatDegree);
    double lonOffset = _width / (2.0 * metersPerLonDegree);
    
    // Generate grid lines (parallel to width)
    int numLines = qMax(1, static_cast<int>(_height / _lineSpacing) + 1);
    double lineSpacingDegrees = _height / (metersPerLatDegree * (numLines - 1));
    
    for (int i = 0; i < numLines; ++i) {
        double lat = _center.latitude() + latOffset - (i * lineSpacingDegrees);
        
        // Create line coordinates
        QVariantList lineCoords;
        for (int j = 0; j < _pointsPerLine; ++j) {
            double lon = _center.longitude() - lonOffset + (j * 2.0 * lonOffset / (_pointsPerLine - 1));
            QGeoCoordinate coord(lat, lon);
            lineCoords.append(QVariant::fromValue(coord));
            
            // Add to grid points
            _gridPoints.append(QVariant::fromValue(coord));
        }
        _gridLines.append(lineCoords);
    }
    
    emit gridPointsChanged();
    emit gridLinesChanged();
}

QGeoCoordinate MissionAreaPlanner::offsetCoordinate(const QGeoCoordinate &base, double latOffset, double lonOffset) const
{
    return QGeoCoordinate(base.latitude() + latOffset, base.longitude() + lonOffset);
} 