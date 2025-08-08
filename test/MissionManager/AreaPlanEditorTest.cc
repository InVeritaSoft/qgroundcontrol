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
#include <QtMath>
#include "MissionManager/AreaPartition.h"

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

void AreaPlanEditorTest::_balancedPartition()
{
    using AreaPlan::assignStripesRoundRobin;

    auto assertFair = [](int drones, int lines) {
        auto rr = assignStripesRoundRobin(drones, lines);
        int minC = INT_MAX, maxC = INT_MIN, sum = 0;
        for (const auto& g : rr) {
            int c = static_cast<int>(g.size());
            minC = qMin(minC, c);
            maxC = qMax(maxC, c);
            sum += c;
        }
        QVERIFY(maxC - minC <= 1);
        QCOMPARE(sum, lines);
    };

    // Boundary cases
    assertFair(1, 1);
    assertFair(1, 5);
    assertFair(5, 1); // more drones than lines → some groups 0

    // Typical cases
    assertFair(2, 3);
    assertFair(3, 10);
    assertFair(4, 17);
}

void AreaPlanEditorTest::_boundsAndRotation()
{
    using AreaPlan::splitIntoStripes;

    const double cx = 0.0, cy = 0.0;
    const double w = 20.0, h = 10.0;
    const int stripes = 5;

    auto lines0 = splitIntoStripes(cx, cy, w, h, stripes, /*alongShortAxis=*/true, /*rot=*/0.0);
    QVERIFY(static_cast<int>(lines0.size()) == stripes);
    // At rotation 0, endpoints must be within rectangle bounds
    for (const auto& ln : lines0) {
        auto check = [&](double x, double y) {
            QVERIFY(x >= -w/2 - 1e-6 && x <= w/2 + 1e-6);
            QVERIFY(y >= -h/2 - 1e-6 && y <= h/2 + 1e-6);
        };
        check(ln.a.x, ln.a.y);
        check(ln.b.x, ln.b.y);
    }

    auto linesR = splitIntoStripes(cx, cy, w, h, stripes, /*alongShortAxis=*/true, /*rot=*/30.0);
    QVERIFY(static_cast<int>(linesR.size()) == stripes);
    // Rotation should change endpoints for at least one line
    bool anyDiff = false;
    for (int i = 0; i < stripes; ++i) {
        if (qAbs(linesR[static_cast<size_t>(i)].a.x - lines0[static_cast<size_t>(i)].a.x) > 1e-9 ||
            qAbs(linesR[static_cast<size_t>(i)].a.y - lines0[static_cast<size_t>(i)].a.y) > 1e-9) {
            anyDiff = true;
            break;
        }
    }
    QVERIFY(anyDiff);

    // Edge cases
    auto empty1 = splitIntoStripes(cx, cy, 0.0, h, stripes, true, 0.0);
    QVERIFY(empty1.empty());
    auto empty2 = splitIntoStripes(cx, cy, w, -1.0, stripes, true, 0.0);
    QVERIFY(empty2.empty());
    auto empty3 = splitIntoStripes(cx, cy, w, h, 0, true, 0.0);
    QVERIFY(empty3.empty());
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

void AreaPlanEditorTest::_perDronePreviewCounts()
{
    AreaPlanEditor* editor = QGroundControlQmlGlobal::instance()->areaPlanEditor();
    QVERIFY(editor);

    // Configure a simple scenario
    editor->setAreaCenter(QGeoCoordinate(47.3977419, 8.5455938));
    editor->setAreaWidth(20.0);
    editor->setAreaHeight(30.0);
    editor->setLineSpacing(10.0); // 3 lines
    editor->setNumPoints(2);
    editor->setMissionAltitude(25.0);
    editor->setDroneCount(2);

    // Preview
    const QVariantList groups = editor->computePerDroneWaypointPreview();
    QVERIFY(groups.size() == 2);

    // Each group has waypoints; total waypoints == lines*numPoints
    int totalWp = 0;
    for (const QVariant& v : groups) {
        const QVariantMap m = v.toMap();
        const QVariantList wps = m.value("waypoints").toList();
        totalWp += wps.size();
        for (const QVariant& wv : wps) {
            QGeoCoordinate c = wv.value<QGeoCoordinate>();
            QVERIFY(c.isValid());
        }
    }
    const int expectedLines = qMax(1, static_cast<int>(qFloor(editor->areaHeight() / editor->lineSpacing())));
    QCOMPARE(totalWp, expectedLines * editor->numPoints());
}


