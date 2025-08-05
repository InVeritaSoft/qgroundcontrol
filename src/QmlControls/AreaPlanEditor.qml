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
				spacing: 20
				anchors.margins: 20
				
				// Header
				QGCLabel {
					text: qsTr("Area Plan Mission Editor")
					font.pointSize: ScreenTools.largeFontPointSize
					font.bold: true
					width: parent.width
					height: 40
					wrapMode: Text.WordWrap
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}

				// Area Configuration Section
				Rectangle {
					width: parent.width
					height: areaConfigColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: areaConfigColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 16

						QGCLabel {
							text: qsTr("Area Configuration")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						Grid {
							columns: 2
							width: parent.width
							rowSpacing: 12
							columnSpacing: 20

							QGCLabel { 
								text: qsTr("Area Width (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: widthTextField
								text: areaPlanEditor ? (areaPlanEditor.areaWidth * 10).toString() : "300"
								width: parent.width * 0.5
								height: 32
								validator: DoubleValidator {
									bottom: 10
									top: 10000
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.areaWidth = parseFloat(text) / 10
									}
								}
							}

							QGCLabel { 
								text: qsTr("Area Height (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: heightTextField
								text: areaPlanEditor ? (areaPlanEditor.areaHeight * 10).toString() : "900"
								width: parent.width * 0.5
								height: 32
								validator: DoubleValidator {
									bottom: 10
									top: 10000
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.areaHeight = parseFloat(text) / 10
									}
								}
							}

							QGCLabel { 
								text: qsTr("Line Spacing (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: lineSpacingTextField
								text: areaPlanEditor ? (areaPlanEditor.lineSpacing * 10).toString() : "30"
								width: parent.width * 0.5
								height: 32
								validator: DoubleValidator {
									bottom: 1
									top: 500
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.lineSpacing = parseFloat(text) / 10
									}
								}
							}

							QGCLabel { 
								text: qsTr("Points per Line:")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: numPointsTextField
								text: areaPlanEditor ? areaPlanEditor.numPoints.toString() : "1"
								width: parent.width * 0.5
								height: 32
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
								text: qsTr("Mission Altitude (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							QGCTextField {
								id: altitudeTextField
								text: areaPlanEditor ? (areaPlanEditor.missionAltitude * 10).toString() : "100"
								width: parent.width * 0.5
								height: 32
								validator: DoubleValidator {
									bottom: 10
									top: 1000
									decimals: 1
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										areaPlanEditor.missionAltitude = parseFloat(text) / 10
									}
								}
							}
						}
					}
				}

				// Interactive Drawing Controls
				Rectangle {
					width: parent.width
					height: drawingColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: drawingColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 16

						QGCLabel {
							text: qsTr("Interactive Drawing")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						QGCButton {
							id: drawingModeButton
							text: {
								if (!areaPlanEditor) return qsTr("Start Drawing Mode")
								return areaPlanEditor.isDrawingMode ? qsTr("Stop Drawing Mode") : qsTr("Start Drawing Mode")
							}
							width: parent.width
							height: 44
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
							text: qsTr("Test C++ Backend")
							width: parent.width
							height: 30
							onClicked: {
								console.log("Test button clicked")
								if (areaPlanEditor) {
									console.log("areaPlanEditor is valid")
									console.log("areaWidth:", areaPlanEditor.areaWidth)
									console.log("areaHeight:", areaPlanEditor.areaHeight)
									console.log("isDrawingMode:", areaPlanEditor.isDrawingMode)
									
									// Set reasonable defaults if they're 0
									if (areaPlanEditor.areaWidth <= 0) {
										areaPlanEditor.setAreaWidth(100.0)
										console.log("Set default areaWidth to 100")
									}
									if (areaPlanEditor.areaHeight <= 0) {
										areaPlanEditor.setAreaHeight(100.0)
										console.log("Set default areaHeight to 100")
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
							text: qsTr("Test Mission Generation")
							width: parent.width
							height: 30
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
							text: qsTr("Debug: Force Map Items")
							width: parent.width
							height: 30
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
							text: qsTr("Test Re-centering")
							width: parent.width
							height: 30
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
							text: qsTr("Reset Area")
							width: parent.width
							height: 30
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
							text: qsTr("Instructions:")
							font.pointSize: ScreenTools.smallFontPointSize
							font.bold: true
							width: parent.width
							height: 20
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("1. Click 'Start Drawing Mode'\n2. Click on map to set center\n3. Drag to resize area\n4. Double-click to finish")
							font.pointSize: ScreenTools.smallFontPointSize
							width: parent.width
							height: 60
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
						}
						
						// Status indicator
						Rectangle {
							width: parent.width
							height: 30
							color: areaPlanEditor && areaPlanEditor.isDrawingMode ? "#40FF0000" : "#4000FF00"
							radius: 4
							border.color: areaPlanEditor && areaPlanEditor.isDrawingMode ? "#FF0000" : "#00FF00"
							border.width: 1
							
							QGCLabel {
								anchors.centerIn: parent
								text: areaPlanEditor && areaPlanEditor.isDrawingMode ? qsTr("DRAWING MODE ACTIVE") : qsTr("Drawing mode ready")
								font.pointSize: ScreenTools.smallFontPointSize
								font.bold: areaPlanEditor && areaPlanEditor.isDrawingMode
								color: areaPlanEditor && areaPlanEditor.isDrawingMode ? "#FF0000" : "#00FF00"
							}
						}
						
						// Step-by-step flow indicator
						Rectangle {
							width: parent.width
							height: stepFlowColumn.height + 20
							color: qgcPal.windowShadeDark
							radius: 4
							border.color: qgcPal.colorGrey
							border.width: 1
							
							Column {
								id: stepFlowColumn
								anchors.left: parent.left
								anchors.right: parent.right
								anchors.top: parent.top
								anchors.margins: 10
								spacing: 8
								
								QGCLabel {
									text: qsTr("Step-by-Step Flow:")
									font.pointSize: ScreenTools.smallFontPointSize
									font.bold: true
									color: qgcPal.text
								}
								
								// Step 1: Set Center
								Row {
									width: parent.width
									height: 20
									spacing: 8
									
									Rectangle {
										width: 16
										height: 16
										radius: 8
										color: areaPlanEditor && areaPlanEditor.areaCenter.isValid ? "#00FF00" : "#808080"
										border.color: "#FFFFFF"
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("1. Set Center Point")
										font.pointSize: ScreenTools.smallFontPointSize
										color: areaPlanEditor && areaPlanEditor.areaCenter.isValid ? "#00FF00" : "#808080"
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 2: Define Area
								Row {
									width: parent.width
									height: 20
									spacing: 8
									
									Rectangle {
										width: 16
										height: 16
										radius: 8
										color: areaPlanEditor && areaPlanEditor.areaWidth > 0 && areaPlanEditor.areaHeight > 0 ? "#00FF00" : "#808080"
										border.color: "#FFFFFF"
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("2. Define Area Size")
										font.pointSize: ScreenTools.smallFontPointSize
										color: areaPlanEditor && areaPlanEditor.areaWidth > 0 && areaPlanEditor.areaHeight > 0 ? "#00FF00" : "#808080"
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 3: Generate Mission
								Row {
									width: parent.width
									height: 20
									spacing: 8
									
									Rectangle {
										width: 16
										height: 16
										radius: 8
										color: areaPlanEditor && areaPlanEditor.numPoints > 0 ? "#00FF00" : "#808080"
										border.color: "#FFFFFF"
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("3. Generate Mission")
										font.pointSize: ScreenTools.smallFontPointSize
										color: areaPlanEditor && areaPlanEditor.numPoints > 0 ? "#00FF00" : "#808080"
										anchors.verticalCenter: parent.verticalCenter
									}
								}
								
								// Step 4: Save Mission
								Row {
									width: parent.width
									height: 20
									spacing: 8
									
									Rectangle {
										width: 16
										height: 16
										radius: 8
										color: "#808080"  // Always grey for now
										border.color: "#FFFFFF"
										border.width: 1
									}
									
									QGCLabel {
										text: qsTr("4. Upload to Vehicle (Optional)")
										font.pointSize: ScreenTools.smallFontPointSize
										color: "#808080"
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
					height: movementColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: movementColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 16

						QGCLabel {
							text: qsTr("Area Position")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						Item {
							width: parent.width
							height: 120
							
							Grid {
								anchors.centerIn: parent
								columns: 3
								rowSpacing: 8
								columnSpacing: 8

								Item { width: 50; height: 40 }
								QGCButton {
									text: qsTr("↑")
									width: 50
									height: 40
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaNorth()
								}
								Item { width: 50; height: 40 }

								QGCButton {
									text: qsTr("←")
									width: 50
									height: 40
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaWest()
								}
								QGCButton {
									text: qsTr("Center")
									width: 100
									height: 40
									onClicked: if (areaPlanEditor) areaPlanEditor.centerArea()
								}
								QGCButton {
									text: qsTr("→")
									width: 50
									height: 40
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaEast()
								}

								Item { width: 50; height: 40 }
								QGCButton {
									text: qsTr("↓")
									width: 50
									height: 40
									onClicked: if (areaPlanEditor) areaPlanEditor.moveAreaSouth()
								}
								Item { width: 50; height: 40 }
							}
						}
					}
				}

				// Rotation Controls Section
				Rectangle {
					width: parent.width
					height: rotationControlsColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: rotationControlsColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 12

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
							height: 32
							spacing: 10

							QGCLabel {
								text: qsTr("Current Rotation:")
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
							height: 32
							spacing: 10

							QGCLabel {
								text: qsTr("Set Rotation:")
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
								text: qsTr("degrees")
								width: parent.width * 0.3
								height: parent.height
								verticalAlignment: Text.AlignVCenter
								color: qgcPal.colorGrey
							}
						}

						// Rotation buttons
						Row {
							width: parent.width
							height: 40
							spacing: 10

							QGCButton {
								text: qsTr("↺ -15°")
								width: parent.width * 0.3
								height: parent.height
								onClicked: if (areaPlanEditor) areaPlanEditor.rotateAreaCounterClockwise()
							}

							QGCButton {
								text: qsTr("Reset to 0°")
								width: parent.width * 0.4
								height: parent.height
								onClicked: if (areaPlanEditor) areaPlanEditor.setAreaRotation(0.0)
							}

							QGCButton {
								text: qsTr("+15° ↻")
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
					height: missionControlsColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: missionControlsColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 12

						QGCLabel {
							text: qsTr("Mission Controls")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Generate waypoints from the current area plan and add them to the Mission Tab. Works with or without a connected vehicle.")
							width: parent.width
							height: 40
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
							font.pointSize: ScreenTools.smallFontPointSize
							color: qgcPal.colorGrey
						}

						QGCButton {
							text: qsTr("Generate Mission & Add to Mission Tab")
							width: parent.width
							height: 44
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
							height: 44
							onClicked: if (areaPlanEditor) areaPlanEditor.saveMissionFile()
						}

						QGCButton {
							text: qsTr("Upload to Vehicle")
							width: parent.width
							height: 44
							enabled: false // TODO: Implement vehicle connection
							onClicked: if (areaPlanEditor) areaPlanEditor.uploadToVehicle()
						}

						QGCButton {
							text: qsTr("Start Mission")
							width: parent.width
							height: 44
							enabled: false // TODO: Implement mission start
							onClicked: if (areaPlanEditor) areaPlanEditor.startMission()
						}
					}
				}

				// Mission Statistics
				Rectangle {
					width: parent.width
					height: statsColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: statsColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 10

						QGCLabel {
							text: qsTr("Mission Statistics")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Area Size: %1m x %2m").arg(areaPlanEditor ? areaPlanEditor.areaWidth : 30).arg(areaPlanEditor ? areaPlanEditor.areaHeight : 90)
							width: parent.width
							height: 20
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Total Waypoints: %1").arg(areaPlanEditor ? areaPlanEditor.calculateTotalWaypoints() : 0)
							width: parent.width
							height: 20
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Estimated Flight Time: %1 min").arg(areaPlanEditor ? areaPlanEditor.calculateFlightTime() : 0)
							width: parent.width
							height: 20
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							text: qsTr("Area Center: %1, %2").arg(areaPlanEditor && areaPlanEditor.areaCenter ? areaPlanEditor.areaCenter.latitude.toFixed(6) : "0.000000").arg(areaPlanEditor && areaPlanEditor.areaCenter ? areaPlanEditor.areaCenter.longitude.toFixed(6) : "0.000000")
							width: parent.width
							height: 40
							wrapMode: Text.WordWrap
							verticalAlignment: Text.AlignTop
						}
					}
				}

				// Status Section
				Rectangle {
					width: parent.width
					height: statusColumn.height + 40
					color: qgcPal.windowShade
					radius: 8
					
					Column {
						id: statusColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: 20
						spacing: 10

						QGCLabel {
							text: qsTr("Status")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: 24
							verticalAlignment: Text.AlignVCenter
						}

						QGCLabel {
							id: statusLabel
							text: qsTr("Ready to generate mission")
							color: qgcPal ? qgcPal.text : "white"
							width: parent.width
							height: 40
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
		height: debugColumn.height + 40
		color: qgcPal.windowShade
		radius: 8
		
		Column {
			id: debugColumn
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.margins: 20
			spacing: 10

			QGCLabel {
				text: qsTr("Debug Tools")
				font.pointSize: ScreenTools.mediumFontPointSize
				font.bold: true
				width: parent.width
				height: 24
				verticalAlignment: Text.AlignVCenter
			}

			// Test C++ Backend
			QGCButton {
				text: qsTr("Test C++ Backend")
				width: parent.width
				height: 30
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
							areaPlanEditor.setAreaWidth(100.0)
							console.log("Set areaWidth to 100.0")
						}
						if (areaPlanEditor.areaHeight <= 0) {
							areaPlanEditor.setAreaHeight(100.0)
							console.log("Set areaHeight to 100.0")
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
				height: 30
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
				height: 30
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