import QtQuick 2.15
import QtTest 1.3

TestCase {
    name: "AreaPlanValidation"

    function hasQgc() {
        try { return typeof QGroundControl !== "undefined" && QGroundControl && QGroundControl.areaPlanEditor } catch (e) { return false }
    }

    function ape() { return QGroundControl.areaPlanEditor }

    function test_validation_numeric_bounds() {
        if (!hasQgc()) skip("No QGC context")
        // areaWidth/Height/lineSpacing > 0
        compare(ape().validateInput("areaWidth", 0).length > 0, true)
        compare(ape().validateInput("areaHeight", -5).length > 0, true)
        compare(ape().validateInput("lineSpacing", 0).length > 0, true)
        compare(ape().validateInput("areaWidth", 10).length, 0)
        compare(ape().validateInput("areaHeight", 10).length, 0)
        compare(ape().validateInput("lineSpacing", 1).length, 0)
    }

    function test_validation_numPoints_and_droneCount() {
        if (!hasQgc()) skip("No QGC context")
        compare(ape().validateInput("numPoints", 0).length > 0, true)
        compare(ape().validateInput("droneCount", 0).length > 0, true)
        compare(ape().validateInput("numPoints", 3).length, 0)
        compare(ape().validateInput("droneCount", 2).length, 0)
    }

    function test_validation_altitude_and_step() {
        if (!hasQgc()) skip("No QGC context")
        compare(ape().validateInput("altitudeBandStep", 0).length > 0, true)
        compare(ape().validateInput("altitudeBandStep", 10).length, 0)
        // missionAltitude: numeric required (>= any)
        compare(ape().validateInput("missionAltitude", 25).length, 0)
    }
}

