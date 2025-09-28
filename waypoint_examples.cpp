/****************************************************************************
 *
 * QGroundControl Waypoint Creation Examples
 * 
 * This file demonstrates various ways to create waypoints in QGroundControl
 * using the MissionController and related classes.
 *
 ****************************************************************************/

#include <QtCore/QObject>
#include <QtCore/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MissionController.h"
#include "PlanMasterController.h"
#include "SimpleMissionItem.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"

/**
 * Example 1: Basic Waypoint Creation
 * 
 * This shows how to create a simple waypoint at a specific coordinate
 */
class BasicWaypointExample : public QObject
{
    Q_OBJECT

public:
    BasicWaypointExample(PlanMasterController* planMasterController, QObject* parent = nullptr)
        : QObject(parent)
        , _planMasterController(planMasterController)
        , _missionController(planMasterController->missionController())
    {
    }

    // Method 1: Using MissionController's insertSimpleMissionItem
    void createBasicWaypoint(double latitude, double longitude, double altitude)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        // Insert waypoint at the end of the mission
        int nextIndex = _missionController->visualItems()->count();
        _missionController->insertSimpleMissionItem(coordinate, nextIndex, true /* makeCurrentItem */);
    }

    // Method 2: Creating waypoint at specific position
    void insertWaypointAtPosition(double latitude, double longitude, double altitude, int position)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        // Insert waypoint at specific position (0-based index)
        _missionController->insertSimpleMissionItem(coordinate, position, true /* makeCurrentItem */);
    }

    // Method 3: Creating waypoint after current item
    void insertWaypointAfterCurrent(double latitude, double longitude, double altitude)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int currentIndex = _missionController->currentPlanViewVIIndex();
        int nextIndex = currentIndex + 1;
        _missionController->insertSimpleMissionItem(coordinate, nextIndex, true /* makeCurrentItem */);
    }

private:
    PlanMasterController* _planMasterController;
    MissionController* _missionController;
};

/**
 * Example 2: Advanced Waypoint Creation with Custom Parameters
 * 
 * This shows how to create waypoints with specific MAVLink commands and parameters
 */
class AdvancedWaypointExample : public QObject
{
    Q_OBJECT

public:
    AdvancedWaypointExample(PlanMasterController* planMasterController, QObject* parent = nullptr)
        : QObject(parent)
        , _planMasterController(planMasterController)
        , _missionController(planMasterController->missionController())
    {
    }

    // Create a waypoint with specific hold time
    void createWaypointWithHoldTime(double latitude, double longitude, double altitude, double holdTime)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        // Create the waypoint
        int nextIndex = _missionController->visualItems()->count();
        VisualMissionItem* waypoint = _missionController->insertSimpleMissionItem(coordinate, nextIndex, true);
        
        if (waypoint) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(waypoint);
            if (simpleItem) {
                // Set hold time in param1 (seconds)
                simpleItem->missionItem().setParam1(holdTime);
                
                // Set acceptance radius in param2 (meters)
                simpleItem->missionItem().setParam2(5.0); // 5 meter acceptance radius
            }
        }
    }

    // Create a waypoint with specific speed
    void createWaypointWithSpeed(double latitude, double longitude, double altitude, double speed)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int nextIndex = _missionController->visualItems()->count();
        VisualMissionItem* waypoint = _missionController->insertSimpleMissionItem(coordinate, nextIndex, true);
        
        if (waypoint) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(waypoint);
            if (simpleItem) {
                // Set speed in param1 (m/s)
                simpleItem->missionItem().setParam1(speed);
            }
        }
    }

    // Create a waypoint with specific heading
    void createWaypointWithHeading(double latitude, double longitude, double altitude, double heading)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int nextIndex = _missionController->visualItems()->count();
        VisualMissionItem* waypoint = _missionController->insertSimpleMissionItem(coordinate, nextIndex, true);
        
        if (waypoint) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(waypoint);
            if (simpleItem) {
                // Set heading in param4 (degrees)
                simpleItem->missionItem().setParam4(heading);
            }
        }
    }

    // Create a ROI (Region of Interest) waypoint
    void createROIWaypoint(double latitude, double longitude, double altitude)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int nextIndex = _missionController->visualItems()->count();
        _missionController->insertROIMissionItem(coordinate, nextIndex, true);
    }

    // Create a takeoff waypoint
    void createTakeoffWaypoint(double latitude, double longitude, double altitude)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int nextIndex = _missionController->visualItems()->count();
        _missionController->insertTakeoffItem(coordinate, nextIndex, true);
    }

    // Create a landing waypoint
    void createLandingWaypoint(double latitude, double longitude, double altitude)
    {
        QGeoCoordinate coordinate(latitude, longitude, altitude);
        
        int nextIndex = _missionController->visualItems()->count();
        _missionController->insertLandItem(coordinate, nextIndex, true);
    }

