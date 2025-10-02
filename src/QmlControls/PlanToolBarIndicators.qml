import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl

import QGroundControl.Controls
import QGroundControl.FactControls

import QGroundControl.UTMSP

// Toolbar for Plan View
Item {
    width: missionStats.width + _margins

    property var planMasterController

    property var _planMasterController: planMasterController
    property var _currentMissionItem: _planMasterController.missionController.currentPlanViewItem ///< Mission item to display status for

    property var missionItems: _controllerValid ? _planMasterController.missionController.visualItems : undefined
    property real missionPlannedDistance: _controllerValid ? _planMasterController.missionController.missionPlannedDistance : NaN
    property real missionTime: _controllerValid ? _planMasterController.missionController.missionTime : 0
    property real missionMaxTelemetry: _controllerValid ? _planMasterController.missionController.missionMaxTelemetry : NaN
    property bool missionDirty: _controllerValid ? _planMasterController.missionController.dirty : false

    property bool _controllerValid: _planMasterController !== undefined && _planMasterController !== null
    property bool _controllerOffline: _controllerValid ? _planMasterController.offline : true
    property var _controllerDirty: _controllerValid ? _planMasterController.dirty : false
    property var _controllerSyncInProgress: _controllerValid ? _planMasterController.syncInProgress : false

    property bool _currentMissionItemValid: _currentMissionItem && _currentMissionItem !== undefined && _currentMissionItem !== null
    property bool _curreItemIsFlyThrough: _currentMissionItemValid && _currentMissionItem.specifiesCoordinate && !_currentMissionItem.isStandaloneCoordinate
    property bool _currentItemIsVTOLTakeoff: _currentMissionItemValid && _currentMissionItem.command == 84
    property bool _missionValid: missionItems !== undefined

    property real _dataFontSize: ScreenTools.defaultFontPointSize
    property real _largeValueWidth: ScreenTools.defaultFontPixelWidth * 8
    property real _mediumValueWidth: ScreenTools.defaultFontPixelWidth * 4
    property real _smallValueWidth: ScreenTools.defaultFontPixelWidth * 3
    property real _labelToValueSpacing: ScreenTools.defaultFontPixelWidth
    property real _rowSpacing: ScreenTools.isMobile ? 1 : 0
    property real _distance: _currentMissionItemValid ? _currentMissionItem.distance : NaN
    property real _altDifference: _currentMissionItemValid ? _currentMissionItem.altDifference : NaN
    property real _azimuth: _currentMissionItemValid ? _currentMissionItem.azimuth : NaN
    property real _heading: _currentMissionItemValid ? _currentMissionItem.missionVehicleYaw : NaN
    property real _missionPlannedDistance: _missionValid ? missionPlannedDistance : NaN
    property real _missionMaxTelemetry: _missionValid ? missionMaxTelemetry : NaN
    property real _missionTime: _missionValid ? missionTime : 0

    // Servo 10 PWM Toggler properties
    property int _servo10PWM: 400  // Current PWM value (MIN: 400, MAX: 2400)
    property bool _servo10Enabled: false  // Current state
    property int _batteryChangePoint: _controllerValid ? _planMasterController.missionController.batteryChangePoint : -1
    property int _batteriesRequired: _controllerValid ? _planMasterController.missionController.batteriesRequired : -1
    property bool _batteryInfoAvailable: _batteryChangePoint >= 0 || _batteriesRequired >= 0
    property real _gradient: _currentMissionItemValid && _currentMissionItem.distance > 0 ? (_currentItemIsVTOLTakeoff ? 0 : (Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) * (180.0 / Math.PI))) : NaN

    // Servo 10 PWM Toggle function
    function toggleServo10PWM() {
        console.log("GenCall1: toggleServo10PWM() - Toggling servo 10 PWM");

        if (!_controllerValid || !_planMasterController.controllerVehicle) {
            console.log("GenCall2: No valid vehicle - cannot send servo command");
            return;
        }

        // Toggle between MIN (400) and MAX (2400)
        if (_servo10Enabled) {
            _servo10PWM = 400;  // MIN value
            _servo10Enabled = false;
            console.log("GenCall3: Setting servo 10 to MIN PWM:", _servo10PWM);
        } else {
            _servo10PWM = 2400; // MAX value
            _servo10Enabled = true;
            console.log("GenCall4: Setting servo 10 to MAX PWM:", _servo10PWM);
        }

        // Send MAV_CMD_DO_SET_SERVO command to vehicle
        // param1: Servo number (10)
        // param2: PWM value (400 or 2400)
        _planMasterController.controllerVehicle.sendMavCommand(_planMasterController.controllerVehicle.defaultComponentId(), 183  // MAV_CMD_DO_SET_SERVO
        , true // showError
        , 10   // param1: Servo number
        , _servo10PWM // param2: PWM value
        , 0, 0, 0, 0, 0, 0  // param3-7: unused
        );

        console.log("GenCall5: Servo 10 PWM command sent - Servo: 10, PWM:", _servo10PWM);
    }

    property string _distanceText: isNaN(_distance) ? "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_distance).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _altDifferenceText: isNaN(_altDifference) ? "-.-" : QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(_altDifference).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
    property string _gradientText: isNaN(_gradient) ? "-.-" : _gradient.toFixed(0) + qsTr(" deg")
    property string _azimuthText: isNaN(_azimuth) ? "-.-" : Math.round(_azimuth) % 360
    property string _headingText: isNaN(_azimuth) ? "-.-" : Math.round(_heading) % 360
    property string _missionPlannedDistanceText: isNaN(_missionPlannedDistance) ? "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionPlannedDistance).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _missionMaxTelemetryText: isNaN(_missionMaxTelemetry) ? "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionMaxTelemetry).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _batteryChangePointText: _batteryChangePoint < 0 ? qsTr("N/A") : _batteryChangePoint
    property string _batteriesRequiredText: _batteriesRequired < 0 ? qsTr("N/A") : _batteriesRequired

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

    // Properties of UTM adapter
    property bool _utmspEnabled: QGroundControl.utmspSupported

    function getMissionTime() {
        if (!_missionTime) {
            return "00:00:00";
        }
        var t = new Date(2021, 0, 0, 0, 0, Number(_missionTime));
        var days = Qt.formatDateTime(t, 'dd');
        var complete;

        if (days == 31) {
            days = '0';
            complete = Qt.formatTime(t, 'hh:mm:ss');
        } else {
            complete = days + " days " + Qt.formatTime(t, 'hh:mm:ss');
        }
        return complete;
    }

    RowLayout {
        id: missionStats
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: _margins
        anchors.left: parent.left
        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCButton {
            id: uploadButton
            text: _controllerDirty ? qsTr("Upload Required") : qsTr("Upload")
            enabled: _utmspEnabled ? !_controllerSyncInProgress && UTMSPStateStorage.enableMissionUploadButton : !_controllerSyncInProgress
            visible: !_controllerOffline && !_controllerSyncInProgress
            primary: _controllerDirty
            onClicked: {
                if (_utmspEnabled) {
                    QGroundControl.utmspManager.utmspVehicle.triggerActivationStatusBar(true);
                    UTMSPStateStorage.removeFlightPlanState = true;
                    UTMSPStateStorage.indicatorDisplayStatus = true;
                }
                _planMasterController.upload();
            }

            PropertyAnimation on opacity {
                easing.type: Easing.OutQuart
                from: 0.5
                to: 1
                loops: Animation.Infinite
                running: _controllerDirty && !_controllerSyncInProgress
                alwaysRunToEnd: true
                duration: 2000
            }
        }

        GridLayout {
            columns: 8
            rowSpacing: _rowSpacing
            columnSpacing: _labelToValueSpacing

            QGCLabel {
                text: qsTr("Selected Waypoint")
                Layout.columnSpan: 8
                font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCLabel {
                text: qsTr("Alt diff:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _altDifferenceText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _mediumValueWidth
            }

            Item {
                width: 1
                height: 1
            }

            QGCLabel {
                text: qsTr("Azimuth:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _azimuthText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _smallValueWidth
            }

            Item {
                width: 1
                height: 1
            }

            QGCLabel {
                text: qsTr("Dist prev WP:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _distanceText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _largeValueWidth
            }

            QGCLabel {
                text: qsTr("Gradient:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _gradientText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _mediumValueWidth
            }

            Item {
                width: 1
                height: 1
            }

            QGCLabel {
                text: qsTr("Heading:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _headingText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _smallValueWidth
            }
        }

        GridLayout {
            columns: 5
            rowSpacing: _rowSpacing
            columnSpacing: _labelToValueSpacing

            QGCLabel {
                text: qsTr("Total Mission")
                Layout.columnSpan: 5
                font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCLabel {
                text: qsTr("Distance:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _missionPlannedDistanceText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _largeValueWidth
            }

            Item {
                width: 1
                height: 1
            }

            QGCLabel {
                text: qsTr("Max telem dist:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _missionMaxTelemetryText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _largeValueWidth
            }

            QGCLabel {
                text: qsTr("Time:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: getMissionTime()
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _largeValueWidth
            }
        }

        GridLayout {
            columns: 3
            rowSpacing: _rowSpacing
            columnSpacing: _labelToValueSpacing
            visible: _batteryInfoAvailable

            QGCLabel {
                text: qsTr("Battery")
                Layout.columnSpan: 3
                font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCLabel {
                text: qsTr("Batteries required:")
                font.pointSize: _dataFontSize
            }
            QGCLabel {
                text: _batteriesRequiredText
                font.pointSize: _dataFontSize
                Layout.minimumWidth: _mediumValueWidth
            }
        }

        QGCButton {
            id: ptahButton
            text: "Ptah"
            Layout.columnSpan: 3
            font.pointSize: _dataFontSize
            onClicked: {
                // Set the vehicles array before opening the dialog
                missionGeneratorDialogue.vehicles = QGroundControl.multiVehicleManager.vehicles;
                missionGeneratorDialogue.open();
            }
        }

        // Servo 10 PWM Toggler
        QGCButton {
            text: _servo10Enabled ? "Servo 10: " + _servo10PWM + " (MAX)" : "Servo 10: " + _servo10PWM + " (MIN)"
            enabled: _controllerValid && _planMasterController.controllerVehicle
            onClicked: {
                console.log("GenCall6: Servo 10 PWM button clicked");
                toggleServo10PWM();
            }
        }

        // Test button for payload installation (for testing purposes)
        QGCButton {
            id: testPayloadButton
            text: "Test: Mark All Payloads Installed"
            visible: true
            onClicked: {
                if (QGroundControl.missionService) {
                    QGroundControl.missionService.testMarkAllPayloadsInstalled();
                }
            }
        }
    }

    // QML Mission Generator Dialogue
    MissionGeneratorDialogue {
        id: missionGeneratorDialogue
        onMissionGenerated: function (missionType, areaSize, altitude, speed, frontDistance, loiterTime, bendHeight, payloadDropHeight, servoDelay, payloadDropMode, observationDistance) {
            // GenCall5: Signal handler receives mission parameters
            console.log("GenCall5: Signal handler receives mission parameters:", missionType, areaSize, altitude, speed, "front distance:", frontDistance, "loiter time:", loiterTime, "bend height:", bendHeight, "payload drop height:", payloadDropHeight, "servo delay:", servoDelay, "payload drop:", payloadDropMode, "observation distance:", observationDistance);

            // GenCall6: Call C++ MissionService to actually generate mission
            console.log("GenCall6: Calling C++ MissionService to generate mission");
            QGroundControl.missionService.generateMission(missionType, areaSize, altitude, speed, "Generated from UI", frontDistance, payloadDropMode, loiterTime, bendHeight, payloadDropHeight, servoDelay, observationDistance);
        }
    }

    // Connect to MissionService signals
    Connections {
        target: QGroundControl.missionService
        function onMissionGenerationStarted() {
            console.log("GenCall7: Mission generation started signal received");
            ptahDialog.text = qsTr("Generating mission...");
            ptahDialog.visible = true;
        }
        function onMissionGenerationCompleted(success, message) {
            console.log("GenCall48: Mission generation completed signal received:", success, message);
            ptahDialog.text = success ? qsTr("Mission generated successfully!\n") + message : qsTr("Mission generation failed!\n") + message;
            ptahDialog.visible = true;
        }
        function onDeminingSuccess() {
            console.log("GenCall70: Demining success signal received");
            deminingSuccessDialog.open();
        }
    }

    QGCSimpleMessageDialog {
        id: ptahDialog
        title: qsTr("Ptah")
        text: qsTr("Waypoint added")
        buttons: Dialog.Ok
        visible: false
        onAccepted: visible = false
    }

    QGCSimpleMessageDialog {
        id: deminingSuccessDialog
        title: qsTr("Demining Complete")
        text: qsTr("Demining successful")
        buttons: Dialog.Ok
        visible: false
        destroyOnClose: false
        onAccepted: {
            visible = false;
            // Trigger green square overlay display
            if (QGroundControl.missionService) {
                QGroundControl.missionService.showDeminingAreaOverlay();
            }
        }
    }

    // All Vehicles Control Functions
    function setAllVehiclesToAUTO() {
        console.log("Setting all vehicles to AUTO mode...");
        var vehicles = QGroundControl.multiVehicleManager.vehicles;
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i);
            if (vehicle) {
                console.log("Setting vehicle", vehicle.id, "to AUTO mode");
                vehicle.flightMode = "AUTO";
            }
        }
    }

    function armAllVehicles() {
        console.log("Arming all vehicles...");
        var vehicles = QGroundControl.multiVehicleManager.vehicles;
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i);
            if (vehicle) {
                console.log("Arming vehicle", vehicle.id);
                vehicle.armed = true;
            }
        }
    }

    function disarmAllVehicles() {
        console.log("Disarming all vehicles...");
        var vehicles = QGroundControl.multiVehicleManager.vehicles;
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i);
            if (vehicle) {
                console.log("Disarming vehicle", vehicle.id);
                vehicle.armed = false;
            }
        }
    }

    function landAllVehicles() {
        console.log("Landing all vehicles...");
        var vehicles = QGroundControl.multiVehicleManager.vehicles;
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i);
            if (vehicle) {
                console.log("Landing vehicle", vehicle.id);
                vehicle.flightMode = "LAND";
            }
        }
    }

    function rtlAllVehicles() {
        console.log("RTL all vehicles...");
        var vehicles = QGroundControl.multiVehicleManager.vehicles;
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i);
            if (vehicle) {
                console.log("RTL vehicle", vehicle.id);
                vehicle.flightMode = "RTL";
            }
        }
    }

    // Collision Alert Dialog - Overlay on top of everything
    Rectangle {
        id: collisionAlertDialog

        property bool showAlert: false
        property string alertMessage: ""

        anchors.centerIn: parent
        z: 1000 // High z-order to appear on top

        width: 400
        height: 200
        color: "#ff4444" // Red background for collision alert
        border.color: "#ff0000"
        border.width: 3
        radius: 10

        visible: showAlert

        // Blinking animation for attention
        SequentialAnimation on opacity {
            running: showAlert
            loops: Animation.Infinite
            NumberAnimation {
                to: 0.7
                duration: 500
            }
            NumberAnimation {
                to: 1.0
                duration: 500
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            // Alert icon and title
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: "⚠️"
                    font.pixelSize: 30
                    color: "white"
                }

                Text {
                    text: "COLLISION DETECTED"
                    font.pixelSize: 20
                    font.bold: true
                    color: "white"
                    Layout.fillWidth: true
                }
            }

            // Alert message
            Text {
                text: alertMessage
                font.pixelSize: 14
                color: "white"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Dismiss button
            QGCButton {
                text: "Dismiss Alert"
                Layout.alignment: Qt.AlignHCenter
                onClicked: {
                    collisionAlertDialog.showAlert = false;
                    QGroundControl.collisionDetectionService.clearCollisionAlerts();
                }
            }
        }

        // Auto-hide after 10 seconds
        Timer {
            id: autoHideTimer
            interval: 10000
            running: showAlert
            onTriggered: {
                collisionAlertDialog.showAlert = false;
            }
        }

        // Connect to collision detection service signals
        Connections {
            target: QGroundControl.collisionDetectionService

            function onCollisionAlert(message) {
                collisionAlertDialog.alertMessage = message;
                collisionAlertDialog.showAlert = true;
                autoHideTimer.restart();
            }

            function onCollisionCleared() {
                collisionAlertDialog.showAlert = false;
            }
        }
    }
}
