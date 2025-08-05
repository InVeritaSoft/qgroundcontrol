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
		areaPlanEditor = QGroundControl.areaPlanEditor
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
							SpinBox {
								id: widthSpinBox
								from: 10
								to: 10000
								value: areaPlanEditor ? areaPlanEditor.areaWidth * 10 : 300
								stepSize: 1
								width: parent.width * 0.5
								height: 32
								onValueChanged: if (areaPlanEditor) areaPlanEditor.areaWidth = value / 10
							}

							QGCLabel { 
								text: qsTr("Area Height (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							SpinBox {
								id: heightSpinBox
								from: 10
								to: 10000
								value: areaPlanEditor ? areaPlanEditor.areaHeight * 10 : 900
								stepSize: 1
								width: parent.width * 0.5
								height: 32
								onValueChanged: if (areaPlanEditor) areaPlanEditor.areaHeight = value / 10
							}

							QGCLabel { 
								text: qsTr("Line Spacing (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							SpinBox {
								id: lineSpacingSpinBox
								from: 1
								to: 500
								value: areaPlanEditor ? areaPlanEditor.lineSpacing * 10 : 30
								stepSize: 1
								width: parent.width * 0.5
								height: 32
								onValueChanged: if (areaPlanEditor) areaPlanEditor.lineSpacing = value / 10
							}

							QGCLabel { 
								text: qsTr("Points per Line:")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							SpinBox {
								id: numPointsSpinBox
								from: 1
								to: 50
								value: areaPlanEditor ? areaPlanEditor.numPoints : 1
								width: parent.width * 0.5
								height: 32
								onValueChanged: if (areaPlanEditor) areaPlanEditor.numPoints = value
							}

							QGCLabel { 
								text: qsTr("Mission Altitude (m):")
								width: parent.width * 0.4
								height: 32
								verticalAlignment: Text.AlignVCenter
							}
							SpinBox {
								id: altitudeSpinBox
								from: 10
								to: 1000
								value: areaPlanEditor ? areaPlanEditor.missionAltitude * 10 : 100
								stepSize: 5
								width: parent.width * 0.5
								height: 32
								onValueChanged: if (areaPlanEditor) areaPlanEditor.missionAltitude = value / 10
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

						QGCButton {
							text: qsTr("Generate Waypoints")
							width: parent.width
							height: 44
							onClicked: if (areaPlanEditor) areaPlanEditor.generateWaypoints()
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
	}
}