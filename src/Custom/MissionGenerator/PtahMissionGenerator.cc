#include "PtahMissionGenerator.h"
#include <QtMath>
#include <QtPositioning/QGeoCoordinate>
#include <QObject>
#include <QList>

PtahMissionGenerator::PtahMissionGenerator(QObject* parent)
    : QObject(parent)
{
}

QGeoCoordinate PtahMissionGenerator::calculateMiddlePoint(const QList<QGeoCoordinate>& coordinates)
{
    // GenCall26: PtahMissionGenerator::calculateMiddlePoint() - Calculate center point
    qDebug() << "GenCall26: PtahMissionGenerator::calculateMiddlePoint() - Calculating center point";
    if (coordinates.isEmpty()) {
        return QGeoCoordinate();
    }
    
    if (coordinates.size() == 1) {
        return coordinates.first();
    }
    
    // GenCall27: Calculate average latitude and longitude
    qDebug() << "GenCall27: Calculating average latitude and longitude";
    double sumLat = 0.0;
    double sumLng = 0.0;
    int validCount = 0;
    
    for (const QGeoCoordinate& coord : coordinates) {
        if (coord.isValid()) {
            sumLat += coord.latitude();
            sumLng += coord.longitude();
            validCount++;
        }
    }
    
    if (validCount == 0) {
        return QGeoCoordinate();
    }
    
    double avgLat = sumLat / validCount;
    double avgLng = sumLng / validCount;
    
    return QGeoCoordinate(avgLat, avgLng);
}

QGeoCoordinate PtahMissionGenerator::calculateNewCoordinates(const QGeoCoordinate& reference, double bearing, double distanceMeters)
{
    if (!reference.isValid()) {
        return QGeoCoordinate();
    }
    
    // Earth's radius in meters
    const double earthRadius = 6371000.0;
    
    // Convert bearing from degrees to radians
    double bearingRad = qDegreesToRadians(bearing);
    
    // Convert distance to angular distance
    double angularDistance = distanceMeters / earthRadius;
    
    // Convert reference coordinates to radians
    double lat1Rad = qDegreesToRadians(reference.latitude());
    double lng1Rad = qDegreesToRadians(reference.longitude());
    
    // Calculate new latitude
    double lat2Rad = qAsin(qSin(lat1Rad) * qCos(angularDistance) + 
                          qCos(lat1Rad) * qSin(angularDistance) * qCos(bearingRad));
    
    // Calculate new longitude
    double lng2Rad = lng1Rad + qAtan2(qSin(bearingRad) * qSin(angularDistance) * qCos(lat1Rad),
                                      qCos(angularDistance) - qSin(lat1Rad) * qSin(lat2Rad));
    
    // Debug logging
    qDebug() << "calculateNewCoordinates - Input: ref=" << reference.toString() 
             << "bearing=" << bearing << "distance=" << distanceMeters;
    qDebug() << "calculateNewCoordinates - Calculated: lat2Rad=" << lat2Rad 
             << "lng2Rad=" << lng2Rad;
    
    // Normalize longitude to [-180, 180]
    while (lng2Rad > M_PI) lng2Rad -= 2.0 * M_PI;
    while (lng2Rad < -M_PI) lng2Rad += 2.0 * M_PI;
    
    // Convert back to degrees
    double newLat = qRadiansToDegrees(lat2Rad);
    double newLng = qRadiansToDegrees(lng2Rad);
    
    QGeoCoordinate result(newLat, newLng, reference.altitude());
    qDebug() << "calculateNewCoordinates - Final result:" << result.toString();
    
    return result;
}

QList<QGeoCoordinate> PtahMissionGenerator::generateCoordinatesInBothDirections(const QGeoCoordinate& reference, double bearing, double gapMeters, double totalDistanceMeters)
{
    // GenCall28: PtahMissionGenerator::generateCoordinatesInBothDirections() - Generate waypoint grid
    qDebug() << "GenCall28: PtahMissionGenerator::generateCoordinatesInBothDirections() - Generating waypoint grid";
    QList<QGeoCoordinate> coordinates;
    
    if (!reference.isValid() || gapMeters <= 0 || totalDistanceMeters <= 0) {
        return coordinates;
    }
    
    // GenCall30: Calculate how many points to generate in each direction
    qDebug() << "GenCall30: Calculating how many points to generate in each direction";
    int pointsInEachDirection = static_cast<int>(totalDistanceMeters / gapMeters);
    
    // GenCall31: Generate coordinates in sequential order (1,2,3,4,5,6,7,8...)
    qDebug() << "GenCall31: Generating coordinates in sequential order";
    qDebug() << "Reference point:" << reference.toString() << "Bearing:" << bearing << "Gap:" << gapMeters << "Total distance:" << totalDistanceMeters;
    qDebug() << "Points in each direction:" << pointsInEachDirection;
    
    // Generate waypoints in a simple linear pattern: 1,2,3,4,5,6,7,8...
    // Create a straight line of waypoints starting from the reference point
    
    // Add the reference point as waypoint 1
    coordinates.append(reference);
    qDebug() << "Added waypoint 1 (reference):" << reference.toString();
    
    // Generate waypoints in a straight line
    for (int i = 1; i <= pointsInEachDirection; i++) {
        double distance = i * gapMeters;
        QGeoCoordinate waypoint = calculateNewCoordinates(reference, bearing, distance);
        if (waypoint.isValid()) {
            coordinates.append(waypoint);
            qDebug() << "Added waypoint" << coordinates.size() << "at distance" << distance << "m:" << waypoint.toString();
        }
    }
    
    return coordinates;
}

