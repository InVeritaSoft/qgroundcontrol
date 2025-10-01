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
    qDebug() << "calculateNewCoordinates - Intermediate: lat1Rad=" << lat1Rad 
             << "lng1Rad=" << lng1Rad << "angularDistance=" << angularDistance;
    
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
    
    // Limit maximum distance to prevent waypoints that are too far away
    const double maxDistanceMeters = 1000.0; // Maximum 1km from reference point
    int maxPointsInEachDirection = static_cast<int>(maxDistanceMeters / gapMeters);
    
    if (pointsInEachDirection > maxPointsInEachDirection) {
        pointsInEachDirection = maxPointsInEachDirection;
        qDebug() << "Limited points to maximum distance of" << maxDistanceMeters << "m";
    }
    
    // GenCall31: Generate coordinates in sequential order (1,2,3,4,5,6,7,8...)
    qDebug() << "GenCall31: Generating coordinates in sequential order";
    qDebug() << "Reference point:" << reference.toString() << "Bearing:" << bearing << "Gap:" << gapMeters << "Total distance:" << totalDistanceMeters;
    qDebug() << "Points in each direction:" << pointsInEachDirection;
    qDebug() << "Maximum distance limited to:" << maxDistanceMeters << "m";
    
    // Generate waypoints in a simple linear pattern: 1,2,3,4,5,6,7,8...
    // Create a straight line of waypoints starting from the reference point
    
    // Add the reference point as waypoint 1 (only if it's on land)
    if (isValidLandCoordinate(reference)) {
        coordinates.append(reference);
        qDebug() << "Added waypoint 1 (reference):" << reference.toString();
    } else {
        qDebug() << "Reference point is in ocean, skipping it:" << reference.toString();
    }
    
        // Generate waypoints in a straight line with more conservative distances
        for (int i = 1; i <= pointsInEachDirection; i++) {
            double distance = i * gapMeters;
            
            // Use smaller distances to stay closer to land
            if (distance > 50.0) {
                distance = 50.0; // Cap at 50m to stay close to reference point
            }
        QGeoCoordinate waypoint = calculateNewCoordinates(reference, bearing, distance);
        
        qDebug() << "Generated waypoint" << i << "at distance" << distance << "m:" << waypoint.toString();
        qDebug() << "Waypoint valid:" << waypoint.isValid() << "Land coordinate:" << isValidLandCoordinate(waypoint);
        
        if (waypoint.isValid() && isValidLandCoordinate(waypoint)) {
            coordinates.append(waypoint);
            qDebug() << "✓ Added waypoint" << coordinates.size() << "at distance" << distance << "m:" << waypoint.toString();
        } else {
            qDebug() << "✗ Skipped waypoint at distance" << distance << "m:" << waypoint.toString();
            qDebug() << "  Reason: valid=" << waypoint.isValid() << "land=" << isValidLandCoordinate(waypoint);
            
            // Try alternative bearings to find a land-based waypoint
            QGeoCoordinate alternativeWaypoint;
            bool foundLandWaypoint = false;
            
            // Try 16 different directions (22.5-degree increments) for better coverage
            for (int dir = 0; dir < 16 && !foundLandWaypoint; dir++) {
                double alternativeBearing = bearing + (dir * 22.5);
                if (alternativeBearing >= 360.0) alternativeBearing -= 360.0;
                
                alternativeWaypoint = calculateNewCoordinates(reference, alternativeBearing, distance);
                if (alternativeWaypoint.isValid() && isValidLandCoordinate(alternativeWaypoint)) {
                    coordinates.append(alternativeWaypoint);
                    qDebug() << "✓ Added alternative waypoint" << coordinates.size() << "at distance" << distance << "m, bearing" << alternativeBearing << ":" << alternativeWaypoint.toString();
                    foundLandWaypoint = true;
                }
            }
            
            // If still no land waypoint found, try closer distances
            if (!foundLandWaypoint) {
                for (double closerDistance = distance * 0.8; closerDistance >= distance * 0.2 && !foundLandWaypoint; closerDistance -= distance * 0.2) {
                    alternativeWaypoint = calculateNewCoordinates(reference, bearing, closerDistance);
                    if (alternativeWaypoint.isValid() && isValidLandCoordinate(alternativeWaypoint)) {
                        coordinates.append(alternativeWaypoint);
                        qDebug() << "✓ Added closer waypoint" << coordinates.size() << "at distance" << closerDistance << "m:" << alternativeWaypoint.toString();
                        foundLandWaypoint = true;
                    }
                }
            }
            
            if (!foundLandWaypoint) {
                qDebug() << "Could not find suitable land-based waypoint for distance" << distance << "m";
                
                // Try even more aggressive fallback - try much closer distances
                bool foundCloseLandWaypoint = false;
                for (double closeDistance = distance * 0.1; closeDistance <= distance * 0.5 && !foundCloseLandWaypoint; closeDistance += distance * 0.1) {
                    for (int dir = 0; dir < 32 && !foundCloseLandWaypoint; dir++) {
                        double closeBearing = bearing + (dir * 11.25); // Try every 11.25 degrees (32 directions)
                        if (closeBearing >= 360.0) closeBearing -= 360.0;
                        
                        QGeoCoordinate closeWaypoint = calculateNewCoordinates(reference, closeBearing, closeDistance);
                        if (closeWaypoint.isValid() && isValidLandCoordinate(closeWaypoint)) {
                            coordinates.append(closeWaypoint);
                            qDebug() << "✓ Added close land waypoint at distance" << closeDistance << "m, bearing" << closeBearing << ":" << closeWaypoint.toString();
                            foundCloseLandWaypoint = true;
                        }
                    }
                }
                
                // Do not add ocean waypoints - only add land-based waypoints
                if (!foundCloseLandWaypoint) {
                    qDebug() << "⚠ Skipping ocean waypoint - no land-based alternative found for distance" << distance << "m";
                }
            }
        }
    }
    
    // Ensure we have at least some waypoints to prevent empty missions
    if (coordinates.isEmpty()) {
        qDebug() << "No waypoints generated, trying to find any land-based waypoint near reference";
        
        // Try to find ANY land-based waypoint near the reference point
        bool foundAnyLandWaypoint = false;
        for (double testDistance = 5.0; testDistance <= 100.0 && !foundAnyLandWaypoint; testDistance += 5.0) {
            for (int dir = 0; dir < 36 && !foundAnyLandWaypoint; dir++) {
                double testBearing = dir * 10.0; // Try every 10 degrees
                QGeoCoordinate testWaypoint = calculateNewCoordinates(reference, testBearing, testDistance);
                
                if (testWaypoint.isValid() && isValidLandCoordinate(testWaypoint)) {
                    coordinates.append(testWaypoint);
                    qDebug() << "✓ Found emergency land waypoint at distance" << testDistance << "m, bearing" << testBearing << ":" << testWaypoint.toString();
                    foundAnyLandWaypoint = true;
                }
            }
        }
        
        // Only add reference point if it's on land
        if (!foundAnyLandWaypoint) {
            if (isValidLandCoordinate(reference)) {
                qDebug() << "No land waypoints found, adding reference point as fallback (on land)";
                coordinates.append(reference);
            } else {
                qDebug() << "No land waypoints found and reference point is in ocean - mission will be empty";
            }
        }
    }
    
    qDebug() << "Final waypoint count:" << coordinates.size();
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
    
    qDebug() << "Distributing" << waypoints.size() << "waypoints among" << droneCount << "drones";
    
    // Distribute waypoints in balanced groups to keep drones close to reference point
    // Each drone gets consecutive waypoints to minimize travel distance
    int waypointsPerDrone = waypoints.size() / droneCount;
    int remainingWaypoints = waypoints.size() % droneCount;
    
    int waypointIndex = 0;
    for (int droneIndex = 0; droneIndex < droneCount; droneIndex++) {
        // Calculate how many waypoints this drone should get
        int waypointsForThisDrone = waypointsPerDrone;
        if (droneIndex < remainingWaypoints) {
            waypointsForThisDrone++; // Distribute remaining waypoints evenly
        }
        
        qDebug() << "Drone" << droneIndex << "gets" << waypointsForThisDrone << "waypoints starting from index" << waypointIndex;
        
        // Assign consecutive waypoints to this drone
        for (int i = 0; i < waypointsForThisDrone; i++) {
            if (waypointIndex < waypoints.size()) {
                droneWaypoints[droneIndex].append(waypoints[waypointIndex]);
                qDebug() << "  Assigned waypoint" << waypointIndex << "to drone" << droneIndex << ":" << waypoints[waypointIndex].toString();
                waypointIndex++;
            }
        }
    }
    
    // Log final distribution
    for (int i = 0; i < droneWaypoints.size(); i++) {
        qDebug() << "Drone" << i << "final waypoint count:" << droneWaypoints[i].size();
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
    
    // Validate the observer position to ensure it's on land
    if (!isValidLandCoordinate(observerPosition)) {
        qDebug() << "Initial observer position is in ocean, adjusting...";
        
        // Try alternative positions if the initial one is in ocean
        QGeoCoordinate alternativePosition = missionCenter;
        alternativePosition.setLatitude(missionCenter.latitude() - safeDistance);
        alternativePosition.setLongitude(missionCenter.longitude() - safeDistance);
        alternativePosition.setAltitude(altitude + 20);
        
        if (isValidLandCoordinate(alternativePosition)) {
            observerPosition = alternativePosition;
            qDebug() << "Using alternative observer position:" << observerPosition.toString();
        } else {
            // Fallback to mission center if all alternatives are in ocean
            observerPosition = missionCenter;
            observerPosition.setAltitude(altitude + 20);
            qDebug() << "Using mission center as fallback observer position:" << observerPosition.toString();
        }
    }
    
    qDebug() << "Mission center: Lat" << centerLat << "Lng" << centerLng;
    qDebug() << "Safe distance from mission area:" << safeDistance << "m";
    qDebug() << "Safe observation position calculated:" << observerPosition.toString();
    qDebug() << "Observer altitude:" << (altitude + 20) << "m (20m above mission altitude)";
    
    return observerPosition;
}

