import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QGroundControl 1.0
import QGroundControl.Controls 1.0

import QGroundControl

Popup {
    id: root

    property var vehicles: null

    width: ScreenTools.defaultFontPixelWidth * 60
    height: ScreenTools.defaultFontPixelHeight * 40
    parent: Overlay.overlay

    // Center the dialog on screen (simpler approach)
    anchors.centerIn: parent

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: qgcPal.window
        border.color: qgcPal.text
        border.width: 1
        radius: ScreenTools.buttonBorderRadius
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth

        // Header
        QGCLabel {
            text: qsTr("Mission Generator")
            font.pointSize: ScreenTools.defaultFontPointSize + 2
            font.bold: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Content
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: ScreenTools.defaultFontPixelWidth

            ColumnLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    text: qsTr("Generate mission parameters:")
                    font.pointSize: ScreenTools.defaultFontPointSize
                }

                // Mission type selection
                QGCLabel {
                    text: qsTr("Mission Type:")
                    font.pointSize: ScreenTools.defaultFontPointSize
                }

                QGCComboBox {
                    id: missionTypeCombo
                    Layout.fillWidth: true
                    model: ["Survey", "Waypoint", "Orbit", "Custom"]
                    currentIndex: 1
                }

                // Mission parameters
                QGCLabel {
                    text: qsTr("Mission Parameters:")
                    font.pointSize: ScreenTools.defaultFontPointSize
                }

                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                    columnSpacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: qsTr("Area Size (m):")
                    }

                    QGCTextField {
                        id: areaSizeField
                        text: "20"
                        Layout.fillWidth: true
                        validator: IntValidator {
                            bottom: 10
                            top: 10000
                        }
                    }

                    QGCLabel {
                        text: qsTr("Altitude (m):")
                    }

                    QGCTextField {
                        id: altitudeField
                        text: "7"
                        Layout.fillWidth: true
                        validator: IntValidator {
                            bottom: 5
                            top: 500
                        }
                    }

                    QGCLabel {
                        text: qsTr("Speed (m/s):")
                    }

                    QGCTextField {
                        id: speedField
                        text: "5"
                        Layout.fillWidth: true
                        validator: DoubleValidator {
                            bottom: 0.1
                            top: 50.0
                        }
                    }
                }
            }
        }

        // Test Button
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: ScreenTools.defaultFontPixelHeight

            QGCButton {
                text: qsTr("Test")
                Layout.fillWidth: true
                onClicked: {
                    console.log("Test button clicked");
                    getAllVehicles();
                }
            }
        }

        // Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: ScreenTools.defaultFontPixelHeight

            QGCButton {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                onClicked: root.close()
            }

            QGCButton {
                text: qsTr("Generate Mission")
                Layout.fillWidth: true
                primary: true
                onClicked: {
                    // GenCall1: Extract parameters from UI fields
                    console.log("GenCall1: Extracting parameters from UI fields");
                    var missionType = missionTypeCombo.currentText;
                    var areaSize = parseInt(areaSizeField.text);
                    var altitude = parseInt(altitudeField.text);
                    var speed = parseFloat(speedField.text);
                    
                    // GenCall2: Log mission parameters
                    console.log("GenCall2: Logging mission parameters:", missionType, areaSize, altitude, speed);
                    
                    // GenCall3: Emit signal to parent (PlanToolBarIndicators.qml)
                    console.log("GenCall3: Emitting signal to parent");
                    root.missionGenerated(missionType, areaSize, altitude, speed);
                    
                    // GenCall4: Close the dialogue
                    console.log("GenCall4: Closing dialogue");
                    root.close();
                }
            }
        }
    }

    QGCSimpleMessageDialog {
        id: testDialog
        title: qsTr("Ptah")
        text: ""
        buttons: Dialog.Ok
        visible: false
        destroyOnClose: false
        onAccepted: visible = false
    }

    // Signal for mission generation
    signal missionGenerated(string missionType, int areaSize, int altitude, real speed)

    // Get All Vehicles function
    function getAllVehicles() {
        var vehicleInfo = "";
        var vehicleList = [];
        
        if (QGroundControl.multiVehicleManager && QGroundControl.multiVehicleManager.vehicles) {
            var vehicles = QGroundControl.multiVehicleManager.vehicles;
            var vehicleCount = vehicles.count;
            
            console.log("Total vehicles:", vehicleCount);
            
            if (vehicleCount > 0) {
                for (var i = 0; i < vehicleCount; i++) {
                    var vehicle = vehicles.get(i);
                    if (vehicle) {
                        var coord = vehicle.coordinate;
                        
                        // Create structured data
                        var vehicleData = {
                            "index": i,
                            "id": vehicle.id,
                            "latitude": coord.latitude,
                            "longitude": coord.longitude,
                            "altitude": coord.altitude,
                            "coordinateString": coord.toString()
                        };
                        vehicleList.push(vehicleData);
                        
                        // Create formatted text
                        var vehicleText = "Vehicle " + i + ": ID=" + vehicle.id + 
                                        ", Lat=" + coord.latitude.toFixed(7) + 
                                        ", Lng=" + coord.longitude.toFixed(7) + 
                                        ", Alt=" + coord.altitude.toFixed(2) + "m";
                        
                        console.log("Vehicle", i, ":", 
                            "ID:", vehicle.id, 
                            "Lat:", coord.latitude, 
                            "Lng:", coord.longitude, 
                            "Alt:", coord.altitude);
                        
                        vehicleInfo += vehicleText + "\n";
                    }
                }
                vehicleInfo += "Total vehicles: " + vehicleCount;
            } else {
                console.log("No vehicles available");
                vehicleInfo = "No vehicles available";
            }
        } else {
            console.log("MultiVehicleManager not available");
            vehicleInfo = "MultiVehicleManager not available";
        }
        
        testDialog.text = vehicleInfo;
        testDialog.visible = true;
    }

}