private:
    PlanMasterController* _planMasterController;
    MissionController* _missionController;
};

/**
 * Example 3: Batch Waypoint Creation
 * 
 * This shows how to create multiple waypoints at once
 */
class BatchWaypointExample : public QObject
{
    Q_OBJECT

public:
    BatchWaypointExample(PlanMasterController* planMasterController, QObject* parent = nullptr)
        : QObject(parent)
        , _planMasterController(planMasterController)
        , _missionController(planMasterController->missionController())
    {
    }

    // Create a grid pattern of waypoints
    void createGridPattern(double centerLat, double centerLon, double altitude, 
                          double spacing, int rows, int cols)
    {
        double startLat = centerLat - (rows - 1) * spacing / 2.0;
        double startLon = centerLon - (cols - 1) * spacing / 2.0;
        
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                double lat = startLat + row * spacing;
                double lon = startLon + col * spacing;
                
                QGeoCoordinate coordinate(lat, lon, altitude);
                int nextIndex = _missionController->visualItems()->count();
                _missionController->insertSimpleMissionItem(coordinate, nextIndex, false);
            }
        }
    }

    // Create a circular pattern of waypoints
    void createCircularPattern(double centerLat, double centerLon, double altitude, 
                              double radius, int numPoints)
    {
        for (int i = 0; i < numPoints; ++i) {
            double angle = 2.0 * M_PI * i / numPoints;
            double lat = centerLat + radius * cos(angle) / 111320.0; // Convert meters to degrees
            double lon = centerLon + radius * sin(angle) / (111320.0 * cos(centerLat * M_PI / 180.0));
            
            QGeoCoordinate coordinate(lat, lon, altitude);
            int nextIndex = _missionController->visualItems()->count();
            _missionController->insertSimpleMissionItem(coordinate, nextIndex, false);
        }
    }

    // Create waypoints from a list of coordinates
    void createWaypointsFromList(const QList<QGeoCoordinate>& coordinates)
    {
        for (const QGeoCoordinate& coord : coordinates) {
            int nextIndex = _missionController->visualItems()->count();
            _missionController->insertSimpleMissionItem(coord, nextIndex, false);
        }
    }

private:
    PlanMasterController* _planMasterController;
    MissionController* _missionController;
};

/**
 * Example 4: Mission Planning with Waypoints
 * 
 * This shows how to create a complete mission with waypoints
 */
class MissionPlanningExample : public QObject
{
    Q_OBJECT

public:
    MissionPlanningExample(PlanMasterController* planMasterController, QObject* parent = nullptr)
        : QObject(parent)
        , _planMasterController(planMasterController)
        , _missionController(planMasterController->missionController())
    {
    }

    // Create a complete survey mission
    void createSurveyMission(double startLat, double startLon, double altitude, 
                           double surveyWidth, double surveyLength, double lineSpacing)
    {
        // Clear existing mission
        _missionController->removeAll();
        
        // Add takeoff waypoint
        QGeoCoordinate takeoffCoord(startLat, startLon, altitude);
        _missionController->insertTakeoffItem(takeoffCoord, 0, true);
        
        // Create survey pattern
        int numLines = static_cast<int>(surveyLength / lineSpacing) + 1;
        double currentLat = startLat;
        double currentLon = startLon;
        
        for (int line = 0; line < numLines; ++line) {
            // Add waypoint at start of line
            QGeoCoordinate startCoord(currentLat, currentLon, altitude);
            int nextIndex = _missionController->visualItems()->count();
            _missionController->insertSimpleMissionItem(startCoord, nextIndex, false);
            
            // Add waypoint at end of line
            QGeoCoordinate endCoord(currentLat, currentLon + surveyWidth, altitude);
            nextIndex = _missionController->visualItems()->count();
            _missionController->insertSimpleMissionItem(endCoord, nextIndex, false);
            
            // Move to next line
            currentLat += lineSpacing / 111320.0; // Convert meters to degrees
        }
        
        // Add return to launch
        QGeoCoordinate rtlCoord(startLat, startLon, altitude);
        int nextIndex = _missionController->visualItems()->count();
        _missionController->insertLandItem(rtlCoord, nextIndex, false);
    }

    // Create a waypoint mission with specific commands
    void createWaypointMission(const QList<QPair<QGeoCoordinate, MAV_CMD>>& waypoints)
    {
        _missionController->removeAll();
        
        for (const auto& waypoint : waypoints) {
            int nextIndex = _missionController->visualItems()->count();
            VisualMissionItem* item = _missionController->insertSimpleMissionItem(waypoint.first, nextIndex, false);
            
            if (item) {
                SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(item);
                if (simpleItem) {
                    simpleItem->setCommand(waypoint.second);
                }
            }
        }
    }

private:
    PlanMasterController* _planMasterController;
    MissionController* _missionController;
};

