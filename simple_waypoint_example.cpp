/****************************************************************************
 *
 * Simple Waypoint Creation Example
 * 
 * This is a minimal example showing how to create waypoints in QGroundControl
 * without all the complexity of the full system.
 *
 ****************************************************************************/

#include <QtCore/QObject>
#include <QtCore/QGeoCoordinate>
#include <QtCore/QList>
#include <QtCore/QDebug>

#include "MissionItem.h"
#include "QGCMAVLink.h"

/**
 * Simple Waypoint Creator
 * 
 * This class demonstrates the basic concepts of creating waypoints
 * without requiring the full QGroundControl infrastructure.
 */
class SimpleWaypointCreator : public QObject
{
    Q_OBJECT

public:
    SimpleWaypointCreator(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * Create a basic waypoint
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees  
     * @param altitude Altitude in meters
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createBasicWaypoint(double latitude, double longitude, double altitude, int sequenceNumber)
    {
        qDebug() << "Creating basic waypoint at" << latitude << longitude << altitude;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_WAYPOINT,             // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: hold time
            0.0,                              // param2: acceptance radius
            0.0,                              // param3: pass radius
            0.0,                              // param4: heading
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a waypoint with hold time
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param holdTime Hold time in seconds
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createWaypointWithHoldTime(double latitude, double longitude, double altitude, 
                                          double holdTime, int sequenceNumber)
    {
        qDebug() << "Creating waypoint with hold time at" << latitude << longitude << altitude << "hold:" << holdTime;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_WAYPOINT,             // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            holdTime,                         // param1: hold time
            0.0,                              // param2: acceptance radius
            0.0,                              // param3: pass radius
            0.0,                              // param4: heading
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a waypoint with acceptance radius
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param acceptanceRadius Acceptance radius in meters
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createWaypointWithRadius(double latitude, double longitude, double altitude, 
                                        double acceptanceRadius, int sequenceNumber)
    {
        qDebug() << "Creating waypoint with radius at" << latitude << longitude << altitude << "radius:" << acceptanceRadius;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_WAYPOINT,             // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: hold time
            acceptanceRadius,                 // param2: acceptance radius
            0.0,                              // param3: pass radius
            0.0,                              // param4: heading
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a waypoint with heading
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param heading Heading in degrees (0-360)
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createWaypointWithHeading(double latitude, double longitude, double altitude, 
                                         double heading, int sequenceNumber)
    {
        qDebug() << "Creating waypoint with heading at" << latitude << longitude << altitude << "heading:" << heading;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_WAYPOINT,             // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: hold time
            0.0,                              // param2: acceptance radius
            0.0,                              // param3: pass radius
            heading,                          // param4: heading
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a takeoff waypoint
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createTakeoffWaypoint(double latitude, double longitude, double altitude, int sequenceNumber)
    {
        qDebug() << "Creating takeoff waypoint at" << latitude << longitude << altitude;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_TAKEOFF,              // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: pitch
            0.0,                              // param2: empty
            0.0,                              // param3: empty
            0.0,                              // param4: yaw
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a landing waypoint
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createLandingWaypoint(double latitude, double longitude, double altitude, int sequenceNumber)
    {
        qDebug() << "Creating landing waypoint at" << latitude << longitude << altitude;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_NAV_LAND,                 // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: abort altitude
            0.0,                              // param2: landing mode
            0.0,                              // param3: empty
            0.0,                              // param4: yaw
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a ROI (Region of Interest) waypoint
     * 
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param sequenceNumber Sequence number for the waypoint
     * @return Pointer to the created MissionItem
     */
    MissionItem* createROIWaypoint(double latitude, double longitude, double altitude, int sequenceNumber)
    {
        qDebug() << "Creating ROI waypoint at" << latitude << longitude << altitude;
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_DO_SET_ROI_LOCATION,      // command type
            MAV_FRAME_GLOBAL_RELATIVE_ALT,    // coordinate frame
            0.0,                              // param1: empty
            0.0,                              // param2: empty
            0.0,                              // param3: empty
            0.0,                              // param4: empty
            latitude,                         // param5: latitude
            longitude,                        // param6: longitude
            altitude,                         // param7: altitude
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a speed change command
     * 
     * @param speed Speed in m/s
     * @param sequenceNumber Sequence number for the command
     * @return Pointer to the created MissionItem
     */
    MissionItem* createSpeedChange(double speed, int sequenceNumber)
    {
        qDebug() << "Creating speed change command:" << speed << "m/s";
        
        return new MissionItem(
            sequenceNumber,                    // sequence number
            MAV_CMD_DO_CHANGE_SPEED,          // command type
            MAV_FRAME_MISSION,                // coordinate frame
            0.0,                              // param1: speed type (0=air speed, 1=ground speed)
            speed,                            // param2: speed value
            -1.0,                             // param3: throttle (-1=no change)
            0.0,                              // param4: absolute/relative (0=absolute)
            0.0,                              // param5: empty
            0.0,                              // param6: empty
            0.0,                              // param7: empty
            true,                             // auto continue
            false,                            // is current item
            this                              // parent
        );
    }

    /**
     * Create a mission with multiple waypoints
     * 
     * @param coordinates List of coordinates for waypoints
     * @param altitude Altitude for all waypoints
     * @return List of created MissionItems
     */
    QList<MissionItem*> createMission(const QList<QGeoCoordinate>& coordinates, double altitude)
    {
        QList<MissionItem*> mission;
        
        qDebug() << "Creating mission with" << coordinates.size() << "waypoints";
        
        for (int i = 0; i < coordinates.size(); ++i) {
            const QGeoCoordinate& coord = coordinates[i];
            MissionItem* waypoint = createBasicWaypoint(coord.latitude(), coord.longitude(), altitude, i + 1);
            mission.append(waypoint);
        }
        
        return mission;
    }

    /**
     * Print mission information
     * 
     * @param mission List of MissionItems
     */
    void printMission(const QList<MissionItem*>& mission)
    {
        qDebug() << "Mission contains" << mission.size() << "items:";
        
        for (int i = 0; i < mission.size(); ++i) {
            MissionItem* item = mission[i];
            qDebug() << "  Item" << (i + 1) << ":"
                     << "Command:" << item->command()
                     << "Lat:" << item->coordinate().latitude()
                     << "Lon:" << item->coordinate().longitude()
                     << "Alt:" << item->coordinate().altitude()
                     << "Param1:" << item->param1()
                     << "Param2:" << item->param2()
                     << "Param4:" << item->param4();
        }
    }

    /**
     * Clean up mission items
     * 
     * @param mission List of MissionItems to delete
     */
    void cleanupMission(QList<MissionItem*>& mission)
    {
        for (MissionItem* item : mission) {
            delete item;
        }
        mission.clear();
    }
};

// Example usage
int main()
{
    SimpleWaypointCreator creator;
    
    // Create a simple mission
    QList<QGeoCoordinate> coordinates = {
        QGeoCoordinate(37.7749, -122.4194, 100.0),  // San Francisco
        QGeoCoordinate(37.7849, -122.4094, 120.0),  // Slightly north
        QGeoCoordinate(37.7649, -122.4294, 80.0),   // Slightly south
        QGeoCoordinate(37.7749, -122.4194, 0.0)     // Return to start
    };
    
    QList<MissionItem*> mission = creator.createMission(coordinates, 100.0);
    creator.printMission(mission);
    
    // Create individual waypoints with different parameters
    MissionItem* takeoff = creator.createTakeoffWaypoint(37.7749, -122.4194, 50.0, 1);
    MissionItem* waypoint1 = creator.createWaypointWithHoldTime(37.7849, -122.4094, 100.0, 10.0, 2);
    MissionItem* waypoint2 = creator.createWaypointWithRadius(37.7649, -122.4294, 120.0, 15.0, 3);
    MissionItem* waypoint3 = creator.createWaypointWithHeading(37.7549, -122.4394, 80.0, 45.0, 4);
    MissionItem* roi = creator.createROIWaypoint(37.7449, -122.4494, 90.0, 5);
    MissionItem* landing = creator.createLandingWaypoint(37.7349, -122.4594, 0.0, 6);
    
    // Create a speed change command
    MissionItem* speedChange = creator.createSpeedChange(15.0, 7);
    
    // Print individual waypoints
    QList<MissionItem*> individualWaypoints = {takeoff, waypoint1, waypoint2, waypoint3, roi, landing, speedChange};
    creator.printMission(individualWaypoints);
    
    // Clean up
    creator.cleanupMission(mission);
    creator.cleanupMission(individualWaypoints);
    
    return 0;
}

#include "simple_waypoint_example.moc"