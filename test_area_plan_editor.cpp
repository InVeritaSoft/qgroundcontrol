#include <QTest>
#include <QSignalSpy>
#include <QGeoCoordinate>
#include "AreaPlanEditor.h"

class TestAreaPlanEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Setup test environment
    }

    void testInitialization()
    {
        AreaPlanEditor editor;
        
        // Test default values
        QCOMPARE(editor.areaWidth(), 30.0);
        QCOMPARE(editor.areaHeight(), 90.0);
        QCOMPARE(editor.lineSpacing(), 3.0);
        QCOMPARE(editor.numPoints(), 1);
        QCOMPARE(editor.missionAltitude(), 10.0);
        QCOMPARE(editor.isDrawingMode(), false);
        
        // Test that area center is invalid initially
        QVERIFY(!editor.areaCenter().isValid());
    }

    void testPropertySetters()
    {
        AreaPlanEditor editor;
        
        // Test area width setter
        editor.setAreaWidth(100.0);
        QCOMPARE(editor.areaWidth(), 100.0);
        
        // Test area height setter
        editor.setAreaHeight(200.0);
        QCOMPARE(editor.areaHeight(), 200.0);
        
        // Test line spacing setter
        editor.setLineSpacing(5.0);
        QCOMPARE(editor.lineSpacing(), 5.0);
        
        // Test num points setter
        editor.setNumPoints(5);
        QCOMPARE(editor.numPoints(), 5);
        
        // Test mission altitude setter
        editor.setMissionAltitude(50.0);
        QCOMPARE(editor.missionAltitude(), 50.0);
    }

    void testDrawingMode()
    {
        AreaPlanEditor editor;
        QSignalSpy spy(&editor, &AreaPlanEditor::isDrawingModeChanged);
        
        // Test initial state
        QCOMPARE(editor.isDrawingMode(), false);
        
        // Test enabling drawing mode
        editor.setIsDrawingMode(true);
        QCOMPARE(editor.isDrawingMode(), true);
        QCOMPARE(spy.count(), 1);
        
        // Test disabling drawing mode
        editor.setIsDrawingMode(false);
        QCOMPARE(editor.isDrawingMode(), false);
        QCOMPARE(spy.count(), 2);
        
        // Test setting same value (should not emit signal)
        editor.setIsDrawingMode(false);
        QCOMPARE(spy.count(), 2); // Should not change
    }

    void testAreaCenter()
    {
        AreaPlanEditor editor;
        QSignalSpy spy(&editor, &AreaPlanEditor::areaCenterChanged);
        
        QGeoCoordinate center(37.123456, -122.654321);
        
        // Test setting area center
        editor.setAreaCenter(center);
        QCOMPARE(editor.areaCenter().latitude(), center.latitude());
        QCOMPARE(editor.areaCenter().longitude(), center.longitude());
        QCOMPARE(spy.count(), 1);
        
        // Test setting same center (should not emit signal)
        editor.setAreaCenter(center);
        QCOMPARE(spy.count(), 1); // Should not change
    }

    void testGeodesicCalculations()
    {
        AreaPlanEditor editor;
        
        QGeoCoordinate start(37.0, -122.0);
        
        // Test north movement
        QGeoCoordinate north = editor.calculateOffsetCoordinate(start, 100.0, 0.0);
        QVERIFY(north.latitude() > start.latitude());
        QCOMPARE(north.longitude(), start.longitude());
        
        // Test south movement
        QGeoCoordinate south = editor.calculateOffsetCoordinate(start, 100.0, 180.0);
        QVERIFY(south.latitude() < start.latitude());
        QCOMPARE(south.longitude(), start.longitude());
        
        // Test east movement
        QGeoCoordinate east = editor.calculateOffsetCoordinate(start, 100.0, 90.0);
        QVERIFY(east.longitude() > start.longitude());
        QCOMPARE(east.latitude(), start.latitude());
        
        // Test west movement
        QGeoCoordinate west = editor.calculateOffsetCoordinate(start, 100.0, 270.0);
        QVERIFY(west.longitude() < start.longitude());
        QCOMPARE(west.latitude(), start.latitude());
    }

    void testAreaMovement()
    {
        AreaPlanEditor editor;
        QSignalSpy spy(&editor, &AreaPlanEditor::areaCenterChanged);
        
        QGeoCoordinate initialCenter(37.0, -122.0);
        editor.setAreaCenter(initialCenter);
        
        // Test north movement
        editor.moveAreaNorth();
        QVERIFY(editor.areaCenter().latitude() > initialCenter.latitude());
        QCOMPARE(editor.areaCenter().longitude(), initialCenter.longitude());
        
        // Test south movement
        editor.setAreaCenter(initialCenter);
        editor.moveAreaSouth();
        QVERIFY(editor.areaCenter().latitude() < initialCenter.latitude());
        QCOMPARE(editor.areaCenter().longitude(), initialCenter.longitude());
        
        // Test east movement
        editor.setAreaCenter(initialCenter);
        editor.moveAreaEast();
        QVERIFY(editor.areaCenter().longitude() > initialCenter.longitude());
        QCOMPARE(editor.areaCenter().latitude(), initialCenter.latitude());
        
        // Test west movement
        editor.setAreaCenter(initialCenter);
        editor.moveAreaWest();
        QVERIFY(editor.areaCenter().longitude() < initialCenter.longitude());
        QCOMPARE(editor.areaCenter().latitude(), initialCenter.latitude());
    }

    void testWaypointCalculations()
    {
        AreaPlanEditor editor;
        
        // Set up a simple area
        editor.setAreaWidth(100.0);
        editor.setAreaHeight(100.0);
        editor.setLineSpacing(20.0);
        editor.setNumPoints(3);
        editor.setAreaCenter(QGeoCoordinate(37.0, -122.0));
        
        // Test total waypoints calculation
        int totalWaypoints = editor.calculateTotalWaypoints();
        QCOMPARE(totalWaypoints, 15); // 5 lines * 3 points
        
        // Test flight time calculation
        int flightTime = editor.calculateFlightTime();
        QCOMPARE(flightTime, 30); // 15 waypoints * 2 minutes
    }

    void testWaypointGeneration()
    {
        AreaPlanEditor editor;
        
        // Set up a simple area
        editor.setAreaWidth(100.0);
        editor.setAreaHeight(100.0);
        editor.setLineSpacing(50.0);
        editor.setNumPoints(2);
        editor.setAreaCenter(QGeoCoordinate(37.0, -122.0));
        
        // Generate waypoints
        QVariantList waypoints = editor.generateWaypoints();
        
        // Should have 4 waypoints (2 lines * 2 points)
        QCOMPARE(waypoints.size(), 4);
        
        // Verify waypoint structure
        for (const QVariant& waypoint : waypoints) {
            QVERIFY(waypoint.canConvert<QVariantMap>());
            QVariantMap waypointMap = waypoint.toMap();
            QVERIFY(waypointMap.contains("coordinate"));
            QVERIFY(waypointMap.contains("altitude"));
        }
    }

    void testStatusUpdates()
    {
        AreaPlanEditor editor;
        QSignalSpy spy(&editor, &AreaPlanEditor::statusChanged);
        
        // Test status update
        editor.updateStatus("Test status message");
        QCOMPARE(spy.count(), 1);
        
        // Test status message content
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), "Test status message");
    }

    void testValidation()
    {
        AreaPlanEditor editor;
        
        // Test valid input
        QVERIFY(editor.isInputValid("areaWidth", 100.0));
        QVERIFY(editor.isInputValid("areaHeight", 200.0));
        QVERIFY(editor.isInputValid("lineSpacing", 10.0));
        QVERIFY(editor.isInputValid("numPoints", 5));
        
        // Test invalid input
        QVERIFY(!editor.isInputValid("areaWidth", -10.0));
        QVERIFY(!editor.isInputValid("areaHeight", 0.0));
        QVERIFY(!editor.isInputValid("lineSpacing", -5.0));
        QVERIFY(!editor.isInputValid("numPoints", 0));
    }

    void testPerformanceOptimizations()
    {
        AreaPlanEditor editor;
        
        // Test optimization enable/disable
        editor.enableOptimizations();
        QCOMPARE(editor.isOptimized(), true);
        
        editor.disableOptimizations();
        QCOMPARE(editor.isOptimized(), false);
        
        // Test cache size setting
        editor.setCacheSize(100);
        QCOMPARE(editor.cacheSize(), 100);
        
        // Test cache clearing
        editor.clearCache();
        QCOMPARE(editor.cacheSize(), 0);
    }

    void cleanupTestCase()
    {
        // Cleanup test environment
    }
};

QTEST_MAIN(TestAreaPlanEditor)
#include "test_area_plan_editor.moc" 