import QtQuick 2.15
import QtTest 1.3

TestCase {
    name: "AreaPlanEdgeCases"

    function hasQgc() {
        try { return typeof QGroundControl !== "undefined" && QGroundControl && QGroundControl.areaPlanEditor } catch (e) { return false }
    }

    function ape() { return QGroundControl.areaPlanEditor }

    function test_zero_dimensions_return_empty() {
        if (!hasQgc()) skip("No QGC context")
        ape().setAreaWidth(0)
        ape().setAreaHeight(0)
        ape().setLineSpacing(10)
        ape().setNumPoints(3)
        var wps = ape().generateWaypoints()
        compare(wps.length, 0)
        var preview = ape().computePerDroneWaypointPreview()
        compare(preview.length >= 0, true) // Should not crash
    }

    function test_minimum_valid_dimensions() {
        if (!hasQgc()) skip("No QGC context")
        ape().setAreaCenter(Qt.positioning.coordinate(37.4275, -122.1697))
        ape().setAreaWidth(1)
        ape().setAreaHeight(1)
        ape().setLineSpacing(1)
        ape().setNumPoints(1)
        var wps = ape().generateWaypoints()
        verify(wps.length >= 1)
    }

    function test_rotation_wraparound() {
        if (!hasQgc()) skip("No QGC context")
        ape().setAreaCenter(Qt.positioning.coordinate(37.4275, -122.1697))
        ape().setAreaWidth(20)
        ape().setAreaHeight(20)
        ape().setLineSpacing(5)
        ape().setNumPoints(3)
        // Test multiple rotations
        var rotations = [0, 90, 180, 270, 360, 450]
        for (var i=0; i<rotations.length; i++) {
            ape().setAreaRotation(rotations[i])
            var wps = ape().generateWaypoints()
            verify(wps.length > 0)
        }
    }
}

