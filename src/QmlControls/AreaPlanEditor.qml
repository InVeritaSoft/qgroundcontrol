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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
	id: _root

	// Reference to the C++ backend
	property var areaPlanEditor: null
    // Per-drone preview data: array of { droneIndex, altitudeOffsetM, timeOffsetS, waypoints[] }
    property var waypointPreview: []

    // Sizing helpers (no hardcoded sizes)
    readonly property real _h: ScreenTools.defaultFontPixelHeight
    readonly property real _w: ScreenTools.defaultFontPixelWidth


	Component.onCompleted: {
		console.log("AreaPlanEditor: Component completed")
		areaPlanEditor = QGroundControl.areaPlanEditor
		console.log("AreaPlanEditor backend:", !!areaPlanEditor)
		if (areaPlanEditor) {
			console.log("AreaPlanEditor properties:")
			console.log("  areaWidth:", areaPlanEditor.areaWidth)
			console.log("  areaHeight:", areaPlanEditor.areaHeight)
			console.log("  isDrawingMode:", areaPlanEditor.isDrawingMode)
		}
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

							QGCLabel { 
								text: qsTr("Area Width (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: widthTextField
								text: areaPlanEditor ? areaPlanEditor.areaWidth.toString() : "10"
								width: parent.width * 0.5
                                height: _h * 1.6
								validator: DoubleValidator {
									bottom: 1
									top: 1000
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.areaWidth = parseFloat(text)
									}
								}
							}

                            // --- Multi-drone parameters ---
                            QGCLabel {
                                text: qsTr("Number of Drones:")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                text: areaPlanEditor ? areaPlanEditor.droneCount.toString() : "2"
                                validator: IntValidator { bottom: 1; top: 50 }
                                onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setDroneCount(parseInt(text))
                            }

                            QGCLabel {
                                text: qsTr("Altitude Band Start (m):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                text: areaPlanEditor ? areaPlanEditor.altitudeBandStart.toString() : "0"
                                validator: DoubleValidator { bottom: 0; top: 10000; decimals: 1 }
                                onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setAltitudeBandStart(parseFloat(text))
                            }

                            QGCLabel {
                                text: qsTr("Altitude Band Step (m):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                text: areaPlanEditor ? areaPlanEditor.altitudeBandStep.toString() : "10"
                                validator: DoubleValidator { bottom: 0.1; top: 10000; decimals: 1 }
                                onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setAltitudeBandStep(parseFloat(text))
                            }

                            QGCLabel {
                                text: qsTr("Time Offset per Drone (s):")
                                width: parent.width * 0.4
                                height: _h * 1.6
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                width: parent.width * 0.5
                                height: _h * 1.6
                                text: areaPlanEditor ? areaPlanEditor.timeOffsetPerDrone.toString() : "0"
                                validator: DoubleValidator { bottom: 0; top: 3600; decimals: 1 }
                                onEditingFinished: if (areaPlanEditor && text !== "") areaPlanEditor.setTimeOffsetPerDrone(parseFloat(text))
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
								text: qsTr("Area Height (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: heightTextField
								text: areaPlanEditor ? areaPlanEditor.areaHeight.toString() : "10"
								width: parent.width * 0.5
                                height: _h * 1.6
								validator: DoubleValidator {
									bottom: 1
									top: 1000
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.areaHeight = parseFloat(text)
									}
								}
							}

							QGCLabel { 
								text: qsTr("Line Spacing (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: lineSpacingTextField
								text: areaPlanEditor ? areaPlanEditor.lineSpacing.toString() : "10"
								width: parent.width * 0.5
                                height: _h * 1.6
								validator: DoubleValidator {
									bottom: 1
									top: 500
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.lineSpacing = parseFloat(text)
									}
								}
							}

							QGCLabel { 
								text: qsTr("Waypoints Per Line:")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: numPointsTextField
								text: areaPlanEditor ? areaPlanEditor.numPoints.toString() : "1"
								width: parent.width * 0.5
                                height: _h * 1.6
								validator: IntValidator {
									bottom: 1
									top: 50
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.numPoints = parseInt(text)
									}
								}
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

                        // Per-Drone Mission Insertion (non-aggregated)
                        Row {
                            width: parent.width
                            height: _h * 2
                            spacing: _w

                            QGCLabel {
                                text: qsTr("Insert Drone # to Mission:")
                                width: parent.width * 0.45
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            QGCTextField {
                                id: droneIndexField
                                width: parent.width * 0.2
                                height: parent.height
                                text: "0"
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
                                            text: qsTr("Drone %1").arg(modelData.droneIndex)
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
    Rectangle {
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		anchors.right: parent.right
        height: debugColumn.height + _h * 2
        color: qgcPal.windowShade
        radius: _w * 0.5
		
		Column {
			id: debugColumn
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
            anchors.margins: _h
            spacing: _h * 0.5

			QGCLabel {
				text: qsTr("Debug Tools")
				font.pointSize: ScreenTools.mediumFontPointSize
				font.bold: true
				width: parent.width
                height: _h * 1.2
				verticalAlignment: Text.AlignVCenter
			}

			// Test C++ Backend
			QGCButton {
				text: qsTr("Test C++ Backend")
				width: parent.width
                height: _h * 1.5
				onClicked: {
					console.log("Test C++ Backend clicked")
					if (areaPlanEditor) {
						console.log("C++ Backend is accessible!")
						console.log("Current properties:")
						console.log("  areaWidth:", areaPlanEditor.areaWidth)
						console.log("  areaHeight:", areaPlanEditor.areaHeight)
						console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
						console.log("  numPoints:", areaPlanEditor.numPoints)
						console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
						console.log("  isDrawingMode:", areaPlanEditor.isDrawingMode)
						
						// Set reasonable defaults if values are zero
						if (areaPlanEditor.areaWidth <= 0) {
							areaPlanEditor.setAreaWidth(10.0)
							console.log("Set areaWidth to 10.0")
						}
						if (areaPlanEditor.areaHeight <= 0) {
							areaPlanEditor.setAreaHeight(10.0)
							console.log("Set areaHeight to 10.0")
						}
						if (areaPlanEditor.lineSpacing <= 0) {
							areaPlanEditor.setLineSpacing(10.0)
							console.log("Set lineSpacing to 10.0")
						}
						if (areaPlanEditor.numPoints <= 0) {
							areaPlanEditor.setNumPoints(1)
							console.log("Set numPoints to 1")
						}
						
						console.log("Updated properties:")
						console.log("  areaWidth:", areaPlanEditor.areaWidth)
						console.log("  areaHeight:", areaPlanEditor.areaHeight)
						console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
						console.log("  numPoints:", areaPlanEditor.numPoints)
					} else {
						console.log("ERROR: areaPlanEditor is null!")
					}
				}
			}

			// Test Mission Generation
			QGCButton {
				text: qsTr("Test Mission Generation")
				width: parent.width
                height: _h * 1.5
				onClicked: {
					console.log("Test Mission Generation clicked")
					if (areaPlanEditor) {
						console.log("Current parameters:")
						console.log("  areaWidth:", areaPlanEditor.areaWidth)
						console.log("  areaHeight:", areaPlanEditor.areaHeight)
						console.log("  lineSpacing:", areaPlanEditor.lineSpacing)
						console.log("  numPoints:", areaPlanEditor.numPoints)
						console.log("  areaCenter:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)
						
						var waypoints = areaPlanEditor.generateWaypoints()
						console.log("Generated waypoints:", waypoints.length)
						
						areaPlanEditor.saveMissionFile()
						console.log("Mission file saved")
					} else {
						console.log("ERROR: areaPlanEditor is null!")
					}
				}
			}

			// Debug: Force Map Items
			QGCButton {
				text: qsTr("Debug: Force Map Items")
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
						
						// Force property changes to trigger signals
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
		}
	}
}