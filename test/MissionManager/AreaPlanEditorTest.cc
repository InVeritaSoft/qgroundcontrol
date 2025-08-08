/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AreaPlanEditorTest.h"
#include "test/qgcunittest/UnitTest.h"
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

static double dist2(double ax, double ay, double bx, double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx*dx + dy*dy;
}

void AreaPlanEditorTest::_rotationHandling()
{
    using AreaPlan::splitIntoStripes;

    const double cx = 0.0, cy = 0.0;
    const double w = 20.0, h = 30.0;
    const int stripes = 4;

    auto atRot = [&](double deg) { return splitIntoStripes(cx, cy, w, h, stripes, true, deg); };

    auto a0   = atRot(0.0);
    auto a360 = atRot(360.0);
    QVERIFY(static_cast<int>(a0.size()) == stripes);
    QVERIFY(static_cast<int>(a360.size()) == stripes);
    for (int i = 0; i < stripes; ++i) {
        QVERIFY(qAbs(a0[static_cast<size_t>(i)].a.x - a360[static_cast<size_t>(i)].a.x) < 1e-9);
        QVERIFY(qAbs(a0[static_cast<size_t>(i)].a.y - a360[static_cast<size_t>(i)].a.y) < 1e-9);
        QVERIFY(qAbs(a0[static_cast<size_t>(i)].b.x - a360[static_cast<size_t>(i)].b.x) < 1e-9);
        QVERIFY(qAbs(a0[static_cast<size_t>(i)].b.y - a360[static_cast<size_t>(i)].b.y) < 1e-9);
    }

    auto a30   = atRot(30.0);
    auto a390  = atRot(390.0);
    for (int i = 0; i < stripes; ++i) {
        QVERIFY(qAbs(a30[static_cast<size_t>(i)].a.x - a390[static_cast<size_t>(i)].a.x) < 1e-9);
        QVERIFY(qAbs(a30[static_cast<size_t>(i)].a.y - a390[static_cast<size_t>(i)].a.y) < 1e-9);
        QVERIFY(qAbs(a30[static_cast<size_t>(i)].b.x - a390[static_cast<size_t>(i)].b.x) < 1e-9);
        QVERIFY(qAbs(a30[static_cast<size_t>(i)].b.y - a390[static_cast<size_t>(i)].b.y) < 1e-9);
    }

    // Segment length invariance across rotations
    auto length2 = [&](const AreaPlan::Line& ln) { return dist2(ln.a.x, ln.a.y, ln.b.x, ln.b.y); };
    auto a60  = atRot(60.0);
    auto a120 = atRot(120.0);
    for (int i = 0; i < stripes; ++i) {
        QVERIFY(qAbs(length2(a60[static_cast<size_t>(i)]) - length2(a120[static_cast<size_t>(i)])) < 1e-6);
    }
}

// Verify stripe orientation vs alongShortAxis flag and minimal area behavior
static bool approxParallelX(const AreaPlan::Line& ln) {
    // Parallel to Y axis means x nearly constant; we check |dx| << |dy|
    const double dx = qAbs(ln.a.x - ln.b.x);
    const double dy = qAbs(ln.a.y - ln.b.y);
    return dx < dy; // predominantly vertical
}
static bool approxParallelY(const AreaPlan::Line& ln) {
    const double dx = qAbs(ln.a.x - ln.b.x);
    const double dy = qAbs(ln.a.y - ln.b.y);
    return dy < dx; // predominantly horizontal
}

void AreaPlanEditorTest::_axisSelectionAndMinimal()
{
    using AreaPlan::splitIntoStripes;

    const double cx = 0.0, cy = 0.0;
    {
        // Width < Height → alongShortAxis=true should yield vertical stripes (x constant)
        auto v = splitIntoStripes(cx, cy, /*w=*/10.0, /*h=*/30.0, /*N=*/4, /*alongShortAxis=*/true, /*rot=*/0.0);
        QVERIFY(static_cast<int>(v.size()) == 4);
        for (const auto& ln : v) QVERIFY(approxParallelX(ln));

        // alongShortAxis=false on same dims → horizontal stripes
        auto h = splitIntoStripes(cx, cy, 10.0, 30.0, 4, /*alongShortAxis=*/false, 0.0);
        QVERIFY(static_cast<int>(h.size()) == 4);
        for (const auto& ln : h) QVERIFY(approxParallelY(ln));
    }

    {
        // Width > Height → alongShortAxis=true should yield horizontal stripes
        auto h = splitIntoStripes(cx, cy, /*w=*/40.0, /*h=*/10.0, /*N=*/3, /*alongShortAxis=*/true, /*rot=*/0.0);
        QVERIFY(static_cast<int>(h.size()) == 3);
        for (const auto& ln : h) QVERIFY(approxParallelY(ln));
    }

    {
        // Minimal area handling
        auto emptyW = splitIntoStripes(cx, cy, 0.0, 5.0, 3, true, 0.0);
        QVERIFY(emptyW.empty());
        auto emptyH = splitIntoStripes(cx, cy, 5.0, 0.0, 3, true, 0.0);
        QVERIFY(emptyH.empty());
        auto emptyN = splitIntoStripes(cx, cy, 5.0, 5.0, 0, true, 0.0);
        QVERIFY(emptyN.empty());

        // Very small but non-zero area still produces stripes within bounds
        const double w = 0.01, h = 0.02;
        auto tiny = splitIntoStripes(cx, cy, w, h, 2, true, 0.0);
        QVERIFY(static_cast<int>(tiny.size()) == 2);
        for (const auto& ln : tiny) {
            auto check = [&](double x, double y) {
                QVERIFY(x >= -w/2 - 1e-9 && x <= w/2 + 1e-9);
                QVERIFY(y >= -h/2 - 1e-9 && y <= h/2 + 1e-9);
            };
            check(ln.a.x, ln.a.y);
            check(ln.b.x, ln.b.y);
        }
    }
}
void AreaPlanEditorTest::_perDroneGeneratedWaypoints()
{
    AreaPlanEditor* editor = QGroundControlQmlGlobal::instance()->areaPlanEditor();
    QVERIFY(editor);

    editor->setAreaCenter(QGeoCoordinate(47.3977419, 8.5455938));
    editor->setAreaWidth(20.0);
    editor->setAreaHeight(30.0);
    editor->setLineSpacing(10.0); // 3 lines
    editor->setNumPoints(2);
    editor->setMissionAltitude(40.0);
    editor->setDroneCount(3);
    editor->setAltitudeBandStart(5.0);
    editor->setAltitudeBandStep(10.0);

    const int lineCount = qMax(1, static_cast<int>(qFloor(editor->areaHeight() / editor->lineSpacing())));
    for (int d = 0; d < editor->droneCount(); ++d) {
        auto wps = editor->generatePerDroneWaypoints(d);
        // Each assigned line contributes numPoints; round-robin groups differ by at most 1
        // We assert total across drones equals lineCount*numPoints across union; here check non-negative and plausible upper bound
        QVERIFY(wps.size() >= 0);
        QVERIFY(wps.size() <= lineCount * editor->numPoints());

        // Altitude check
        const double expectedAlt = editor->missionAltitude() + (editor->altitudeBandStart() + d * editor->altitudeBandStep());
        for (const QVariant& v : wps) {
            QGeoCoordinate c = v.value<QGeoCoordinate>();
            QVERIFY(c.isValid());
            QVERIFY(qAbs(c.altitude() - expectedAlt) < 1e-6);
        }
    }
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


