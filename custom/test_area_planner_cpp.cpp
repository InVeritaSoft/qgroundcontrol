/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTest>
#include <QSignalSpy>
#include <QGeoCoordinate>
#include <QVariantList>

#include "MissionAreaPlanner.h"

class MissionAreaPlannerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Property tests
    void testWidthProperty();
    void testHeightProperty();
    void testLineSpacingProperty();
    void testNumPointsProperty();
    void testCenterLatProperty();
    void testCenterLonProperty();
    void testBusyProperty();
    void testStatusProperty();
    
    // Method tests
    void testGenerateMission();
    void testClearMission();
    void testCalculateWaypoints();
    void testUploadMission();
    
    // Integration tests
    void testFullWorkflow();
    void testInvalidInputs();
    void testCoordinateCalculations();

private:
    MissionAreaPlanner* _areaPlanner;
    QSignalSpy* _widthSpy;
    QSignalSpy* _heightSpy;
    QSignalSpy* _lineSpacingSpy;
    QSignalSpy* _numPointsSpy;
    QSignalSpy* _centerLatSpy;
    QSignalSpy* _centerLonSpy;
    QSignalSpy* _busySpy;
    QSignalSpy* _statusSpy;
    QSignalSpy* _waypointsChangedSpy;
};

void MissionAreaPlannerTest::initTestCase()
{
    _areaPlanner = new MissionAreaPlanner(this);
    
    // Create signal spies for all properties
    _widthSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::widthChanged);
    _heightSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::heightChanged);
    _lineSpacingSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::lineSpacingChanged);
    _numPointsSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::numPointsChanged);
    _centerLatSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::centerLatChanged);
    _centerLonSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::centerLonChanged);
    _busySpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::busyChanged);
    _statusSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::statusChanged);
    _waypointsChangedSpy = new QSignalSpy(_areaPlanner, &MissionAreaPlanner::waypointsChanged);
}

void MissionAreaPlannerTest::cleanupTestCase()
{
    delete _areaPlanner;
    delete _widthSpy;
    delete _heightSpy;
    delete _lineSpacingSpy;
    delete _numPointsSpy;
    delete _centerLatSpy;
    delete _centerLonSpy;
    delete _busySpy;
    delete _statusSpy;
    delete _waypointsChangedSpy;
}

void MissionAreaPlannerTest::testWidthProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->width(), 30.0);
    
    // Test setting width
    _areaPlanner->setWidth(50.0);
    QCOMPARE(_areaPlanner->width(), 50.0);
    QCOMPARE(_widthSpy->count(), 1);
    
    // Test setting same value (should not emit signal)
    _areaPlanner->setWidth(50.0);
    QCOMPARE(_widthSpy->count(), 1); // Should not increase
    
    // Test setting different value
    _areaPlanner->setWidth(100.0);
    QCOMPARE(_areaPlanner->width(), 100.0);
    QCOMPARE(_widthSpy->count(), 2);
}

void MissionAreaPlannerTest::testHeightProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->height(), 90.0);
    
    // Test setting height
    _areaPlanner->setHeight(120.0);
    QCOMPARE(_areaPlanner->height(), 120.0);
    QCOMPARE(_heightSpy->count(), 1);
    
    // Test setting same value (should not emit signal)
    _areaPlanner->setHeight(120.0);
    QCOMPARE(_heightSpy->count(), 1); // Should not increase
}

void MissionAreaPlannerTest::testLineSpacingProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->lineSpacing(), 3.0);
    
    // Test setting line spacing
    _areaPlanner->setLineSpacing(5.0);
    QCOMPARE(_areaPlanner->lineSpacing(), 5.0);
    QCOMPARE(_lineSpacingSpy->count(), 1);
}

void MissionAreaPlannerTest::testNumPointsProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->numPoints(), 1);
    
    // Test setting num points
    _areaPlanner->setNumPoints(3);
    QCOMPARE(_areaPlanner->numPoints(), 3);
    QCOMPARE(_numPointsSpy->count(), 1);
}

