/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning
import Qt.labs.settings 1.1

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
	id: _root

	// Reference to the C++ backend
	property var areaPlanEditor: null
    // Selected vehicle (object) for upload
    property var selectedVehicle: QGroundControl.multiVehicleManager.activeVehicle
    // Mapping: droneIndex -> vehicle object (rebuilt from persisted IDs)
    property var vehicleMapping: ({})
    // Per-drone preview data: array of { droneIndex, altitudeOffsetM, timeOffsetS, waypoints[] }
    property var waypointPreview: []
    // Simple list model for dropdowns (labels) and parallel storage of objects
    property var vehicleLabels: []              // ["Vehicle <id>", ...]
    property var vehicleObjects: []             // [vehicleObj, ...]
    // Track which droneIndex mission was uploaded for (green indicator)
    property var uploadedMap: ({})              // { droneIndex: true }

    // Persisted mapping (vehicle IDs), stored per session/user
    Settings {
        id: areaMapSettings
        category: "AreaPlanEditor"
        property var savedMap: ({}) // { droneIndex: vehicleId }
    }

    // Helper: find current vehicle object by numeric id
    function getVehicleById(id) {
        if (id === undefined || id === null) return null
        for (var i = 0; i < vehicleObjects.length; i++) {
            var v = vehicleObjects[i]
            if (v && v.id === id) return v
        }
        return null
    }

    // Helper: set mapping for a drone, updating both runtime and persisted state
    function setMappingForDrone(droneIndex, veh) {
        vehicleMapping[droneIndex] = veh || null
        var sm = areaMapSettings.savedMap || {}
        sm[droneIndex] = veh ? veh.id : null
        areaMapSettings.savedMap = sm
    }

    // Rebind runtime mapping from persisted IDs (run on app start and when vehicles change)
    function rebindMappingsFromSaved() {
        var sm = areaMapSettings.savedMap
        if (!sm) return
        for (var k in sm) {
            if (!sm.hasOwnProperty(k)) continue
            var id = sm[k]
            var obj = getVehicleById(id)
            vehicleMapping[k] = obj
        }
    }

    function refreshVehicleList() {
        var labels = []
        var objs = []
        var mv = QGroundControl.multiVehicleManager
        if (mv && mv.vehicles) {
            var n = mv.vehicles.count
            for (var i = 0; i < n; i++) {
                var veh = mv.vehicles.get(i)
                if (veh) {
                    var label = veh.id !== undefined ? (qsTr("Vehicle %1").arg(veh.id)) : qsTr("Vehicle")
                    labels.push(label)
                    objs.push(veh)
                }
            }
        }
        vehicleLabels = labels
        vehicleObjects = objs
        // Attempt to rebind runtime mappings to current vehicle objects after list refresh
        rebindMappingsFromSaved()
    }

    // Sizing helpers (no hardcoded sizes)
    readonly property real _h: ScreenTools.defaultFontPixelHeight
    readonly property real _w: ScreenTools.defaultFontPixelWidth


    Component.onCompleted: {
		console.log("AreaPlanEditor: Component completed")
		areaPlanEditor = QGroundControl.areaPlanEditor
		console.log("AreaPlanEditor backend:", !!areaPlanEditor)
        refreshVehicleList()
        rebindMappingsFromSaved()
		if (areaPlanEditor) {
			console.log("AreaPlanEditor properties:")
			console.log("  areaWidth:", areaPlanEditor.areaWidth)
			console.log("  areaHeight:", areaPlanEditor.areaHeight)
			console.log("  isDrawingMode:", areaPlanEditor.isDrawingMode)
            // Initialize preview data
            waypointPreview = areaPlanEditor.computePerDroneWaypointPreview()
		}
	}

    // Keep waypoint preview in sync with editor changes
    // Safe validator shim to avoid errors if backend doesn't expose validateInput
    function _safeValidate(fieldName, value) {
        try {
            if (areaPlanEditor && areaPlanEditor.validateInput) {
                return areaPlanEditor.validateInput(fieldName, value)
            }
        } catch (e) {}
        return ""
    }

    Connections {
        target: areaPlanEditor
        function onMissionUploaded(droneIndex, vehicle) {
            // Mark uploaded for this drone
            uploadedMap[droneIndex] = true
        }
        function _refreshPreview() { if (areaPlanEditor) waypointPreview = areaPlanEditor.computePerDroneWaypointPreview() }
        function onDroneCountChanged() { _refreshPreview() }
        function onAltitudeBandStartChanged() { _refreshPreview() }
        function onAltitudeBandStepChanged() { _refreshPreview() }
        function onTimeOffsetPerDroneChanged() { _refreshPreview() }
        function onMissionAltitudeChanged() { _refreshPreview() }
        function onAreaWidthChanged() { _refreshPreview() }
        function onAreaHeightChanged() { _refreshPreview() }
        function onAreaCenterChanged() { _refreshPreview() }
        function onAreaRotationChanged() { _refreshPreview() }
        function onLineSpacingChanged() { _refreshPreview() }
        function onNumPointsChanged() { _refreshPreview() }
    }

    // Keep vehicle list in sync with connections
    Connections {
        target: QGroundControl.multiVehicleManager
        function onActiveVehicleChanged() { refreshVehicleList() }
    }
    Connections {
        target: QGroundControl.multiVehicleManager ? QGroundControl.multiVehicleManager.vehicles : null
        function onCountChanged() { refreshVehicleList() }
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Rectangle {
		id: background
		anchors.fill: parent
		color: qgcPal.window
		
		ScrollView {
			id: scrollView
			anchors.fill: parent
			ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
			ScrollBar.vertical.policy: ScrollBar.AsNeeded
			
			Column {
				id: mainColumn
				width: scrollView.width
                spacing: _h
                anchors.margins: _h
				
				// Header
				QGCLabel {
					text: qsTr("Area Planning Mission Editor")
					font.pointSize: ScreenTools.largeFontPointSize
					font.bold: true
					width: parent.width
                    height: _h * 2
					wrapMode: Text.WordWrap
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}

				// Area Configuration Section
				Rectangle {
					width: parent.width
                    height: areaConfigColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: areaConfigColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.8

						QGCLabel {
							text: qsTr("Area Configuration Parameters")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
                            height: _h * 1.2
							verticalAlignment: Text.AlignVCenter
						}

						Grid {
							columns: 2
							width: parent.width
                            rowSpacing: _h * 0.6
                            columnSpacing: _w
                            property color _err: '#E53935'

							QGCLabel { 
								text: qsTr("Area Width (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: widthTextField
                                    text: areaPlanEditor ? areaPlanEditor.areaWidth.toString() : "10"
                                    height: _h * 1.6
                                    validator: DoubleValidator { bottom: 1; top: 1000; decimals: 1 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("areaWidth", parseFloat(text))
                                            widthError.text = err
                                            if (err === "") areaPlanEditor.areaWidth = parseFloat(text)
                                        }
                                    }
                                }
                                QGCLabel { id: widthError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

                            // --- Multi-drone parameters ---
                            QGCLabel {
                                text: qsTr("Number of Drones:")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: droneCountField
                                    height: _h * 1.6
                                    text: areaPlanEditor ? areaPlanEditor.droneCount.toString() : "2"
                                    validator: IntValidator { bottom: 1; top: 50 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("droneCount", parseInt(text))
                                            droneCountError.text = err
                                            if (err === "") areaPlanEditor.setDroneCount(parseInt(text))
                                        }
                                    }
                                }
                                QGCLabel { id: droneCountError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

                            QGCLabel {
                                text: qsTr("Altitude Band Start (m):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: bandStartField
                                    height: _h * 1.6
                                    text: areaPlanEditor ? areaPlanEditor.altitudeBandStart.toString() : "0"
                                    validator: DoubleValidator { bottom: 0; top: 10000; decimals: 1 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("missionAltitude", parseFloat(text))
                                            // missionAltitude validator is generic numeric; accept >=0 here
                                            bandStartError.text = ""  // no hard error at UI level
                                            areaPlanEditor.setAltitudeBandStart(parseFloat(text))
                                        }
                                    }
                                }
                                QGCLabel { id: bandStartError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

                            QGCLabel {
                                text: qsTr("Altitude Band Step (m):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: bandStepField
                                    height: _h * 1.6
                                    text: areaPlanEditor ? areaPlanEditor.altitudeBandStep.toString() : "10"
                                    validator: DoubleValidator { bottom: 0.1; top: 10000; decimals: 1 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("altitudeBandStep", parseFloat(text))
                                            bandStepError.text = err
                                            if (err === "") areaPlanEditor.setAltitudeBandStep(parseFloat(text))
                                        }
                                    }
                                }
                                QGCLabel { id: bandStepError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

                            QGCLabel {
                                text: qsTr("Time Offset per Drone (s):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: timeOffsetField
                                    height: _h * 1.6
                                    text: areaPlanEditor ? areaPlanEditor.timeOffsetPerDrone.toString() : "0"
                                    validator: DoubleValidator { bottom: 0; top: 3600; decimals: 1 }
                                    onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setTimeOffsetPerDrone(parseFloat(text))
                                }
                                QGCLabel { text: qsTr("Start staggering per aircraft"); color: qgcPal.colorGrey }
                            }

                            QGCLabel {
                                text: qsTr("Per-Target Separation (s):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                text: areaPlanEditor ? areaPlanEditor.perTargetSeparationS.toString() : "5"
                                validator: DoubleValidator { bottom: 0; top: 3600; decimals: 1 }
                                onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setPerTargetSeparationS(parseFloat(text))
                            }

                            QGCLabel {
                                text: qsTr("RTL after every waypoint:")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCSwitch {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                checked: areaPlanEditor ? areaPlanEditor.rtlAfterEveryWaypoint : false
                                onClicked: if (areaPlanEditor) areaPlanEditor.setRtlAfterEveryWaypoint(checked)
                            }

                            QGCLabel {
                                text: qsTr("Loiter after RTL:")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCSwitch {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                checked: areaPlanEditor ? areaPlanEditor.loiterAfterRtl : false
                                onClicked: if (areaPlanEditor) areaPlanEditor.setLoiterAfterRtl(checked)
                            }

                            QGCLabel {
                                text: qsTr("Land at Target then Return:")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCSwitch {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                checked: areaPlanEditor ? areaPlanEditor.landAtTargetReturn : false
                                onClicked: if (areaPlanEditor) areaPlanEditor.setLandAtTargetReturn(checked)
                            }
							QGCLabel { 
								text: qsTr("Area Height (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: heightTextField
                                    text: areaPlanEditor ? areaPlanEditor.areaHeight.toString() : "10"
                                    height: _h * 1.6
                                    validator: DoubleValidator { bottom: 1; top: 1000; decimals: 1 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("areaHeight", parseFloat(text))
                                            heightError.text = err
                                            if (err === "") areaPlanEditor.areaHeight = parseFloat(text)
                                        }
                                    }
                                }
                                QGCLabel { id: heightError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

							QGCLabel { 
								text: qsTr("Line Spacing (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: lineSpacingTextField
                                    text: areaPlanEditor ? areaPlanEditor.lineSpacing.toString() : "10"
                                    height: _h * 1.6
                                    validator: DoubleValidator { bottom: 1; top: 500; decimals: 1 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("lineSpacing", parseFloat(text))
                                            lineSpacingError.text = err
                                            if (err === "") areaPlanEditor.lineSpacing = parseFloat(text)
                                        }
                                    }
                                }
                                QGCLabel { id: lineSpacingError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

							QGCLabel { 
								text: qsTr("Waypoints Per Line:")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							Column {
                                width: parent.width * 0.5
                                spacing: _h * 0.2
                                QGCTextField {
                                    id: numPointsTextField
                                    text: areaPlanEditor ? areaPlanEditor.numPoints.toString() : "1"
                                    height: _h * 1.6
                                    validator: IntValidator { bottom: 1; top: 50 }
                                    onEditingFinished: {
                                        if (areaPlanEditor && text !== "") {
var err = _safeValidate("numPoints", parseInt(text))
                                            numPointsError.text = err
                                            if (err === "") areaPlanEditor.numPoints = parseInt(text)
                                        }
                                    }
                                }
                                QGCLabel { id: numPointsError; color: parent.parent._err; visible: text.length>0; text: "" }
                            }

							QGCLabel { 
								text: qsTr("Mission Altitude (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}

							QGCTextField {
                                width: parent.width * 0.6
                                height: _h * 1.6
								text: areaPlanEditor ? areaPlanEditor.missionAltitude : 10.0
								placeholderText: qsTr("10.0")
								inputMethodHints: Qt.ImhFormattedNumbersOnly
								validator: DoubleValidator {
									bottom: 1.0
									top: 1000.0
									decimals: 1
									notation: DoubleValidator.StandardNotation
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										var altitude = parseFloat(text)
										if (!isNaN(altitude)) {
											areaPlanEditor.setMissionAltitude(altitude)
										}
									}
								}
							}

							QGCLabel { 
								text: qsTr("Loiter Duration (Seconds):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}

							QGCTextField {
                                width: parent.width * 0.6
                                height: _h * 1.6
								text: areaPlanEditor ? areaPlanEditor.loiterTime : 10.0
								placeholderText: qsTr("10.0")
								inputMethodHints: Qt.ImhFormattedNumbersOnly
								validator: DoubleValidator {
									bottom: 0.0
									top: 3600.0
									decimals: 1
									notation: DoubleValidator.StandardNotation
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										var time = parseFloat(text)
										if (!isNaN(time)) {
											areaPlanEditor.setLoiterTime(time)
										}
									}
								}
							}
						}
					}
				}

				// Interactive Drawing Controls
				Rectangle {
					width: parent.width
                    height: drawingColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: drawingColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.8

						QGCLabel {
							text: qsTr("Interactive Area Definition")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

                        QGCButton {
							id: drawingModeButton
							text: {
								if (!areaPlanEditor) return qsTr("Activate Area Definition Mode")
								return areaPlanEditor.isDrawingMode ? qsTr("Deactivate Area Definition Mode") : qsTr("Activate Area Definition Mode")
							}
							width: parent.width
                            height: _h * 2.2
							onClicked: {
								// Toggle drawing mode using C++ backend
								console.log("Drawing mode button clicked")
								console.log("areaPlanEditor valid:", !!areaPlanEditor)
								if (areaPlanEditor) {
									console.log("Current isDrawingMode:", areaPlanEditor.isDrawingMode)
									var newMode = !areaPlanEditor.isDrawingMode
									console.log("Setting new mode to:", newMode)
									areaPlanEditor.setIsDrawingMode(newMode)
									console.log("After setIsDrawingMode, isDrawingMode:", areaPlanEditor.isDrawingMode)
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}
						
						// Test button to verify C++ backend is working
						QGCButton {
							text: qsTr("Verify System Integration")
							width: parent.width
                            height: _h * 1.5
							onClicked: {
								console.log("Test button clicked")
								if (areaPlanEditor) {
									console.log("areaPlanEditor is valid")
									console.log("areaWidth:", areaPlanEditor.areaWidth)
									console.log("areaHeight:", areaPlanEditor.areaHeight)
									console.log("isDrawingMode:", areaPlanEditor.isDrawingMode)
									
									// Set reasonable defaults if they're 0
									if (areaPlanEditor.areaWidth <= 0) {
										areaPlanEditor.setAreaWidth(10.0)
										console.log("Set default areaWidth to 10")
									}
									if (areaPlanEditor.areaHeight <= 0) {
										areaPlanEditor.setAreaHeight(10.0)
										console.log("Set default areaHeight to 10")
									}
									if (areaPlanEditor.lineSpacing <= 0) {
										areaPlanEditor.setLineSpacing(20.0)
										console.log("Set default lineSpacing to 20")
									}
									if (areaPlanEditor.numPoints <= 0) {
										areaPlanEditor.setNumPoints(5)
										console.log("Set default numPoints to 5")
									}
									
									console.log("After setting defaults:")
									console.log("  areaWidth:", areaPlanEditor.areaWidth)
									console.log("  areaHeight:", areaPlanEditor.areaHeight)
									console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
									console.log("  numPoints:", areaPlanEditor.numPoints)
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}
						
						// Test mission generation
						QGCButton {
							text: qsTr("Validate Mission Generation")
							width: parent.width
                            height: _h * 1.5
							onClicked: {
								console.log("Mission generation test clicked")
								if (areaPlanEditor) {
									console.log("Testing mission generation...")
									console.log("Current parameters:")
									console.log("  areaWidth:", areaPlanEditor.areaWidth)
									console.log("  areaHeight:", areaPlanEditor.areaHeight)
									console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
									console.log("  numPoints:", areaPlanEditor.numPoints)
									console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
									console.log("  areaCenter valid:", areaPlanEditor.areaCenter.isValid)
									
									var waypoints = areaPlanEditor.generateWaypoints()
									console.log("Generated waypoints:", waypoints.length)
									if (waypoints.length > 0) {
										console.log("First waypoint:", waypoints[0])
									}
									// Test saving mission
									areaPlanEditor.saveMissionFile()
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}

						// Debug button to force map item creation
						QGCButton {
							text: qsTr("Refresh Map Display")
							width: parent.width
                            height: _h * 1.5
							onClicked: {
								console.log("Debug: Force map items clicked")
								if (areaPlanEditor) {
									console.log("Current area state:")
									console.log("  areaWidth:", areaPlanEditor.areaWidth)
									console.log("  areaHeight:", areaPlanEditor.areaHeight)
									console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
									console.log("  areaCenter valid:", areaPlanEditor.areaCenter.isValid)
									
									// Force property changes to trigger map updates
									var currentWidth = areaPlanEditor.areaWidth
									var currentHeight = areaPlanEditor.areaHeight
									
									// Temporarily change and restore to trigger signals
									areaPlanEditor.setAreaWidth(currentWidth + 0.1)
									areaPlanEditor.setAreaWidth(currentWidth)
									areaPlanEditor.setAreaHeight(currentHeight + 0.1)
									areaPlanEditor.setAreaHeight(currentHeight)
									
									console.log("Forced property updates completed")
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}

						// Test re-centering functionality
						QGCButton {
							text: qsTr("Validate Area Centering")
							width: parent.width
                            height: _h * 1.5
							onClicked: {
								console.log("Test re-centering clicked")
								if (areaPlanEditor) {
									console.log("Current center:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
									
									// Move center slightly to test re-centering
									var newLat = areaPlanEditor.areaCenter.latitude + 0.001
									var newLon = areaPlanEditor.areaCenter.longitude + 0.001
									var newCenter = QtPositioning.coordinate(newLat, newLon)
									
									console.log("Moving center to:", newLat, newLon)
									areaPlanEditor.setAreaCenter(newCenter)
									
									console.log("New center:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}

						// Reset button
						QGCButton {
							text: qsTr("Reset to Default Parameters")
							width: parent.width
                            height: _h * 1.5
							onClicked: {
								console.log("Reset button clicked")
								if (areaPlanEditor) {
									console.log("Resetting area to default values...")
									areaPlanEditor.resetArea()
									console.log("Area reset completed")
								} else {
									console.log("ERROR: areaPlanEditor is null!")
								}
							}
						}

						QGCLabel {
							text: qsTr("Operating Instructions")
							font.pointSize: ScreenTools.smallFontPointSize
							font.bold: true
							width: parent.width
                            height: _h
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Step 1: Activate Area Definition Mode\nStep 2: Select Center Point on Map\nStep 3: Define Area Boundaries by Dragging\nStep 4: Complete Area Definition with Double-Click")
							font.pointSize: ScreenTools.smallFontPointSize
							width: parent.width
                            height: _h * 3
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
						}
						
						// Status indicator
						Rectangle {
							width: parent.width
                            height: _h * 1.5
                            color: areaPlanEditor && areaPlanEditor.isDrawingMode ? qgcPal.windowShadeDark : qgcPal.windowShade
                            radius: _w * 0.25
                            border.color: qgcPal.colorGrey
                            border.width: 1
							
							QGCLabel {
								anchors.centerIn: parent
								text: areaPlanEditor && areaPlanEditor.isDrawingMode ? qsTr("Area Definition Mode Active") : qsTr("Area Definition Mode Ready")
								font.pointSize: ScreenTools.smallFontPointSize
                                font.bold: areaPlanEditor && areaPlanEditor.isDrawingMode
                                color: qgcPal.text
							}
						}
						
						// Step-by-step flow indicator
						Rectangle {
							width: parent.width
                            height: stepFlowColumn.height + _h
                            color: qgcPal.windowShadeDark
                            radius: _w * 0.25
							border.color: qgcPal.colorGrey
							border.width: 1
							
							Column {
								id: stepFlowColumn
								anchors.left: parent.left
								anchors.right: parent.right
								anchors.top: parent.top
                                anchors.margins: _h * 0.5
                                spacing: _h * 0.4
								
								QGCLabel {
									text: qsTr("Mission Planning Workflow")
									font.pointSize: ScreenTools.smallFontPointSize
									font.bold: true
									color: qgcPal.text
								}
								
								// Step 1: Set Center
								Row {
									width: parent.width
                                    height: _h
                                    spacing: _w * 0.5
									
									Rectangle {
                                        width: _h * 0.8
                                        height: _h * 0.8
                                        radius: _h * 0.4
                                        color: areaPlanEditor && areaPlanEditor.areaCenter.isValid ? qgcPal.windowShade : qgcPal.window
                                        border.color: qgcPal.colorGrey
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("Step 1: Define Area Center Point")
										font.pointSize: ScreenTools.smallFontPointSize
                                        color: qgcPal.text
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 2: Define Area
								Row {
									width: parent.width
                                    height: _h
                                    spacing: _w * 0.5
									
									Rectangle {
                                        width: _h * 0.8
                                        height: _h * 0.8
                                        radius: _h * 0.4
                                        color: areaPlanEditor && areaPlanEditor.areaWidth > 0 && areaPlanEditor.areaHeight > 0 ? qgcPal.windowShade : qgcPal.window
                                        border.color: qgcPal.colorGrey
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("Step 2: Configure Area Dimensions")
										font.pointSize: ScreenTools.smallFontPointSize
                                        color: qgcPal.text
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 3: Generate Mission
								Row {
									width: parent.width
                                    height: _h
                                    spacing: _w * 0.5
									
									Rectangle {
                                        width: _h * 0.8
                                        height: _h * 0.8
                                        radius: _h * 0.4
                                        color: areaPlanEditor && areaPlanEditor.numPoints > 0 ? qgcPal.windowShade : qgcPal.window
                                        border.color: qgcPal.colorGrey
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("Step 3: Generate Mission Waypoints")
										font.pointSize: ScreenTools.smallFontPointSize
                                        color: qgcPal.text
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 4: Save Mission
								Row {
									width: parent.width
                                    height: _h
                                    spacing: _w * 0.5
									
									Rectangle {
                                        width: _h * 0.8
                                        height: _h * 0.8
                                        radius: _h * 0.4
                                        color: qgcPal.window
                                        border.color: qgcPal.colorGrey
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("Step 4: Transfer Mission to Vehicle (Optional)")
										font.pointSize: ScreenTools.smallFontPointSize
                                        color: qgcPal.colorGrey
										anchors.verticalCenter: parent.verticalCenter
									}
								}
							}
						}
					}
				}

				// Area Movement Controls
				Rectangle {
					width: parent.width
                    height: movementColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: movementColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.8

						QGCLabel {
							text: qsTr("Area Position Controls")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						Item {
							width: parent.width
                            height: _h * 6
							
							Grid {
								anchors.centerIn: parent
								columns: 3
                                rowSpacing: _h * 0.4
                                columnSpacing: _w * 0.5

                                Item { width: _w * 6; height: _h * 2 }
								QGCButton {
									text: qsTr("↑")
                                    width: _w * 6
                                    height: _h * 2
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaNorth()
								}
                                Item { width: _w * 6; height: _h * 2 }

								QGCButton {
									text: qsTr("←")
                                    width: _w * 6
                                    height: _h * 2
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaWest()
								}
								QGCButton {
									text: qsTr("Center Area")
                                    width: _w * 12
                                    height: _h * 2
									onClicked: if (areaPlanEditor) areaPlanEditor.centerArea()
								}
								QGCButton {
									text: qsTr("→")
                                    width: _w * 6
                                    height: _h * 2
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaEast()
								}

                                Item { width: _w * 6; height: _h * 2 }
								QGCButton {
									text: qsTr("↓")
                                    width: _w * 6
                                    height: _h * 2
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaSouth()
								}
                                Item { width: _w * 6; height: _h * 2 }
							}
						}
					}
				}

				// Rotation Controls Section
				Rectangle {
					width: parent.width
                    height: rotationControlsColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: rotationControlsColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.6

						QGCLabel {
							text: qsTr("Rotation Controls")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						// Current rotation display
						Row {
							width: parent.width
                            height: _h * 1.6
                            spacing: _w * 0.5

							QGCLabel {
								text: qsTr("Current Rotation")
								width: parent.width * 0.4
								height: parent.height
								verticalAlignment: Text.AlignVCenter
							}

							QGCLabel {
								text: areaPlanEditor ? qsTr("%1°").arg(areaPlanEditor.areaRotation.toFixed(1)) : qsTr("0.0°")
								width: parent.width * 0.3
                                height: parent.height
								verticalAlignment: Text.AlignVCenter
								font.bold: true
							}

							QGCLabel {
								text: areaPlanEditor && areaPlanEditor.areaRotation > 0 ? qsTr("(North = 0°)") : ""
								width: parent.width * 0.3
								height: parent.height
								verticalAlignment: Text.AlignVCenter
								font.pointSize: ScreenTools.smallFontPointSize
								color: qgcPal.colorGrey
							}
						}

						// Rotation input field
						Row {
							width: parent.width
                            height: _h * 1.6
                            spacing: _w * 0.5

							QGCLabel {
								text: qsTr("Set Rotation")
								width: parent.width * 0.4
								height: parent.height
								verticalAlignment: Text.AlignVCenter
							}

							QGCTextField {
								id: rotationInput
								width: parent.width * 0.3
                                height: parent.height
								text: areaPlanEditor ? areaPlanEditor.areaRotation.toFixed(1) : "0.0"
								placeholderText: qsTr("0.0")
								inputMethodHints: Qt.ImhFormattedNumbersOnly
								validator: DoubleValidator {
									bottom: 0.0
									top: 359.9
									decimals: 1
									notation: DoubleValidator.StandardNotation
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										var rotation = parseFloat(text)
										if (!isNaN(rotation)) {
											areaPlanEditor.setAreaRotation(rotation)
										}
									}
								}
							}

							QGCLabel {
								text: qsTr("Degrees")
								width: parent.width * 0.3
								height: parent.height
								verticalAlignment: Text.AlignVCenter
								color: qgcPal.colorGrey
							}
						}

						// Rotation buttons
						Row {
							width: parent.width
                            height: _h * 2
                            spacing: _w * 0.5

							QGCButton {
								text: qsTr("Rotate Counterclockwise (-15°)")
                                width: parent.width * 0.3
                                height: parent.height
								onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaCounterClockwise()
							}

							QGCButton {
								text: qsTr("Reset Rotation to 0°")
                                width: parent.width * 0.4
                                height: parent.height
								onClicked: if (areaPlanEditor) areaPlanEditor.setAreaRotation(0.0)
							}

							QGCButton {
								text: qsTr("Rotate Clockwise (+15°)")
                                width: parent.width * 0.3
                                height: parent.height
								onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaClockwise()
							}
						}
					}
				}

				// Mission Controls Section
                Rectangle {
                    width: parent.width
                    height: missionControlsColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5

                    Column {
                        id: missionControlsColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.6

						QGCLabel {
							text: qsTr("Mission Controls")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
                            height: _h * 1.2
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Generate waypoints from the current area plan and add them to the Mission Tab. This function works with or without a connected vehicle.")
							width: parent.width
                            height: _h * 2
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
							font.pointSize: ScreenTools.smallFontPointSize
							color: qgcPal.colorGrey
						}

						QGCButton {
							text: qsTr("Generate Mission and Add to Mission Tab")
							width: parent.width
                            height: _h * 2.2
							onClicked: {
								if (areaPlanEditor) {
									console.log("Generate Mission button clicked")
									areaPlanEditor.addWaypointsToMission()
								}
							}
						}

						QGCButton {
							text: qsTr("Save Mission File")
							width: parent.width
                            height: _h * 2.2
							onClicked: if (areaPlanEditor) areaPlanEditor.saveMissionFile()
						}

                        QGCButton {
                            text: qsTr("Clear Mission Items")
                            width: parent.width
                            height: _h * 2.2
                            onClicked: if (areaPlanEditor) areaPlanEditor.clearMission()
                        }

                        // Per-Drone Mission Insertion (non-aggregated)
                        Row {
                            width: parent.width
                            height: _h * 2
                            spacing: _w

                            QGCLabel {
                                text: qsTr("Insert Aircraft Number\ninto Mission:")
                                width: parent.width * 0.45
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            QGCTextField {
                                id: droneIndexField
                                width: parent.width * 0.2
                                height: parent.height
                                text: qsTr("0")
                                validator: IntValidator { bottom: 0; top: 99 }
                            }
                            QGCButton {
                                text: qsTr("Insert")
                                width: parent.width * 0.3
                                height: parent.height
                                onClicked: {
                                    if (areaPlanEditor) {
                                        var idx = parseInt(droneIndexField.text)
                                        if (!isNaN(idx)) {
                                            areaPlanEditor.addPerDroneToMission(idx)
                                        }
                                    }
                                }
                            }
                        }

                        QGCButton {
                            text: qsTr("Insert All Drones")
                            width: parent.width
                            height: _h * 2
                            onClicked: if (areaPlanEditor) areaPlanEditor.addAllDronesToMission()
                        }

                        // Save per-drone WPL files
                        QGCButton {
                            text: qsTr("Save Per-Drone Mission Files")
                            width: parent.width
                            height: _h * 2
                            onClicked: if (areaPlanEditor) areaPlanEditor.savePerDroneMissionFiles()
                        }

                        // Upload per-drone mission to active vehicle
                        Row {
                            width: parent.width
                            height: _h * 2
                            spacing: _w
                            // Vehicle selector
                            // Guarded vehicle selector for upload
                            Item {
                                width: parent.width * 0.35
                                height: parent.height
                                property int _vehCount: vehicleLabels.length
                                QGCComboBox {
                                    id: droneVehicleSelector
                                    anchors.fill: parent
                                    visible: parent._vehCount > 0
                                    model: vehicleLabels
                                    // choose active vehicle if present
                                    currentIndex: (function(){
                                        if (QGroundControl.multiVehicleManager.activeVehicle) {
                                            for (var i = 0; i < vehicleObjects.length; i++) {
                                                if (vehicleObjects[i] === QGroundControl.multiVehicleManager.activeVehicle) return i
                                            }
                                        }
                                        return parent._vehCount > 0 ? 0 : -1
                                    })()
                                    onActivated: {
                                        if (currentIndex >= 0 && currentIndex < vehicleObjects.length) {
                                            selectedVehicle = vehicleObjects[currentIndex]
                                        }
                                    }
                                }
                                QGCLabel {
                                    anchors.centerIn: parent
                                    visible: parent._vehCount === 0
                                    color: qgcPal.colorGrey
                                    text: qsTr("No vehicles")
                                }
                            }
                            QGCLabel {
                                text: qsTr("Upload Aircraft Number\n to Vehicle:")
                                width: parent.width * 0.25
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            QGCTextField {
                                id: uploadDroneIndexField
                                width: parent.width * 0.1
                                height: parent.height
                                text: qsTr("0")
                                validator: IntValidator { bottom: 0; top: 99 }
                            }
                            QGCButton {
                                text: qsTr("Upload")
                                width: parent.width * 0.2
                                height: parent.height
                                onClicked: {
                                    if (areaPlanEditor) {
                                        var idx = parseInt(uploadDroneIndexField.text)
                                        if (!isNaN(idx)) {
                                            if (selectedVehicle) {
                                                areaPlanEditor.uploadPerDroneMissionToVehicle(idx, selectedVehicle)
                                            } else {
                                                areaPlanEditor.uploadPerDroneMissionToVehicle(idx)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Quick planning status
                        Row {
                            width: parent.width
                            height: _h * 1.6
                            spacing: _w
                            QGCLabel {
                                text: qsTr("Planned aircraft: %1").arg(areaPlanEditor ? areaPlanEditor.droneCount : 0)
                                width: parent.width * 0.33
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCLabel {
                                text: qsTr("Connected vehicles: %1").arg(QGroundControl.multiVehicleManager.vehicles.count)
                                width: parent.width * 0.33
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCLabel {
                                text: {
                                    var mapped = 0
                                    for (var k in vehicleMapping) if (vehicleMapping.hasOwnProperty(k) && vehicleMapping[k]) mapped++
                                    return qsTr("Mapped: %1").arg(mapped)
                                }
                                width: parent.width * 0.33
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: (function(){
                                    var planned = areaPlanEditor ? areaPlanEditor.droneCount : 0
                                    var m=0; for (var k in vehicleMapping) if (vehicleMapping[k]) m++
                                    return (m < planned) ? qgcPal.colorOrange : qgcPal.text
                                })()
                            }
                        }

                        // Per-Drone Vehicle Mapping
                        Rectangle {
                            width: parent.width
                            height: perDroneMapColumn.height + _h
                            color: qgcPal.windowShade
                            radius: _w * 0.25
                            border.color: qgcPal.colorGrey
                            border.width: 1

                            Column {
                                id: perDroneMapColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: _h * 0.5
                                spacing: _h * 0.4

                                // Safety confirm dialog
                                property var _pendingAction: null
                                    Dialog {
                                    id: confirmDialog
                                    modal: true
                                    title: qsTr("Confirm Action")
                                    standardButtons: Dialog.Ok | Dialog.Cancel
                                    onAccepted: { if (perDroneMapColumn._pendingAction) { perDroneMapColumn._pendingAction(); perDroneMapColumn._pendingAction = null } }
                                    onRejected: { perDroneMapColumn._pendingAction = null }
                                    contentItem: Column {
                                        padding: _h * 0.5
                                        spacing: _h * 0.5
                                        QGCLabel { id: confirmText; wrapMode: Text.WordWrap }
                                    }
                                    Keys.onEscapePressed: confirmDialog.close()
                                }
                                function confirmAndRun(msg, fn) {
                                    confirmText.text = msg
                                    perDroneMapColumn._pendingAction = fn
                                    confirmDialog.open()
                                }

                                // Helper to schedule delayed starts
                                function scheduleStart(veh, delaySec) {
                                    var t = Qt.createQmlObject('import QtQuick 2.15; Timer { property var _veh; interval: ' + Math.max(0, Math.floor(delaySec * 1000)) + '; repeat: false; }', perDroneMapColumn, 'StartTimer')
                                    t._veh = veh
                                    t.triggered.connect(function(){ if (areaPlanEditor) areaPlanEditor.startMissionOnVehicle(t._veh); t.destroy() })
                                    t.start()
                                }

                                QGCLabel {
                                    text: qsTr("Per-Drone Vehicle Mapping")
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                }
                                Row {
                                    width: parent.width
                                    height: _h * 1.6
                                    spacing: _w
                                    QGCButton {
                                        text: qsTr("Map All")
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Map Aircraft 0..N to first N connected vehicles")
                                        enabled: vehicleObjects.length > 0 && waypointPreview && waypointPreview.length > 0
                                        onClicked: {
                                            if (waypointPreview && vehicleObjects.length > 0) {
                                                var count = Math.min(waypointPreview.length, vehicleObjects.length)
                                                for (var i = 0; i < count; i++) {
                                                    var veh = vehicleObjects[i]
                                                    setMappingForDrone(waypointPreview[i].droneIndex, veh)
                                                }
                                                if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Mapped %1 aircraft to %2 vehicles").arg(count).arg(count))
                                            }
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("Unmap All")
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Clear all aircraft-to-vehicle mappings")
                                        enabled: (function(){
                                            if (!waypointPreview) return false
                                            var sm = areaMapSettings.savedMap
                                            if (!sm) return false
                                            for (var i = 0; i < waypointPreview.length; i++) {
                                                if (sm[waypointPreview[i].droneIndex]) return true
                                            }
                                            return false
                                        })()
                                        onClicked: {
                                            if (waypointPreview) {
                                                for (var i = 0; i < waypointPreview.length; i++) {
                                                    var di = waypointPreview[i].droneIndex
                                                    setMappingForDrone(di, null)
                                                }
                                                if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Cleared all mappings"))
                                            }
                                        }
                                    }
                                }

                                // Build rows dynamically from preview (one row per drone)
                                Repeater {
                                    model: waypointPreview
                                    delegate: Row {
                                        width: parent.width
                                        height: _h * 1.6
                                        spacing: _w

                                        QGCLabel {
                                            text: qsTr("Aircraft %1").arg(modelData.droneIndex)
                                            width: parent.width * 0.15
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        // Vehicle selector (guarded for empty vehicle list)
                                        Item {
                                            width: parent.width * 0.30
                                            height: parent.height
                                            visible: true
                                            property int _vehCount: vehicleLabels.length
                                            QGCComboBox {
                                                id: vehicleCombo
                                                anchors.fill: parent
                                                visible: parent._vehCount > 0
                                                model: vehicleLabels
                                                currentIndex: parent._vehCount > 0 ? 0 : -1
                                                onActivated: {
                                                    if (currentIndex >= 0 && currentIndex < vehicleObjects.length) {
                                                        var veh = vehicleObjects[currentIndex]
                                                        // Do not auto-commit mapping on selection; wait for explicit Map click
                                                    }
                                                }
                                            }
                                            QGCLabel {
                                                anchors.centerIn: parent
                                                visible: parent._vehCount === 0
                                                color: qgcPal.colorGrey
                                                text: qsTr("No vehicles")
                                            }
                                        }

                                        // Explicit map button for user confirmation
                                        QGCButton {
                                            text: qsTr("Map")
                                            width: _w * 10
                                            height: parent.height
                                            enabled: (function(){
                                                var count = vehicleLabels.length
                                                return count > 0 && vehicleCombo.currentIndex >= 0 && vehicleCombo.currentIndex < vehicleObjects.length
                                            })()
                                            onClicked: {
                                                if (vehicleCombo.currentIndex >= 0 && vehicleCombo.currentIndex < vehicleObjects.length) {
                                                    var veh = vehicleObjects[vehicleCombo.currentIndex]
                                                    setMappingForDrone(modelData.droneIndex, veh)
                                                    if (areaPlanEditor) {
                                                        var vid = (veh && typeof veh.id !== 'undefined') ? veh.id : "?"
                                                        areaPlanEditor.updateStatus(qsTr("Mapped Aircraft %1 to Vehicle %2").arg(modelData.droneIndex).arg(vid))
                                                    }
                                                }
                                            }
                                            Keys.onReturnPressed: clicked()
                                            Keys.onEnterPressed: clicked()
                                            focus: true
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Assign selected vehicle to this aircraft")
                                        }

                                        // Status summary (live via vehicle properties)
                                        QGCLabel {
                                            width: parent.width * 0.25
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            color: qgcPal.colorGrey
                                            text: {
                                                var veh = vehicleMapping[modelData.droneIndex]
                                                var sm = areaMapSettings.savedMap || {}
                                                var hadId = sm[modelData.droneIndex] !== undefined && sm[modelData.droneIndex] !== null
                                                if (!veh) return hadId ? qsTr("Vehicle disconnected") : qsTr("No vehicle mapped")
                                                var arm = veh.armed === true ? qsTr("ARMED") : qsTr("DISARMED")
                                                var fm = veh.flightMode ? veh.flightMode : "?"
                                                return qsTr("%1 | %2").arg(arm).arg(fm)
                                            }
                                        }
                                        QGCButton {
                                            text: qsTr("Unmap")
                                            width: _w * 10
                                            height: parent.height
                                            enabled: (function(){
                                                var sm = areaMapSettings.savedMap || {}
                                                return sm[modelData.droneIndex] !== undefined && sm[modelData.droneIndex] !== null
                                            })()
                                            onClicked: {
                                                setMappingForDrone(modelData.droneIndex, null)
                                                if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Unmapped Aircraft %1").arg(modelData.droneIndex))
                                            }
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Clear mapping for this aircraft")
                                        }

                                        QGCButton {
                                            text: qsTr("Upload")
                                            width: parent.width * 0.2
                                            height: parent.height
                                            onClicked: {
                                                if (areaPlanEditor) {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh) {
                                                        // Reset status to pending until signal arrives
                                                        uploadedMap[modelData.droneIndex] = false
                                                        areaPlanEditor.uploadPerDroneMissionToVehicle(modelData.droneIndex, veh)
                                                    }
                                                }
                                            }
                                        }
                                        // Upload status indicator
                                        Rectangle {
                                            width: _w * 2
                                            height: _w * 2
                                            radius: _w
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: uploadedMap[modelData.droneIndex] ? "#3CB371" : qgcPal.windowShade
                                            border.color: qgcPal.colorGrey
                                            border.width: 1
                                            ToolTip.visible: hovered
                                            ToolTip.text: uploadedMap[modelData.droneIndex] ? qsTr("Relative mission uploaded") : qsTr("Not uploaded yet")
                                        }
                                    }
                                }

                                Row {
                                    width: parent.width
                                    height: _h * 2
                                    spacing: _w
                                    QGCButton {
                                        text: qsTr("Upload All Mapped")
                                        onClicked: {
                                            if (areaPlanEditor && waypointPreview && waypointPreview.length > 0) {
                                                for (var i = 0; i < waypointPreview.length; i++) {
                                                    var d = waypointPreview[i]
                                                    var veh = vehicleMapping[d.droneIndex]
                                                    if (veh) areaPlanEditor.uploadPerDroneMissionToVehicle(d.droneIndex, veh)
                                                }
                                            }
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("Sync Missions (All Mapped)")
                                        onClicked: {
                                            if (areaPlanEditor && waypointPreview) {
                                                for (var i = 0; i < waypointPreview.length; i++) {
                                                    var d = waypointPreview[i]
                                                    var veh = vehicleMapping[d.droneIndex]
                                                    if (veh) areaPlanEditor.uploadPerDroneMissionToVehicle(d.droneIndex, veh)
                                                }
                                            }
                                        }
                                    }
                                    // Start all mapped missions with optional stagger
                                    QGCTextField {
                                        id: staggerField
                                        width: _w * 10
                                        height: parent.height
                                        text: "3"
                                        validator: DoubleValidator { bottom: 0; top: 3600; decimals: 1 }
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Stagger seconds between mission starts")
                                    }
                                    QGCButton {
                                        text: qsTr("Start All Mapped (Staggered)")
                                        onClicked: {
                                            var stagger = parseFloat(staggerField.text)
                                            if (isNaN(stagger) || stagger < 0) stagger = 0
                                            perDroneMapColumn.confirmAndRun(qsTr("Start missions on all mapped vehicles?\nStagger: %1 s").arg(stagger), function(){
                                                if (waypointPreview) {
                                                    for (var i = 0; i < waypointPreview.length; i++) {
                                                        var d = waypointPreview[i]
                                                        var veh = vehicleMapping[d.droneIndex]
                                                        if (veh) perDroneMapColumn.scheduleStart(veh, i * stagger)
                                                    }
                                                }
                                            })
                                        }
                                    }
                                }

                                // Helpers for per-vehicle gating similar to FlyView
                                function _hasMissionItems() {
                                    if (!areaPlanEditor) return false
                                    var mc = areaPlanEditor.getMissionController()
                                    return mc && mc.containsItems
                                }
                                function _canStartMissionFor(veh) {
                                    if (!veh) return false
                                    if (veh.flying === true) return false
                                    // Require at least 1m relative altitude before mission start
                                    var altOk = (veh.altitudeRelative !== undefined) ? (veh.altitudeRelative >= 1.0) : true
                                    // Health and arming report gating
                                    var rep = veh.healthAndArmingCheckReport
                                    var ok = (!rep || rep.supported === false || rep.canStartMission === true)
                                    return ok && altOk && _hasMissionItems()
                                }

                                // Per-vehicle controls
                                Column {
                                    width: parent.width
                                    spacing: _h * 0.25
                                    QGCLabel { text: qsTr("Per-Aircraft Controls"); font.bold: true }
                                    Repeater {
                                        model: waypointPreview
                                        delegate: Row {
                                            width: parent.width
                                            height: _h * 1.8
                                            spacing: _w
                                            QGCLabel { text: qsTr("Aircraft %1").arg(modelData.droneIndex); width: parent.width * 0.15; verticalAlignment: Text.AlignVCenter }
                                            QGCButton {
                                                text: qsTr("Arm")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && !Boolean(veh.armed)
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) {
                                                        perDroneMapColumn.confirmAndRun(qsTr("Arm vehicle %1?").arg(veh.id), function(){ areaPlanEditor.armVehicle(veh, true) })
                                                    }
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (Boolean(veh.armed)) return qsTr("Already armed")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                            QGCButton {
                                                text: qsTr("Disarm")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && Boolean(veh.armed)
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.armVehicle(veh, false)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (!Boolean(veh.armed)) return qsTr("Already disarmed")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                            QGCButton {
                                                text: qsTr("Takeoff")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && veh.takeoffVehicleSupported === true && veh.flying === false
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.takeoffVehicle(veh, areaPlanEditor ? areaPlanEditor.missionAltitude : 10)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (veh.flying === true) return qsTr("Already flying")
                                                    if (veh.takeoffVehicleSupported !== true) return qsTr("Takeoff not supported")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                            QGCButton {
                                                text: qsTr("Start Mission")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return _canStartMissionFor(veh)
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) {
                                                        perDroneMapColumn.confirmAndRun(qsTr("Start mission on vehicle %1?").arg(veh.id), function(){ areaPlanEditor.startMissionOnVehicle(veh) })
                                                    }
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: qsTr("Cannot start: ensure mission is uploaded and health checks pass")
                                            }
                                            QGCButton {
                                                text: qsTr("Pause")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && veh.pauseVehicleSupported === true && veh.flying === true
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.pauseMissionOnVehicle(veh)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (veh.pauseVehicleSupported !== true) return qsTr("Pause not supported")
                                                    if (veh.flying !== true) return qsTr("Vehicle not flying")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                            QGCButton {
                                                text: qsTr("Land")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    // Enable if we have a vehicle, it's armed (or flying), and guided/land supported if exposed
                                                    var armed = Boolean(veh && veh.armed)
                                                    var flying = Boolean(veh && veh.flying)
                                                    var guidedOk = (veh && (veh.guidedModeSupported === true || veh.landModeSupported === true))
                                                    return (!!veh) && (armed || flying) && (guidedOk || true)
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.landVehicle(veh)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (!Boolean(veh.armed) && !Boolean(veh.flying)) return qsTr("Not armed or flying")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                            QGCButton {
                                                text: qsTr("RTL")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && veh.guidedModeSupported === true && veh.armed === true
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.rtlVehicle(veh)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (veh.guidedModeSupported !== true) return qsTr("Guided/RTL not supported")
                                                    if (!Boolean(veh.armed)) return qsTr("Vehicle not armed")
                                                    return qsTr("Unavailable")
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

						QGCButton {
							text: qsTr("Upload to Vehicle")
							width: parent.width
                            height: _h * 2.2
							enabled: false // TODO: Implement vehicle connection
							onClicked: if (areaPlanEditor) areaPlanEditor.uploadToVehicle()
						}

						QGCButton {
							text: qsTr("Start Mission")
							width: parent.width
                            height: _h * 2.2
							enabled: false // TODO: Implement mission start
							onClicked: if (areaPlanEditor) areaPlanEditor.startMission()
						}
					}
				}

				// Mission Statistics
				Rectangle {
					width: parent.width
                    height: statsColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: statsColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.5

						QGCLabel {
							text: qsTr("Mission Statistics")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
                            height: _h * 1.2
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Area Size: %1 m × %2 m").arg(areaPlanEditor ? areaPlanEditor.areaWidth : 30).arg(areaPlanEditor ? areaPlanEditor.areaHeight : 90)
							width: parent.width
                            height: _h
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Total Number of Waypoints: %1").arg(areaPlanEditor ? areaPlanEditor.calculateTotalWaypoints() : 0)
							width: parent.width
                            height: _h
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Estimated Flight Time: %1 Minutes").arg(areaPlanEditor ? areaPlanEditor.calculateFlightTime() : 0)
							width: parent.width
                            height: _h
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Loiter Time per Waypoint: %1 Seconds").arg(areaPlanEditor ? areaPlanEditor.loiterTime : 10)
							width: parent.width
                            height: _h
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Area Center Coordinates: %1, %2").arg(areaPlanEditor && areaPlanEditor.areaCenter ? areaPlanEditor.areaCenter.latitude.toFixed(6) : "0.000000").arg(areaPlanEditor && areaPlanEditor.areaCenter ? areaPlanEditor.areaCenter.longitude.toFixed(6) : "0.000000")
							width: parent.width
                            height: _h * 2
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
						}

                        // Per-Drone Waypoint Preview
                        Rectangle {
                            width: parent.width
                            height: previewColumn.height + _h
                            color: qgcPal.windowShadeDark
                            radius: _w * 0.25
                            border.color: qgcPal.colorGrey
                            border.width: 1

                            Column {
                                id: previewColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: _h * 0.5
                                spacing: _h * 0.4

                                QGCLabel {
                                    text: qsTr("Per-Drone Waypoint Preview")
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                    color: qgcPal.text
                                }

                                QGCButton {
                                    text: qsTr("Refresh Preview")
                                    width: parent.width
                                    height: _h * 1.8
                                    onClicked: {
                                        if (areaPlanEditor) {
                                            waypointPreview = areaPlanEditor.computePerDroneWaypointPreview()
                                        }
                                    }
                                }

                                Repeater {
                                    model: waypointPreview
                                    delegate: Row {
                                        width: parent.width
                                        height: _h * 1.4
                                        spacing: _w

                                        QGCLabel {
                                            text: qsTr("Aircraft %1").arg(modelData.droneIndex)
                                            width: parent.width * 0.3
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        QGCLabel {
                                            text: qsTr("Waypoints: %1").arg(modelData.waypoints ? modelData.waypoints.length : 0)
                                            width: parent.width * 0.3
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        QGCLabel {
                                            text: qsTr("Alt +%1 m, T +%2 s").arg(modelData.altitudeOffsetM).arg(modelData.timeOffsetS)
                                            width: parent.width * 0.4
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            color: qgcPal.colorGrey
                                        }
                                    }
                                }
                            }
                        }
					}
				}

				// Status Section
				Rectangle {
					width: parent.width
                    height: statusColumn.height + _h * 2
                    color: qgcPal.windowShade
                    radius: _w * 0.5
					
					Column {
						id: statusColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
                        anchors.margins: _h
                        spacing: _h * 0.5

						QGCLabel {
							text: qsTr("Status")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
                            height: _h * 1.2
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							id: statusLabel
							text: qsTr("Ready to Generate Mission")
							color: qgcPal ? qgcPal.text : "white"
							width: parent.width
                            height: _h * 2
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
						}
					}
				}
			}
		}

	Connections {
		target: areaPlanEditor
		function onStatusChanged(message) {
			if (statusLabel) statusLabel.text = message
		}
		
		function onIsDrawingModeChanged() {
			console.log("AreaPlanEditor: C++ backend isDrawingMode changed to:", areaPlanEditor.isDrawingMode)
			// Force button text update
			drawingModeButton.text = areaPlanEditor.isDrawingMode ? qsTr("Stop Drawing Mode") : qsTr("Start Drawing Mode")
		}
	}

	// Debug Section
    // Rectangle {
    //     // Hide debug tools unless Advanced UI is enabled
    //     visible: QGroundControl.corePlugin && QGroundControl.corePlugin.showAdvancedUI
	// 	anchors.bottom: parent.bottom
	// 	anchors.left: parent.left
	// 	anchors.right: parent.right
    //     height: debugColumn.height + _h * 2
    //     color: qgcPal.windowShade
    //     radius: _w * 0.5
		
	// 	Column {
	// 		id: debugColumn
	// 		anchors.left: parent.left
	// 		anchors.right: parent.right
	// 		anchors.top: parent.top
    //         anchors.margins: _h
    //         spacing: _h * 0.5

	// 		QGCLabel {
	// 			text: qsTr("Debug Tools")
	// 			font.pointSize: ScreenTools.mediumFontPointSize
	// 			font.bold: true
	// 			width: parent.width
    //             height: _h * 1.2
	// 			verticalAlignment: Text.AlignVCenter
	// 		}

	// 		// Test C++ Backend
	// 		QGCButton {
	// 			text: qsTr("Test C++ Backend")
	// 			width: parent.width
    //             height: _h * 1.5
	// 			onClicked: {
	// 				console.log("Test C++ Backend clicked")
	// 				if (areaPlanEditor) {
	// 					console.log("C++ Backend is accessible!")
	// 					console.log("Current properties:")
	// 					console.log("  areaWidth:", areaPlanEditor.areaWidth)
	// 					console.log("  areaHeight:", areaPlanEditor.areaHeight)
	// 					console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
	// 					console.log("  numPoints:", areaPlanEditor.numPoints)
	// 					console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
	// 					console.log("  isDrawingMode:", areaPlanEditor.isDrawingMode)
						
	// 					// Set reasonable defaults if values are zero
	// 					if (areaPlanEditor.areaWidth <= 0) {
	// 						areaPlanEditor.setAreaWidth(10.0)
	// 						console.log("Set areaWidth to 10.0")
	// 					}
	// 					if (areaPlanEditor.areaHeight <= 0) {
	// 						areaPlanEditor.setAreaHeight(10.0)
	// 						console.log("Set areaHeight to 10.0")
	// 					}
	// 					if (areaPlanEditor.lineSpacing <= 0) {
	// 						areaPlanEditor.setLineSpacing(10.0)
	// 						console.log("Set lineSpacing to 10.0")
	// 					}
	// 					if (areaPlanEditor.numPoints <= 0) {
	// 						areaPlanEditor.setNumPoints(1)
	// 						console.log("Set numPoints to 1")
	// 					}
						
	// 					console.log("Updated properties:")
	// 					console.log("  areaWidth:", areaPlanEditor.areaWidth)
	// 					console.log("  areaHeight:", areaPlanEditor.areaHeight)
	// 					console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
	// 					console.log("  numPoints:", areaPlanEditor.numPoints)
	// 				} else {
	// 					console.log("ERROR: areaPlanEditor is null!")
	// 				}
	// 			}
	// 		}

	// 		// Test Mission Generation
	// 		QGCButton {
	// 			text: qsTr("Test Mission Generation")
	// 			width: parent.width
    //             height: _h * 1.5
	// 			onClicked: {
	// 				console.log("Test Mission Generation clicked")
	// 				if (areaPlanEditor) {
	// 					console.log("Current parameters:")
	// 					console.log("  areaWidth:", areaPlanEditor.areaWidth)
	// 					console.log("  areaHeight:", areaPlanEditor.areaHeight)
	// 					console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
	// 					console.log("  numPoints:", areaPlanEditor.numPoints)
	// 					console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
						
	// 					var waypoints = areaPlanEditor.generateWaypoints()
	// 					console.log("Generated waypoints:", waypoints.length)
						
	// 					areaPlanEditor.saveMissionFile()
	// 					console.log("Mission file saved")
	// 				} else {
	// 					console.log("ERROR: areaPlanEditor is null!")
	// 				}
	// 			}
	// 		}

	// 		// Debug: Force Map Items
	// 		QGCButton {
	// 			text: qsTr("Debug: Force Map Items")
	// 			width: parent.width
    //             height: _h * 1.5
	// 			onClicked: {
	// 				console.log("Debug: Force map items clicked")
	// 				if (areaPlanEditor) {
	// 					console.log("Current area state:")
	// 					console.log("  areaWidth:", areaPlanEditor.areaWidth)
	// 					console.log("  areaHeight:", areaPlanEditor.areaHeight)
	// 					console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
	// 					console.log("  areaCenter valid:", areaPlanEditor.areaCenter.isValid)
						
	// 					// Force property changes to trigger signals
	// 					var currentWidth = areaPlanEditor.areaWidth
	// 					var currentHeight = areaPlanEditor.areaHeight
						
	// 					// Temporarily change and restore to trigger signals
	// 					areaPlanEditor.setAreaWidth(currentWidth + 0.1)
	// 					areaPlanEditor.setAreaWidth(currentWidth)
	// 					areaPlanEditor.setAreaHeight(currentHeight + 0.1)
	// 					areaPlanEditor.setAreaHeight(currentHeight)
						
	// 					console.log("Forced property updates completed")
	// 				} else {
	// 					console.log("ERROR: areaPlanEditor is null!")
	// 				}
	// 			}
	// 		}
	// 	}
	}
}