/**
 * Example 5: Direct MissionItem Creation
 * 
 * This shows how to create MissionItem objects directly
 */
class DirectMissionItemExample : public QObject
{
    Q_OBJECT

public:
    DirectMissionItemExample(QObject* parent = nullptr) : QObject(parent) {}

    // Create a MissionItem directly
    MissionItem* createDirectMissionItem(int sequenceNumber, 
                                       double latitude, double longitude, double altitude,
                                       MAV_CMD command = MAV_CMD_NAV_WAYPOINT,
                                       MAV_FRAME frame = MAV_FRAME_GLOBAL_RELATIVE_ALT)
    {
        return new MissionItem(
            sequenceNumber,
            command,
            frame,
            0.0,  // param1
            0.0,  // param2
            0.0,  // param3
            0.0,  // param4
            latitude,   // param5 (lat)
            longitude,  // param6 (lon)
            altitude,   // param7 (alt)
            true,  // autoContinue
            false, // isCurrentItem
            this   // parent
        );
    }

    // Create a waypoint with custom parameters
    MissionItem* createCustomWaypoint(int sequenceNumber,
                                    double latitude, double longitude, double altitude,
                                    double holdTime, double acceptanceRadius, double heading)
    {
        MissionItem* item = new MissionItem(
            sequenceNumber,
            MAV_CMD_NAV_WAYPOINT,
            MAV_FRAME_GLOBAL_RELATIVE_ALT,
            holdTime,           // param1: hold time
            acceptanceRadius,   // param2: acceptance radius
            0.0,               // param3: pass radius
            heading,           // param4: heading
            latitude,          // param5: latitude
            longitude,         // param6: longitude
            altitude,          // param7: altitude
            true,              // autoContinue
            false,             // isCurrentItem
            this               // parent
        );
        
        return item;
    }
};

// Usage examples
void demonstrateWaypointCreation()
{
    // Create a PlanMasterController (this would typically be done by the application)
    PlanMasterController* planMasterController = new PlanMasterController();
    
    // Example 1: Basic waypoint creation
    BasicWaypointExample basicExample(planMasterController);
    basicExample.createBasicWaypoint(37.7749, -122.4194, 100.0); // San Francisco, 100m altitude
    basicExample.insertWaypointAtPosition(37.7849, -122.4094, 120.0, 0); // Insert at beginning
    basicExample.insertWaypointAfterCurrent(37.7649, -122.4294, 80.0); // Insert after current
    
    // Example 2: Advanced waypoint creation
    AdvancedWaypointExample advancedExample(planMasterController);
    advancedExample.createWaypointWithHoldTime(37.7749, -122.4194, 100.0, 10.0); // 10 second hold
    advancedExample.createWaypointWithSpeed(37.7849, -122.4094, 120.0, 15.0); // 15 m/s speed
    advancedExample.createWaypointWithHeading(37.7649, -122.4294, 80.0, 45.0); // 45 degree heading
    advancedExample.createROIWaypoint(37.7549, -122.4394, 90.0); // ROI waypoint
    advancedExample.createTakeoffWaypoint(37.7449, -122.4494, 50.0); // Takeoff waypoint
    advancedExample.createLandingWaypoint(37.7349, -122.4594, 0.0); // Landing waypoint
    
    // Example 3: Batch waypoint creation
    BatchWaypointExample batchExample(planMasterController);
    batchExample.createGridPattern(37.7749, -122.4194, 100.0, 100.0, 3, 3); // 3x3 grid, 100m spacing
    batchExample.createCircularPattern(37.7749, -122.4194, 100.0, 500.0, 8); // 500m radius, 8 points
    
    // Example 4: Mission planning
    MissionPlanningExample missionExample(planMasterController);
    missionExample.createSurveyMission(37.7749, -122.4194, 100.0, 1000.0, 2000.0, 100.0); // 1km x 2km survey
    
    // Example 5: Direct MissionItem creation
    DirectMissionItemExample directExample;
    MissionItem* waypoint1 = directExample.createDirectMissionItem(1, 37.7749, -122.4194, 100.0);
    MissionItem* waypoint2 = directExample.createCustomWaypoint(2, 37.7849, -122.4094, 120.0, 5.0, 10.0, 90.0);
    
    // Clean up
    delete waypoint1;
    delete waypoint2;
    delete planMasterController;
}

#include "waypoint_examples.moc"