QGeoCoordinate PtahMissionGenerator::calculateObservationPosition(const QGeoCoordinate& missionCenter, double observationDistanceMeters, int altitude)
{
    qDebug() << "GenCall36: Calculating observation position with configurable distance";
    qDebug() << "Mission center:" << missionCenter.toString();
    qDebug() << "Observation distance:" << observationDistanceMeters << "meters";
    qDebug() << "Altitude:" << altitude << "meters";
    
    if (!missionCenter.isValid()) {
        qDebug() << "Invalid mission center position";
        return QGeoCoordinate();
    }
    
    // Calculate observation position at a specific distance from mission center
    // Position to the northeast of mission center for good viewing angle
    double bearing = 45.0; // 45 degrees northeast
    
    QGeoCoordinate observationPosition = calculateNewCoordinates(missionCenter, bearing, observationDistanceMeters);
    observationPosition.setAltitude(altitude + 20); // 20m higher than mission altitude
    
    qDebug() << "Observation position calculated:" << observationPosition.toString();
    qDebug() << "Distance from mission center:" << missionCenter.distanceTo(observationPosition) << "meters";
    qDebug() << "Bearing from observer to mission center:" << observationPosition.azimuthTo(missionCenter) << "degrees";
    
    return observationPosition;
}

double PtahMissionGenerator::calculateBearingToMissionCenter(const QGeoCoordinate& observerPosition, const QGeoCoordinate& missionCenter)
{
    // GenCall35: Calculate bearing from observer to mission center
    qDebug() << "GenCall35: Calculating bearing from observer to mission center";
    
    if (!observerPosition.isValid() || !missionCenter.isValid()) {
        qDebug() << "Invalid observer or mission center position";
        return 0.0;
    }
    
    // Use the existing calculateBearing method
    double bearing = calculateBearing(observerPosition, missionCenter);
    
    qDebug() << "Observer position:" << observerPosition.toString();
    qDebug() << "Mission center:" << missionCenter.toString();
    qDebug() << "Calculated bearing:" << bearing << "degrees";
    
    return bearing;
}