void MissionAreaPlannerTest::testCenterLatProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->centerLat(), 49.82824897481479);
    
    // Test setting center lat
    _areaPlanner->setCenterLat(50.0);
    QCOMPARE(_areaPlanner->centerLat(), 50.0);
    QCOMPARE(_centerLatSpy->count(), 1);
}

void MissionAreaPlannerTest::testCenterLonProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->centerLon(), 24.033390804256005);
    
    // Test setting center lon
    _areaPlanner->setCenterLon(25.0);
    QCOMPARE(_areaPlanner->centerLon(), 25.0);
    QCOMPARE(_centerLonSpy->count(), 1);
}

void MissionAreaPlannerTest::testBusyProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->busy(), false);
    
    // Test setting busy
    _areaPlanner->setBusy(true);
    QCOMPARE(_areaPlanner->busy(), true);
    QCOMPARE(_busySpy->count(), 1);
    
    _areaPlanner->setBusy(false);
    QCOMPARE(_areaPlanner->busy(), false);
    QCOMPARE(_busySpy->count(), 2);
}

void MissionAreaPlannerTest::testStatusProperty()
{
    // Test initial value
    QCOMPARE(_areaPlanner->status(), QString("Ready"));
    
    // Test setting status
    _areaPlanner->setStatus("Testing");
    QCOMPARE(_areaPlanner->status(), QString("Testing"));
    QCOMPARE(_statusSpy->count(), 1);
}

void MissionAreaPlannerTest::testGenerateMission()
{
    // Reset to known state
    _areaPlanner->setWidth(30.0);
    _areaPlanner->setHeight(90.0);
    _areaPlanner->setLineSpacing(3.0);
    _areaPlanner->setNumPoints(1);
    _areaPlanner->setCenterLat(49.82824897481479);
    _areaPlanner->setCenterLon(24.033390804256005);
    
    // Clear any existing waypoints
    _areaPlanner->clearMission();
    
    // Generate mission
    _areaPlanner->generateMission();
    
    // Check that waypoints were generated
    QVariantList waypoints = _areaPlanner->waypoints();
    QVERIFY(waypoints.size() > 0);
    
    // Check that busy state was managed correctly
    QCOMPARE(_areaPlanner->busy(), false);
    
    // Check status
    QVERIFY(_areaPlanner->status().contains("Generated"));
    
    // Check that signal was emitted
    QCOMPARE(_waypointsChangedSpy->count(), 1);
}

void MissionAreaPlannerTest::testClearMission()
{
    // First generate a mission
    _areaPlanner->generateMission();
    
    // Clear mission
    _areaPlanner->clearMission();
    
    // Check that waypoints are cleared
    QVariantList waypoints = _areaPlanner->waypoints();
    QCOMPARE(waypoints.size(), 0);
    
    // Check status
    QCOMPARE(_areaPlanner->status(), QString("Mission cleared"));
    
    // Check that signal was emitted
    QCOMPARE(_waypointsChangedSpy->count(), 2); // One for generate, one for clear
}

void MissionAreaPlannerTest::testCalculateWaypoints()
{
    // Set up test parameters
    _areaPlanner->setWidth(30.0);
    _areaPlanner->setHeight(90.0);
    _areaPlanner->setLineSpacing(3.0);
    _areaPlanner->setNumPoints(1);
    _areaPlanner->setCenterLat(49.82824897481479);
    _areaPlanner->setCenterLon(24.033390804256005);
    
    // Calculate waypoints
    QVariantList waypoints = _areaPlanner->calculateWaypoints();
    
    // Verify waypoints were calculated
    QVERIFY(waypoints.size() > 0);
    
    // Check first waypoint structure
    QVERIFY(waypoints[0].canConvert<QVariantMap>());
    QVariantMap firstWaypoint = waypoints[0].toMap();
    QVERIFY(firstWaypoint.contains("lat"));
    QVERIFY(firstWaypoint.contains("lon"));
    QVERIFY(firstWaypoint.contains("alt"));
    
    // Check coordinate values are reasonable
    double lat = firstWaypoint["lat"].toDouble();
    double lon = firstWaypoint["lon"].toDouble();
    QVERIFY(lat >= 49.0 && lat <= 51.0);
    QVERIFY(lon >= 23.0 && lon <= 25.0);
}

