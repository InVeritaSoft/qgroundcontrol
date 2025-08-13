import QtQuick 2.15
import QtTest 1.3

TestCase {
    name: "AreaPlanPerformance"

    readonly property int iterations: 5

    function hasQgc() {
        try { return typeof QGroundControl !== "undefined" && QGroundControl && QGroundControl.areaPlanEditor } catch (e) { return false }
    }

    function ape() { return QGroundControl.areaPlanEditor }

    function setupSimpleArea() {
        ape().setAreaCenter(Qt.positioning.coordinate(37.4275, -122.1697))
        ape().setAreaWidth(100)
        ape().setAreaHeight(120)
        ape().setLineSpacing(10)
        ape().setNumPoints(6)
        ape().setAreaRotation(17)
    }

    function test_waypoint_generation_benchmark() {
        if (!hasQgc()) skip("No QGC context")
        setupSimpleArea()
        var times = []
        for (var i = 0; i < iterations; i++) {
            var t0 = Date.now()
            var wps = ape().generateWaypoints()
            var dt = Date.now() - t0
            times.push(dt)
            verify(wps.length > 0, "generateWaypoints returned empty")
        }
        // Log summary
        var sum = 0
        for (var j = 0; j < times.length; j++) sum += times[j]
        var avg = sum / times.length
        console.log("AreaPlanPerformance: avg generateWaypoints(ms)=", avg, "runs=", times.join(","))
        compare(avg >= 0, true)
    }
}