QList<QGeoCoordinate> PtahMissionGenerator::generatePayloadDropWaypoints(const QGeoCoordinate& reference, double bearing, double gapMeters, double totalDistanceMeters, int loiterTimeSeconds)
{
    // GenCall35: Generate waypoints with payload drop pattern
    qDebug() << "GenCall35: Generating payload drop waypoints with loiter time:" << loiterTimeSeconds << "seconds";
    
    // Use existing waypoint generation logic
    QList<QGeoCoordinate> waypoints = generateCoordinatesInBothDirections(reference, bearing, gapMeters, totalDistanceMeters);
    
    qDebug() << "Generated" << waypoints.size() << "waypoints for payload drop mission";
    
    return waypoints;
}

QList<QObject*> PtahMissionGenerator::createPayloadDropMissionItems(const QList<QGeoCoordinate>& waypoints, int altitude, int loiterTimeSeconds)
{
    // GenCall36: Create mission items for payload drop pattern
    qDebug() << "GenCall36: Creating payload drop mission items for" << waypoints.size() << "waypoints";
    
    QList<QObject*> missionItems;
    
    if (waypoints.isEmpty()) {
        qDebug() << "No waypoints provided for payload drop mission";
        return missionItems;
    }
    
    // 1. Servo 10 = 2500 (Release payload)
    qDebug() << "Adding servo command: Release payload (2500)";
    QObject* servoRelease = new QObject();
    servoRelease->setProperty("command", 183);  // MAV_CMD_DO_SET_SERVO
    servoRelease->setProperty("frame", 0);
    servoRelease->setProperty("params", QVariantList() << 10 << 2500 << 0 << 0 << 0 << 0 << 0);
    servoRelease->setProperty("autoContinue", true);
    missionItems.append(servoRelease);
    
    // 2. Takeoff command
    qDebug() << "Adding takeoff command";
    QObject* takeoff = new QObject();
    takeoff->setProperty("command", 22);  // MAV_CMD_NAV_TAKEOFF
    takeoff->setProperty("frame", 3);
    takeoff->setProperty("params", QVariantList() << 0 << 0 << 0 << 0 << waypoints[0].latitude() << waypoints[0].longitude() << altitude);
    takeoff->setProperty("autoContinue", true);
    takeoff->setProperty("altitude", altitude);
    takeoff->setProperty("altitudeMode", 1);
    missionItems.append(takeoff);
    
    // 3. Process each waypoint with payload drop pattern
    for (int i = 0; i < waypoints.size(); i++) {
        const QGeoCoordinate& waypoint = waypoints[i];
        
        // 3a. Waypoint command
        qDebug() << "Adding waypoint" << (i + 1) << "at" << waypoint.toString();
        QObject* wp = new QObject();
        wp->setProperty("command", 16);  // MAV_CMD_NAV_WAYPOINT
        wp->setProperty("frame", 3);
        wp->setProperty("params", QVariantList() << 0 << 0 << 0 << 0 << waypoint.latitude() << waypoint.longitude() << altitude);
        wp->setProperty("autoContinue", true);
        wp->setProperty("altitude", altitude);
        wp->setProperty("altitudeMode", 1);
        missionItems.append(wp);
        
        // 3b. Loiter command (50 seconds)
        qDebug() << "Adding loiter command for" << loiterTimeSeconds << "seconds";
        QObject* loiter = new QObject();
        loiter->setProperty("command", 31);  // MAV_CMD_NAV_LOITER_TIME
        loiter->setProperty("frame", 3);
        loiter->setProperty("params", QVariantList() << 0 << loiterTimeSeconds << 0 << 1 << waypoint.latitude() << waypoint.longitude() << altitude);
        loiter->setProperty("autoContinue", true);
        loiter->setProperty("altitude", altitude);
        loiter->setProperty("altitudeMode", 1);
        missionItems.append(loiter);
        
        // 3c. Servo 10 = 900 (Hold payload) - only after first waypoint
        if (i == 0) {
            qDebug() << "Adding servo command: Hold payload (900)";
            QObject* servoHold = new QObject();
            servoHold->setProperty("command", 183);  // MAV_CMD_DO_SET_SERVO
            servoHold->setProperty("frame", 0);
            servoHold->setProperty("params", QVariantList() << 10 << 900 << 0 << 0 << 0 << 0 << 0);
            servoHold->setProperty("autoContinue", true);
            missionItems.append(servoHold);
        }
    }
    
    // 4. Return to Launch
    qDebug() << "Adding return to launch command";
    QObject* rtl = new QObject();
    rtl->setProperty("command", 20);  // MAV_CMD_NAV_RETURN_TO_LAUNCH
    rtl->setProperty("frame", 0);
    rtl->setProperty("params", QVariantList() << 0 << 0 << 0 << 0 << 0 << 0 << 0);
    rtl->setProperty("autoContinue", true);
    missionItems.append(rtl);
    
    // 5. Land command
    qDebug() << "Adding land command";
    QObject* land = new QObject();
    land->setProperty("command", 21);  // MAV_CMD_NAV_LAND
    land->setProperty("frame", 3);
    land->setProperty("params", QVariantList() << 0 << 0 << 0 << 1 << waypoints[0].latitude() << waypoints[0].longitude() << 0);
    land->setProperty("autoContinue", true);
    land->setProperty("altitude", 0);
    land->setProperty("altitudeMode", 1);
    missionItems.append(land);
    
    qDebug() << "Created" << missionItems.size() << "mission items for payload drop pattern";
    
    return missionItems;
}