void MissionAreaPlannerTest::testUploadMission()
{
    // First generate a mission
    _areaPlanner->generateMission();
    
    // Upload mission
    _areaPlanner->uploadMission();
    
    // Check status
    QVERIFY(_areaPlanner->status().contains("Uploaded"));
    
    // Check that busy state was managed correctly
    QCOMPARE(_areaPlanner->busy(), false);
}

void MissionAreaPlannerTest::testFullWorkflow()
{
    // Test complete workflow: set parameters -> generate -> clear
    
    // 1. Set parameters
    _areaPlanner->setWidth(50.0);
    _areaPlanner->setHeight(100.0);
    _areaPlanner->setLineSpacing(5.0);
    _areaPlanner->setNumPoints(2);
    _areaPlanner->setCenterLat(50.0);
    _areaPlanner->setCenterLon(25.0);
    
    // Verify parameters were set
    QCOMPARE(_areaPlanner->width(), 50.0);
    QCOMPARE(_areaPlanner->height(), 100.0);
    QCOMPARE(_areaPlanner->lineSpacing(), 5.0);
    QCOMPARE(_areaPlanner->numPoints(), 2);
    QCOMPARE(_areaPlanner->centerLat(), 50.0);
    QCOMPARE(_areaPlanner->centerLon(), 25.0);
    
    // 2. Generate mission
    _areaPlanner->generateMission();
    
    // Verify mission was generated
    QVariantList waypoints = _areaPlanner->waypoints();
    QVERIFY(waypoints.size() > 0);
    QVERIFY(_areaPlanner->status().contains("Generated"));
    
    // 3. Clear mission
    _areaPlanner->clearMission();
    
    // Verify mission was cleared
    waypoints = _areaPlanner->waypoints();
    QCOMPARE(waypoints.size(), 0);
    QCOMPARE(_areaPlanner->status(), QString("Mission cleared"));
}

void MissionAreaPlannerTest::testInvalidInputs()
{
    // Test negative values
    _areaPlanner->setWidth(-10.0);
    QCOMPARE(_areaPlanner->width(), 0.0); // Should clamp to 0
    
    _areaPlanner->setHeight(-20.0);
    QCOMPARE(_areaPlanner->height(), 0.0); // Should clamp to 0
    
    _areaPlanner->setLineSpacing(-1.0);
    QCOMPARE(_areaPlanner->lineSpacing(), 0.1); // Should clamp to minimum
    
    _areaPlanner->setNumPoints(0);
    QCOMPARE(_areaPlanner->numPoints(), 1); // Should clamp to minimum
    
    // Test very large values
    _areaPlanner->setWidth(10000.0);
    QCOMPARE(_areaPlanner->width(), 10000.0); // Should accept large values
    
    _areaPlanner->setHeight(10000.0);
    QCOMPARE(_areaPlanner->height(), 10000.0); // Should accept large values
}

void MissionAreaPlannerTest::testCoordinateCalculations()
{
    // Test coordinate calculations with known values
    _areaPlanner->setCenterLat(50.0);
    _areaPlanner->setCenterLon(25.0);
    _areaPlanner->setWidth(100.0);
    _areaPlanner->setHeight(200.0);
    
    QVariantList waypoints = _areaPlanner->calculateWaypoints();
    
    // Verify waypoints are within expected bounds
    for (const QVariant& waypoint : waypoints) {
        QVariantMap wp = waypoint.toMap();
        double lat = wp["lat"].toDouble();
        double lon = wp["lon"].toDouble();
        
        // Waypoints should be within the defined area
        QVERIFY(lat >= 49.0 && lat <= 51.0); // Rough bounds for 100m width
        QVERIFY(lon >= 24.0 && lon <= 26.0); // Rough bounds for 200m height
    }
}

QTEST_MAIN(MissionAreaPlannerTest)
#include "test_area_planner_cpp.moc" 