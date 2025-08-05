/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AreaPlanEditor.h"
#include <QDebug>
#include <QtMath>
#include <QtPositioning>
#include <QVariant>

AreaPlanEditor::AreaPlanEditor(QObject* parent)
    : QObject(parent)
{
}

void AreaPlanEditor::setAreaWidth(qreal width)
{
    if (qFuzzyCompare(_areaWidth, width))
        return;
    _areaWidth = width;
    emit areaWidthChanged();
}

void AreaPlanEditor::setAreaHeight(qreal height)
{
    if (qFuzzyCompare(_areaHeight, height))
        return;
    _areaHeight = height;
    emit areaHeightChanged();
}

void AreaPlanEditor::setLineSpacing(qreal spacing)
{
    if (qFuzzyCompare(_lineSpacing, spacing))
        return;
    _lineSpacing = spacing;
    emit lineSpacingChanged();
}

void AreaPlanEditor::setNumPoints(int points)
{
    if (_numPoints == points)
        return;
    _numPoints = points;
    emit numPointsChanged();
}

void AreaPlanEditor::setMissionAltitude(qreal altitude)
{
    if (qFuzzyCompare(_missionAltitude, altitude))
        return;
    _missionAltitude = altitude;
    emit missionAltitudeChanged();
}

void AreaPlanEditor::setAreaCenter(const QGeoCoordinate& center)
{
    if (_areaCenter == center)
        return;
    _areaCenter = center;
    emit areaCenterChanged();
}

void AreaPlanEditor::setHomeLocation(const QGeoCoordinate& location)
{
    if (_homeLocation == location)
        return;
    _homeLocation = location;
    emit homeLocationChanged();
}

void AreaPlanEditor::moveAreaNorth()
{
    const qreal step = 0.5; // meters
    const qreal newLat = _areaCenter.latitude() + (step / 111320.0); // Approximate conversion
    setAreaCenter(QGeoCoordinate(newLat, _areaCenter.longitude()));
    updateStatus(QStringLiteral("Area moved north"));
}

void AreaPlanEditor::moveAreaSouth()
{
    const qreal step = 0.5; // meters
    const qreal newLat = _areaCenter.latitude() - (step / 111320.0); // Approximate conversion
    setAreaCenter(QGeoCoordinate(newLat, _areaCenter.longitude()));
    updateStatus(QStringLiteral("Area moved south"));
}

void AreaPlanEditor::moveAreaEast()
{
    const qreal step = 0.5; // meters
    const qreal newLon = _areaCenter.longitude() + (step / (111320.0 * qCos(_areaCenter.latitude() * M_PI / 180.0)));
    setAreaCenter(QGeoCoordinate(_areaCenter.latitude(), newLon));
    updateStatus(QStringLiteral("Area moved east"));
}

void AreaPlanEditor::moveAreaWest()
{
    const qreal step = 0.5; // meters
    const qreal newLon = _areaCenter.longitude() - (step / (111320.0 * qCos(_areaCenter.latitude() * M_PI / 180.0)));
    setAreaCenter(QGeoCoordinate(_areaCenter.latitude(), newLon));
    updateStatus(QStringLiteral("Area moved west"));
}

void AreaPlanEditor::centerArea()
{
    setAreaCenter(_homeLocation);
    updateStatus(QStringLiteral("Area centered on home location"));
}

int AreaPlanEditor::calculateTotalWaypoints() const
{
    const int numLines = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    return numLines * _numPoints;
}

int AreaPlanEditor::calculateFlightTime() const
{
    const int totalWaypoints = calculateTotalWaypoints();
    const int timePerPoint = 2; // minutes per waypoint (including hover time)
    return totalWaypoints * timePerPoint;
}

QVariantList AreaPlanEditor::generateWaypoints()
{
    QVariantList waypoints;
    const int numLines = qMax(1, static_cast<int>(qFloor(_areaHeight / _lineSpacing)));
    
    for (int i = 0; i < numLines; ++i) {
        const qreal offset = (-(_areaHeight/2) + (i + 0.5) * _lineSpacing);
        const QGeoCoordinate lineCenter = calculateOffsetCoordinate(_areaCenter, offset, 180);
        
        for (int j = 0; j < _numPoints; ++j) {
            const qreal frac = (j + 0.5) / _numPoints;
            const qreal ptOffset = (frac - 0.5) * _areaWidth;
            QGeoCoordinate waypoint = calculateOffsetCoordinate(lineCenter, ptOffset, 90);
            waypoint.setAltitude(_missionAltitude);
            waypoints.append(QVariant::fromValue(waypoint));
        }
    }
    
    updateStatus(QStringLiteral("Generated %1 waypoints").arg(waypoints.size()));
    return waypoints;
}

QGeoCoordinate AreaPlanEditor::calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const
{
    // Simplified geodesic calculation
    const qreal R = 6378137.0; // Earth radius in meters
    const qreal d = meters;
    const qreal bearingRad = bearing * M_PI / 180.0;
    const qreal lat1 = coord.latitude() * M_PI / 180.0;
    const qreal lon1 = coord.longitude() * M_PI / 180.0;
    
    const qreal lat2 = qAsin(qSin(lat1) * qCos(d / R) + qCos(lat1) * qSin(d / R) * qCos(bearingRad));
    const qreal lon2 = lon1 + qAtan2(qSin(bearingRad) * qSin(d / R) * qCos(lat1), qCos(d / R) - qSin(lat1) * qSin(lat2));
    
    return QGeoCoordinate(lat2 * 180.0 / M_PI, lon2 * 180.0 / M_PI);
}

void AreaPlanEditor::saveMissionFile()
{
    generateWaypoints(); // Generate waypoints for demo
    // TODO: Implement actual file saving using QGroundControl's mission system
    updateStatus(QStringLiteral("Mission file saved (demo)"));
}

void AreaPlanEditor::uploadToVehicle()
{
    // TODO: Implement vehicle upload using QGroundControl's mission system
    updateStatus(QStringLiteral("Upload to vehicle not implemented"));
}

void AreaPlanEditor::startMission()
{
    // TODO: Implement mission start using QGroundControl's mission system
    updateStatus(QStringLiteral("Start mission not implemented"));
}

void AreaPlanEditor::updateStatus(const QString& message)
{
    qDebug() << "AreaPlanEditor:" << message;
    emit statusChanged(message);
} 