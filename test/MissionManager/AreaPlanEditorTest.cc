/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AreaPlanEditorTest.h"
#include "QGroundControlQmlGlobal.h"
#include "AreaPlanEditor.h"
#include "PlanMasterController.h"
#include "MissionController.h"

#include <QtTest/QTest>

void AreaPlanEditorTest::_basicProperties()
{
    // Access singleton-exposed editor
    AreaPlanEditor* editor = QGroundControlQmlGlobal::instance()->areaPlanEditor();
    QVERIFY(editor);

    // Set basic parameters and verify
    editor->setAreaWidth(20.0);
    editor->setAreaHeight(30.0);
    editor->setLineSpacing(10.0);
    editor->setNumPoints(2);
    editor->setMissionAltitude(15.0);

    QCOMPARE(editor->areaWidth(), 20.0);
    QCOMPARE(editor->areaHeight(), 30.0);
    QCOMPARE(editor->lineSpacing(), 10.0);
    QCOMPARE(editor->numPoints(), 2);
    QCOMPARE(editor->missionAltitude(), 15.0);

    // Compute totals
    const int expectedLines = qMax(1, static_cast<int>(qFloor(editor->areaHeight() / editor->lineSpacing())));
    const int expectedWp = expectedLines * editor->numPoints();
    QCOMPARE(editor->calculateTotalWaypoints(), expectedWp);
}

void AreaPlanEditorTest::_generateWaypointsAndAddToMission()
{
    // Prepare master/mission controller for offline planning
    PlanMasterController master(nullptr);
    master.setFlyView(false);
    master.start();

    MissionController* mission = master.missionController();
    QVERIFY(mission);

    // Hook editor to controller so it can add items
    AreaPlanEditor* editor = QGroundControlQmlGlobal::instance()->areaPlanEditor();
    QVERIFY(editor);
    editor->setPlanMasterController(&master);

    // Provide inputs
    editor->setAreaCenter(QGeoCoordinate(47.3977419, 8.5455938));
    editor->setAreaWidth(20.0);
    editor->setAreaHeight(30.0);
    editor->setLineSpacing(10.0);
    editor->setNumPoints(2);
    editor->setMissionAltitude(20.0);
    editor->setLoiterTime(5.0);

    // Generate and apply to mission
    auto waypoints = editor->generateWaypoints();
    QVERIFY(!waypoints.isEmpty());

    editor->addWaypointsToMission();

    // Expect: MissionSettings + Takeoff + (waypoint+loiter)*N + back to home + land
    auto* visual = mission->visualItems();
    QVERIFY(visual);
    QVERIFY(visual->count() > 3);
}

void AreaPlanEditorTest::_multiDroneDefaultsAndSetters()
{
    AreaPlanEditor* editor = QGroundControlQmlGlobal::instance()->areaPlanEditor();
    QVERIFY(editor);

    // Defaults
    QVERIFY(editor->droneCount() >= 1);
    QCOMPARE(editor->altitudeBandStart(), 0.0);
    QVERIFY(editor->altitudeBandStep() > 0.0);
    QVERIFY(editor->timeOffsetPerDrone() >= 0.0);
    QCOMPARE(editor->rtlAfterEveryWaypoint(), false);
    QCOMPARE(editor->loiterAfterRtl(), false);

    // Setters with clamping
    editor->setDroneCount(0); // clamp to 1
    QCOMPARE(editor->droneCount(), 1);

    editor->setAltitudeBandStart(-5.0); // clamp to 0
    QCOMPARE(editor->altitudeBandStart(), 0.0);

    editor->setAltitudeBandStep(-1.0); // reset to default step when invalid
    QVERIFY(editor->altitudeBandStep() > 0.0);

    editor->setTimeOffsetPerDrone(-2.0); // clamp to 0
    QCOMPARE(editor->timeOffsetPerDrone(), 0.0);

    editor->setRtlAfterEveryWaypoint(true);
    QCOMPARE(editor->rtlAfterEveryWaypoint(), true);

    editor->setLoiterAfterRtl(true);
    QCOMPARE(editor->loiterAfterRtl(), true);
}


