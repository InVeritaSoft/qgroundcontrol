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
#include <cmath>

MissionAreaPlanner::MissionAreaPlanner(QObject* parent)
    : QObject(parent)
    , _areaWidth(100.0)
    , _areaHeight(100.0)
    , _lineSpacing(10.0)
    , _pointsPerLine(10)
    , _status("Ready")
    , _statusColor(Qt::green)
    , _busy(false)
{
}

void MissionAreaPlanner::setAreaCenter(const QGeoCoordinate& center)
{
    if (_areaCenter != center) {
        _areaCenter = center;
        emit areaCenterChanged();
        updateArea();
    }
}

void MissionAreaPlanner::setAreaWidth(double width)
{
    if (_areaWidth != width) {
        _areaWidth = width;
        emit areaWidthChanged();
        updateArea();
    }
}

void MissionAreaPlanner::setAreaHeight(double height)
{
    if (_areaHeight != height) {
        _areaHeight = height;
        emit areaHeightChanged();
        updateArea();
    }
}

void MissionAreaPlanner::setLineSpacing(double spacing)
{
    if (_lineSpacing != spacing) {
        _lineSpacing = spacing;
        emit lineSpacingChanged();
        updateGrid();
    }
}

void MissionAreaPlanner::setPointsPerLine(int points)
{
    if (_pointsPerLine != points) {
        _pointsPerLine = points;
        emit pointsPerLineChanged();
        updateGrid();
    }
}

double MissionAreaPlanner::geodesicDistance(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2)
{
    double lat1 = coord1.latitude() * PI / 180.0;
    double lon1 = coord1.longitude() * PI / 180.0;
    double lat2 = coord2.latitude() * PI / 180.0;
    double lon2 = coord2.longitude() * PI / 180.0;

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return EARTH_RADIUS * c;
}

QGeoCoordinate MissionAreaPlanner::coordinateAtDistance(const QGeoCoordinate& referenceCoord, double distance, double bearing)
{
    double lat1 = referenceCoord.latitude() * PI / 180.0;
    double lon1 = referenceCoord.longitude() * PI / 180.0;
    double brng = bearing * PI / 180.0;

    double angularDistance = distance / EARTH_RADIUS;

    double lat2 = asin(sin(lat1) * cos(angularDistance) +
                      cos(lat1) * sin(angularDistance) * cos(brng));

    double lon2 = lon1 + atan2(sin(brng) * sin(angularDistance) * cos(lat1),
                              cos(angularDistance) - sin(lat1) * sin(lat2));

    return QGeoCoordinate(lat2 * 180.0 / PI, lon2 * 180.0 / PI);
}

double MissionAreaPlanner::calculateBearing(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2)
{
    double lat1 = coord1.latitude() * PI / 180.0;
    double lon1 = coord1.longitude() * PI / 180.0;
    double lat2 = coord2.latitude() * PI / 180.0;
    double lon2 = coord2.longitude() * PI / 180.0;

    double dLon = lon2 - lon1;

    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

    double bearing = atan2(y, x) * 180.0 / PI;
    return fmod(bearing + 360, 360);
}

void MissionAreaPlanner::updateArea()
{
    if (!_areaCenter.isValid()) {
        setStatus("Invalid area center", Qt::red);
        return;
    }

    setStatus("Updating area...", Qt::yellow);
    setBusy(true);

    // Calculate area corners
    double halfWidth = _areaWidth / 2.0;
    double halfHeight = _areaHeight / 2.0;

    // Calculate corners using geodesic calculations
    QGeoCoordinate northWest = coordinateAtDistance(_areaCenter, halfWidth, 270);
    northWest = coordinateAtDistance(northWest, halfHeight, 0);

    QGeoCoordinate northEast = coordinateAtDistance(_areaCenter, halfWidth, 90);
    northEast = coordinateAtDistance(northEast, halfHeight, 0);

    QGeoCoordinate southWest = coordinateAtDistance(_areaCenter, halfWidth, 270);
    southWest = coordinateAtDistance(southWest, halfHeight, 180);

    QGeoCoordinate southEast = coordinateAtDistance(_areaCenter, halfWidth, 90);
    southEast = coordinateAtDistance(southEast, halfHeight, 180);

    // Store area corners
    _areaCorners.clear();
    _areaCorners.append(QVariant::fromValue(northWest));
    _areaCorners.append(QVariant::fromValue(northEast));
    _areaCorners.append(QVariant::fromValue(southEast));
    _areaCorners.append(QVariant::fromValue(southWest));

    setStatus("Area updated", Qt::green);
    setBusy(false);
    emit areaCornersChanged();
    emit areaUpdated();

    // Update grid after area update
    updateGrid();
}

