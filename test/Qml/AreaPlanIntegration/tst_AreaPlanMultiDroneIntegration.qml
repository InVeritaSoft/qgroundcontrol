import QtQuick 2.15
import QtTest 1.3

TestCase {
    name: "AreaPlanMultiDroneIntegration"

    function hasQgc() {
        try { return typeof QGroundControl !== "undefined" && QGroundControl && QGroundControl.areaPlanEditor } catch (e) { return false }
    }

    function ape() { return QGroundControl.areaPlanEditor }

    function setupMulti() {
        ape().setAreaCenter(Qt.positioning.coordinate(37.4275, -122.1697))
        ape().setAreaWidth(60)
        ape().setAreaHeight(40)
        ape().setLineSpacing(10)
        ape().setNumPoints(3)
        ape().setDroneCount(3)
        ape().setAltitudeBandStart(0)
        ape().setAltitudeBandStep(10)
        ape().setTimeOffsetPerDrone(5)
        ape().setPerTargetSeparationS(3)
        ape().setMissionAltitude(30)
    }

    function test_preview_groups_and_counts() {
        if (!hasQgc()) skip("No QGC context")
        setupMulti()
        var preview = ape().computePerDroneWaypointPreview()
        verify(preview.length === 3)
        // Each drone has same waypoint count (round-robin stripes distribution)
        var c0 = preview[0].waypoints.length
        for (var i=1;i<preview.length;i++) compare(preview[i].waypoints.length, c0)
    }

    function test_policy_land_at_target_return_toggles() {
        if (!hasQgc()) skip("No QGC context")
        setupMulti()
        ape().setLandAtTargetReturn(true)
        var before = ape().computePerDroneWaypointPreview()
        verify(before.length === 3)
        // Toggle
        ape().setLandAtTargetReturn(false)
        var after = ape().computePerDroneWaypointPreview()
        verify(after.length === 3)
    }
}