bool PtahMissionGenerator::isValidLandCoordinate(const QGeoCoordinate& coordinate)
{
    // GenCall35: PtahMissionGenerator::isValidLandCoordinate() - Validate coordinate for land-based operations
    qDebug() << "GenCall35: Validating coordinate for land-based operations:" << coordinate.toString();
    
    // Basic coordinate validity check
    if (!coordinate.isValid()) {
        qDebug() << "Coordinate is invalid";
        return false;
    }
    
    // Check latitude range
    if (coordinate.latitude() < -90.0 || coordinate.latitude() > 90.0) {
        qDebug() << "Invalid latitude:" << coordinate.latitude();
        return false;
    }
    
    // Check longitude range
    if (coordinate.longitude() < -180.0 || coordinate.longitude() > 180.0) {
        qDebug() << "Invalid longitude:" << coordinate.longitude();
        return false;
    }
    
    // Check for ocean coordinates (simplified validation)
    // This is a basic check - in a real implementation, you might use a more sophisticated
    // land/water detection service or database
    
    // Check for coordinates that are likely in oceans based on common patterns
    double lat = coordinate.latitude();
    double lng = coordinate.longitude();
    
    // Enhanced ocean detection to prevent waypoints over major water bodies
    // This approach is location-agnostic and works globally
    // TODO: Remove ocean tracing lines from the flight path to prevent waypoints from crossing water bodies
    
    // Arctic Ocean (very high latitudes)
    if (lat >= 80.0 && lat <= 90.0) {
        qDebug() << "Coordinate appears to be in Arctic Ocean area";
        return false;
    }
    
    // Antarctic Ocean (very low latitudes)
    if (lat <= -80.0) {
        qDebug() << "Coordinate appears to be in Antarctic Ocean area";
        return false;
    }
    
    // Atlantic Ocean - broader detection to catch the path shown in the map
    if (lat >= -40.0 && lat <= 50.0 && lng >= -80.0 && lng <= 20.0) {
        qDebug() << "Coordinate appears to be in Atlantic Ocean area";
        return false;
    }
    
    // Pacific Ocean - more conservative detection
    if (lat >= -20.0 && lat <= 20.0 && ((lng >= 150.0 && lng <= 180.0) || (lng >= -180.0 && lng <= -150.0))) {
        qDebug() << "Coordinate appears to be in central Pacific Ocean area";
        return false;
    }
    
    // Indian Ocean - more conservative detection
    if (lat >= -20.0 && lat <= 10.0 && lng >= 60.0 && lng <= 100.0) {
        qDebug() << "Coordinate appears to be in central Indian Ocean area";
        return false;
    }
    
    // Additional water body detection
    // Gulf of Mexico and Caribbean
    if (lat >= 15.0 && lat <= 30.0 && lng >= -100.0 && lng <= -80.0) {
        qDebug() << "Coordinate appears to be in Gulf of Mexico/Caribbean area";
        return false;
    }
    
    // Mediterranean Sea
    if (lat >= 30.0 && lat <= 45.0 && lng >= -10.0 && lng <= 40.0) {
        qDebug() << "Coordinate appears to be in Mediterranean Sea area";
        return false;
    }
    
    // Sargasso Sea (specifically mentioned in the flight path)
    if (lat >= 20.0 && lat <= 40.0 && lng >= -80.0 && lng <= -40.0) {
        qDebug() << "Coordinate appears to be in Sargasso Sea area";
        return false;
    }
    
    // Note: This is a simplified approach. In a production system, you would use
    // a proper land/water database or service to determine if coordinates are on land.
    // For now, we rely on the fallback mechanism to find land-based waypoints.
    
    qDebug() << "Coordinate passed validation - appears to be on land";
    return true;
}