void MissionAreaPlanner::updateGrid()
{
    if (!_areaCenter.isValid() || _areaCorners.isEmpty()) {
        return;
    }

    setStatus("Generating grid...", Qt::yellow);
    setBusy(true);

    _gridLines.clear();
    _waypoints.clear();

    // Calculate grid lines parallel to width (North-South lines)
    int numLines = ceil(_areaHeight / _lineSpacing) + 1;
    double lineSpacingActual = _areaHeight / (numLines - 1);

    for (int i = 0; i < numLines; i++) {
        double progress = static_cast<double>(i) / (numLines - 1);
        
        QGeoCoordinate startCoord = interpolateCoordinate(
            _areaCorners[0].value<QGeoCoordinate>(),  // NW
            _areaCorners[3].value<QGeoCoordinate>(),  // SW
            progress
        );
        
        QGeoCoordinate endCoord = interpolateCoordinate(
            _areaCorners[1].value<QGeoCoordinate>(),  // NE
            _areaCorners[2].value<QGeoCoordinate>(),  // SE
            progress
        );

        QVariantMap line;
        line["start"] = QVariant::fromValue(startCoord);
        line["end"] = QVariant::fromValue(endCoord);
        
        QVariantList points;
        for (int j = 0; j < _pointsPerLine; j++) {
            double pointProgress = static_cast<double>(j) / (_pointsPerLine - 1);
            QGeoCoordinate pointCoord = interpolateCoordinate(startCoord, endCoord, pointProgress);
            
            points.append(QVariant::fromValue(pointCoord));
            _waypoints.append(QVariant::fromValue(pointCoord));
        }
        
        line["points"] = points;
        _gridLines.append(QVariant::fromValue(line));
    }

    setStatus(QString("Grid generated: %1 waypoints").arg(_waypoints.size()), Qt::green);
    setBusy(false);
    emit gridLinesChanged();
    emit waypointsChanged();
    emit gridUpdated();
}

QGeoCoordinate MissionAreaPlanner::interpolateCoordinate(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2, double progress)
{
    double lat = coord1.latitude() + (coord2.latitude() - coord1.latitude()) * progress;
    double lon = coord1.longitude() + (coord2.longitude() - coord1.longitude()) * progress;
    return QGeoCoordinate(lat, lon);
}

void MissionAreaPlanner::moveArea(double deltaX, double deltaY)
{
    if (!_areaCenter.isValid()) {
        return;
    }

    // Calculate new center position
    double bearingX = deltaX >= 0 ? 90 : 270;
    double bearingY = deltaY >= 0 ? 180 : 0;

    QGeoCoordinate newCenter = _areaCenter;

    if (fabs(deltaX) > 0) {
        newCenter = coordinateAtDistance(newCenter, fabs(deltaX), bearingX);
    }

    if (fabs(deltaY) > 0) {
        newCenter = coordinateAtDistance(newCenter, fabs(deltaY), bearingY);
    }

    setAreaCenter(newCenter);
}

void MissionAreaPlanner::generateMission()
{
    if (_waypoints.isEmpty()) {
        setStatus("No waypoints to generate mission", Qt::red);
        return;
    }

    setStatus("Generating mission...", Qt::yellow);
    setBusy(true);

    // This would integrate with QGC's mission controller
    // For now, we'll just emit a signal with the waypoints
    setStatus(QString("Mission generated with %1 waypoints").arg(_waypoints.size()), Qt::green);
    setBusy(false);
    emit missionGenerated();
}

void MissionAreaPlanner::setStatus(const QString& status, const QColor& color)
{
    if (_status != status || _statusColor != color) {
        _status = status;
        _statusColor = color;
        emit statusChanged();
        emit statusColorChanged();
    }
}

void MissionAreaPlanner::setBusy(bool busy)
{
    if (_busy != busy) {
        _busy = busy;
        emit busyChanged();
    }
} 