double PtahMissionGenerator::calculateBearing(const QGeoCoordinate& from, const QGeoCoordinate& to)
{
    if (!from.isValid() || !to.isValid()) {
        return 0.0;
    }
    
    return from.azimuthTo(to);
}

QList<QList<QGeoCoordinate>> PtahMissionGenerator::distributeWaypointsAmongDrones(const QList<QGeoCoordinate>& waypoints, int droneCount)
{
    QList<QList<QGeoCoordinate>> droneWaypoints;
    
    if (waypoints.isEmpty() || droneCount <= 0) {
        return droneWaypoints;
    }
    
    // Initialize drone waypoint lists
    for (int i = 0; i < droneCount; i++) {
        droneWaypoints.append(QList<QGeoCoordinate>());
    }
    
    // Distribute waypoints in alternating pattern
    // Drone 0 (ID 2): 1,3,5,7,9,11,13...
    // Drone 1 (ID 3): 2,4,6,8,10,12...
    for (int waypointIndex = 0; waypointIndex < waypoints.size(); waypointIndex++) {
        int droneIndex = waypointIndex % droneCount;
        droneWaypoints[droneIndex].append(waypoints[waypointIndex]);
    }
    
    return droneWaypoints;
}

QGeoCoordinate PtahMissionGenerator::calculateSafeObservationPosition(const QGeoCoordinate& missionCenter, const QList<QGeoCoordinate>& waypoints, int altitude)
{
    // GenCall34: Calculate safe observation position for drone ID 1
    qDebug() << "GenCall34: Calculating safe observation position for drone ID 1";
    
    if (!missionCenter.isValid() || waypoints.isEmpty()) {
        qDebug() << "Invalid mission center or no waypoints provided";
        return QGeoCoordinate();
    }
    
    // Calculate the mission area bounds
    double minLat = waypoints[0].latitude();
    double maxLat = waypoints[0].latitude();
    double minLng = waypoints[0].longitude();
    double maxLng = waypoints[0].longitude();
    
    for (const QGeoCoordinate& coord : waypoints) {
        if (coord.latitude() < minLat) minLat = coord.latitude();
        if (coord.latitude() > maxLat) maxLat = coord.latitude();
        if (coord.longitude() < minLng) minLng = coord.longitude();
        if (coord.longitude() > maxLng) maxLng = coord.longitude();
    }
    
    // Calculate mission area dimensions
    double latSpan = maxLat - minLat;
    double lngSpan = maxLng - minLng;
    
    qDebug() << "Mission area bounds - Lat:" << minLat << "to" << maxLat << "Lng:" << minLng << "to" << maxLng;
    qDebug() << "Mission area span - Lat:" << latSpan << "Lng:" << lngSpan;
    
    // Calculate mission area center
    double centerLat = (minLat + maxLat) / 2.0;
    double centerLng = (minLng + maxLng) / 2.0;
    
    // Calculate safe distance from mission area (2x the mission area size)
    double maxDimension = qMax(latSpan, lngSpan);
    double safeDistance = maxDimension * 2.0; // 2x safety margin to ensure it's well outside flight path
    
    // Choose the best observation position that's NOT in the flight path
    // Priority: 1) Perpendicular to the main mission axis, 2) Away from populated areas
    double observerLat, observerLng;
    
    if (latSpan > lngSpan) {
        // Mission is more north-south oriented, position east or west
        // Choose the side that's further from the center
        if (centerLng > 0) {
            // Mission is in eastern hemisphere, position to the west
            observerLat = centerLat;
            observerLng = centerLng - safeDistance;
            qDebug() << "Mission is north-south oriented, positioning observer WEST of mission area";
        } else {
            // Mission is in western hemisphere, position to the east
            observerLat = centerLat;
            observerLng = centerLng + safeDistance;
            qDebug() << "Mission is north-south oriented, positioning observer EAST of mission area";
        }
    } else {
        // Mission is more east-west oriented, position north or south
        // Choose the side that's further from the center
        if (centerLat > 0) {
            // Mission is in northern hemisphere, position to the south
            observerLat = centerLat - safeDistance;
            observerLng = centerLng;
            qDebug() << "Mission is east-west oriented, positioning observer SOUTH of mission area";
        } else {
            // Mission is in southern hemisphere, position to the north
            observerLat = centerLat + safeDistance;
            observerLng = centerLng;
            qDebug() << "Mission is east-west oriented, positioning observer NORTH of mission area";
        }
    }
    
    QGeoCoordinate observerPosition(observerLat, observerLng, altitude + 20); // 20m higher than mission altitude
    
    qDebug() << "Mission center: Lat" << centerLat << "Lng" << centerLng;
    qDebug() << "Safe distance from mission area:" << safeDistance << "m";
    qDebug() << "Safe observation position calculated:" << observerPosition.toString();
    qDebug() << "Observer altitude:" << (altitude + 20) << "m (20m above mission altitude)";
    
    return observerPosition;
}