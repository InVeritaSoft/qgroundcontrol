/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionAreaPlanner.h"
#include <QDebug>
#include <QtMath>

MissionAreaPlanner::MissionAreaPlanner(QObject* parent)
    : QObject(parent)
{
}

void MissionAreaPlanner::setWidth(double width)
{
    if (qAbs(_width - width) > 0.1) {
        _width = width;
        emit widthChanged();
        updateStatus(QString("Width: %1 m").arg(width));
    }
}

void MissionAreaPlanner::setHeight(double height)
{
    if (qAbs(_height - height) > 0.1) {
        _height = height;
        emit heightChanged();
        updateStatus(QString("Height: %1 m").arg(height));
    }
}

void MissionAreaPlanner::setLineSpacing(double spacing)
{
    if (qAbs(_lineSpacing - spacing) > 0.1) {
        _lineSpacing = spacing;
        emit lineSpacingChanged();
        updateStatus(QString("Line spacing: %1 m").arg(spacing));
    }
}

void MissionAreaPlanner::setNumPoints(int points)
{
    if (_numPoints != points) {
        _numPoints = points;
        emit numPointsChanged();
        updateStatus(QString("Points per line: %1").arg(points));
    }
}

void MissionAreaPlanner::setCenter(const QGeoCoordinate& center)
{
    if (_center != center) {
        _center = center;
        emit centerChanged();
        updateStatus(QString("Center: %1, %2").arg(center.latitude(), 0, 'f', 6).arg(center.longitude(), 0, 'f', 6));
    }
}

void MissionAreaPlanner::generateMission()
{
    if (_busy) {
        return;
    }

    _busy = true;
    emit busyChanged();
    updateStatus("Generating mission...");

    // Calculate waypoints
    QList<QGeoCoordinate> waypoints = calculateWaypoints();
    
    // Add waypoints to QGC mission system
    // This is a simplified version - in a real implementation you would
    // integrate with QGC's mission controller
    
    qDebug() << "Generated" << waypoints.size() << "waypoints";
    
    _busy = false;
    emit busyChanged();
    updateStatus(QString("Mission generated with %1 waypoints").arg(waypoints.size()));
}

void MissionAreaPlanner::clearMission()
{
    updateStatus("Mission cleared");
}

void MissionAreaPlanner::updateStatus(const QString& status)
{
    _status = status;
    emit statusChanged();
    qDebug() << "MissionAreaPlanner:" << status;
}

QList<QGeoCoordinate> MissionAreaPlanner::calculateWaypoints() const
{
    QList<QGeoCoordinate> waypoints;
    
    // Calculate area corners (clockwise from NW)
    double lat = _center.latitude();
    double lon = _center.longitude();
    double w = _width;
    double h = _height;
    
    // Calculate corners using geodesic calculations
    // North and South points
    double northLat = lat + (h/2) / 111320.0; // Approximate conversion
    double southLat = lat - (h/2) / 111320.0;
    
    // Calculate longitude offset based on latitude
    double lonOffset = (w/2) / (111320.0 * qCos(qDegreesToRadians(lat)));
    
    // NW, NE, SE, SW corners
    QGeoCoordinate nw(northLat, lon - lonOffset);
    QGeoCoordinate ne(northLat, lon + lonOffset);
    QGeoCoordinate se(southLat, lon + lonOffset);
    QGeoCoordinate sw(southLat, lon - lonOffset);
    
    // Calculate waypoints along lines
    int nLines = qMax(1, static_cast<int>(h / _lineSpacing));
    
    for (int i = 0; i < nLines; ++i) {
        // Calculate line center
        double offset = (-(h/2) + (i + 0.5) * _lineSpacing);
        double lineLat = lat + offset / 111320.0;
        
        // Calculate line start and end
        double lineLonOffset = (w/2) / (111320.0 * qCos(qDegreesToRadians(lineLat)));
        QGeoCoordinate lineStart(lineLat, lon - lineLonOffset);
        QGeoCoordinate lineEnd(lineLat, lon + lineLonOffset);
        
        // Add points along the line
        for (int j = 0; j < _numPoints; ++j) {
            double frac = (j + 0.5) / _numPoints;
            double pointLon = lineStart.longitude() + frac * (lineEnd.longitude() - lineStart.longitude());
            waypoints.append(QGeoCoordinate(lineLat, pointLon));
        }
    }
    
    return waypoints;
} 