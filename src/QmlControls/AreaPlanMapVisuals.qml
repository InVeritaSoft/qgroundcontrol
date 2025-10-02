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
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// AreaPlanMapVisuals provides map visualization for the AreaPlanEditor
Item {
	id: _root

	property var mapControl                                  ///< Map control to place item in
	property var areaPlanEditor                              ///< AreaPlanEditor object
	property bool interactive: true
    property color interiorColor: (qgcPal && qgcPal.windowShadeDark) ? qgcPal.windowShadeDark : "#303030"
    property color borderColor: (qgcPal && qgcPal.buttonHighlight) ? qgcPal.buttonHighlight : "#2196F3"
    property int borderWidth: Math.max(1, Math.round(ScreenTools.defaultFontPixelWidth * 0.4))
	property real interiorOpacity: 0.7

	// Z-order management following QGC patterns
	property real _zorderRectangle:     QGroundControl.zOrderMapItems
	property real _zorderLines:         QGroundControl.zOrderMapItems + 1
	property real _zorderPoints:        QGroundControl.zOrderMapItems + 2
	property real _zorderCenterMarker:  QGroundControl.zOrderMapItems + 3

	// Interactive drawing properties
	property bool isDrawingMode: areaPlanEditor ? areaPlanEditor.isDrawingMode : false
	property bool showGridLines: true
	property bool showWaypoints: true
    property bool isDragging: false
    // Waypoint visualization colors
    readonly property var droneColors: [
        "#FF9800",  // orange
        "#1E88E5",  // blue
        "#8E24AA",  // purple
        "#43A047",  // green
        "#E53935",  // red
        "#00ACC1",  // cyan
        "#FDD835",  // yellow
        "#EC407A"   // pink
    ]
    readonly property color waypointBorderColor: "#FFFFFF"  // white
    readonly property real waypointBaseSize: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real waypointLabelSize: ScreenTools.defaultFontPixelHeight * 0.8

    // Altitude-band visual mapping helpers
    function _altitudeColor(offset) {
        // Positive offset -> blue band, negative -> red, near zero -> light gray
        if (offset === undefined || offset === null) return "#BDBDBD";
        if (offset > 0.5) return "#1E88E5";       // blue for higher band
        if (offset < -0.5) return "#E53935";      // red for lower band
        return "#BDBDBD";                         // neutral
    }
    function _altitudeThickness(offset) {
        // Thickness from 1..5 px based on magnitude (per 3 m)
        var mag = Math.abs(offset || 0);
        var t = 1 + Math.min(4, Math.floor(mag / 3));
        return Math.max(1, t);
    }

    function _makePreviewKey() {
        if (!areaPlanEditor) return "";
        var c = areaPlanEditor.areaCenter || QtPositioning.coordinate();
        var w = areaPlanEditor.areaWidth || 0;
        var h = areaPlanEditor.areaHeight || 0;
        var s = areaPlanEditor.lineSpacing || 0;
        var r = areaPlanEditor.areaRotation || 0;
        var n = areaPlanEditor.numPoints || 0;
        var d = areaPlanEditor.droneCount || 0;
        return [c.latitude.toFixed(7), c.longitude.toFixed(7), w.toFixed(3), h.toFixed(3), s.toFixed(3), r.toFixed(3), n, d].join("|");
    }

    function _updatePreviewIfChanged() {
        if (!(areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview)) return;
        var key = _makePreviewKey();
        if (key === _lastPreviewKey && _lastPreviewData && _lastPreviewData.length === perDronePreview.length) {
            return; // unchanged
        }
        var t0 = Date.now();
        var data = areaPlanEditor.computePerDroneWaypointPreview();
        _lastPreviewKey = key;
        _lastPreviewData = data;
        perDronePreview = data;
        // Ensure droneVisibility matches length and initialize missing entries to true
        if (!droneVisibility || droneVisibility.length !== (perDronePreview ? perDronePreview.length : 0)) {
            var n = perDronePreview ? perDronePreview.length : 0;
            var vis = [];
            for (var i = 0; i < n; i++) vis.push(true);
            droneVisibility = vis;
        }
        console.log("AreaPlanMapVisuals: preview recomputed in", (Date.now()-t0), "ms; drones:", perDronePreview.length);
    }

	Component.onCompleted: {
		console.log("AreaPlanMapVisuals: Component completed")
		console.log("AreaPlanMapVisuals: interactive:", interactive)
		console.log("AreaPlanMapVisuals: isDrawingMode:", isDrawingMode)
		console.log("AreaPlanMapVisuals: areaPlanEditor:", areaPlanEditor)
		console.log("AreaPlanMapVisuals: mapControl:", mapControl)
		
		// Initial creation of map items
		addMapItems()
	}

    // Object managers following QGC patterns
	QGCDynamicObjectManager { id: _objMgrRectangle }
	QGCDynamicObjectManager { id: _objMgrCenterMarker }
	QGCDynamicObjectManager { id: _objMgrGridLines }
    QGCDynamicObjectManager { id: _objMgrWaypointMarkers }
    QGCDynamicObjectManager { id: _objMgrPerDroneGrid }
    QGCDynamicObjectManager { id: _objMgrPerDroneMarkers }

    // Debounce Timer for overlays
    Timer {
        id: _overlayDebounce
        interval: 120
        repeat: false
        onTriggered: addPerDroneOverlays()
    }

    // Reuse cache for per-drone marker objects to avoid churn
    property var _perDroneMarkerObjects: [] // array of arrays of MapQuickItem
    property var _perDroneGridObjects: [] // array of arrays of MapPolyline

    // Preview data and visibility controls
    property var perDronePreview: [] // array of {droneIndex, altitudeOffsetM, timeOffsetS, waypoints[]}
    property var droneVisibility: [] // bool per drone
    property var _seriesColors: droneColors
    // Cache keys for preview change detection
    property string _lastPreviewKey: ""
    property var _lastPreviewData: null

    // Diagnostic counters for tests/validation
    property int lastGridLineCount: 0
    property int lastWaypointMarkerCount: 0
    property int lastPerDroneMarkerCount: 0
    property int lastRectangleCornerCount: 0

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

	// Calculate rectangle corners based on area parameters
	property var rectangleCorners: {
		if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.areaWidth || !areaPlanEditor.areaHeight) {
			return []
		}

		var center = areaPlanEditor.areaCenter
		var width = areaPlanEditor.areaWidth
		var height = areaPlanEditor.areaHeight
		var rotation = areaPlanEditor.areaRotation || 0.0

		var corners = []

		try {
			// Validate inputs
			if (!center.isValid) {
				console.log("AreaPlanMapVisuals: ERROR - Invalid center coordinate")
				return []
			}

			if (width <= 0 || height <= 0) {
				console.log("AreaPlanMapVisuals: ERROR - Invalid dimensions:", width, "x", height)
				return []
			}

			// Calculate corners using geodesic calculations with rotation
			var halfWidth = width / 2
			var halfHeight = height / 2

			var cornerDistance = Math.sqrt(halfWidth * halfWidth + halfHeight * halfHeight)

			var angleNW = Math.atan2(-halfHeight, -halfWidth) * 180 / Math.PI  // North-West
			var angleNE = Math.atan2(-halfHeight, halfWidth) * 180 / Math.PI   // North-East
			var angleSE = Math.atan2(halfHeight, halfWidth) * 180 / Math.PI    // South-East
			var angleSW = Math.atan2(halfHeight, -halfWidth) * 180 / Math.PI   // South-West

			var northWest = areaPlanEditor.calculateOffsetCoordinate(center, cornerDistance, rotation + angleNW)
			var northEast = areaPlanEditor.calculateOffsetCoordinate(center, cornerDistance, rotation + angleNE)
			var southEast = areaPlanEditor.calculateOffsetCoordinate(center, cornerDistance, rotation + angleSE)
			var southWest = areaPlanEditor.calculateOffsetCoordinate(center, cornerDistance, rotation + angleSW)

			console.log("AreaPlanMapVisuals: North-West:", northWest.latitude, northWest.longitude)
			console.log("AreaPlanMapVisuals: North-East:", northEast.latitude, northEast.longitude)
			console.log("AreaPlanMapVisuals: South-East:", southEast.latitude, southEast.longitude)
			console.log("AreaPlanMapVisuals: South-West:", southWest.latitude, southWest.longitude)

			// Add corners in clockwise order starting from top-left
			corners.push(northWest)
			corners.push(northEast)
			corners.push(southEast)
			corners.push(southWest)

			console.log("AreaPlanMapVisuals: Calculated", corners.length, "corners")
			if (corners.length > 0) {
				console.log("First corner:", corners[0].latitude, corners[0].longitude)
				console.log("Last corner:", corners[corners.length-1].latitude, corners[corners.length-1].longitude)

				// Validate all corners
				for (var i = 0; i < corners.length; i++) {
					if (!corners[i].isValid) {
						console.log("AreaPlanMapVisuals: ERROR - Invalid corner at index", i)
						return []
					}
				}
				console.log("AreaPlanMapVisuals: All corners are valid")
			}
		} catch (e) {
			console.log("Error calculating rectangle corners:", e)
			return []
		}

		return corners
	}

	// Component for the area rectangle polygon
	Component {
		id: areaRectangleComponent

		MapPolygon {
			id: missionShape
			path: rectangleCorners
			color: interiorColor
			border.color: borderColor
			border.width: borderWidth
			opacity: interiorOpacity
			visible: areaPlanEditor && areaPlanEditor.areaCenter && areaPlanEditor.areaCenter.isValid
			z: _zorderRectangle

			// Debug: Add console logging to track polygon updates
			onPathChanged: {
				console.log("AreaPlanMapVisuals: Polygon path changed, corners count:", path.length)
				if (path.length > 0) {
					console.log("First corner:", path[0].latitude, path[0].longitude)
				}
			}

			// Smooth opacity transition for real-time feedback
			Behavior on opacity {
				NumberAnimation { duration: 150 }
			}

			// MouseArea for dragging the rectangle
			MouseArea {
				id: rectangleMouseArea
				anchors.fill: parent
				enabled: false  // Disabled since we have map area MouseArea handling everything
				hoverEnabled: true
				preventStealing: true
				z: 1000  // High z-order to ensure it receives events

				property point startPos
				property var startCenter
				property var startCoordinate
				property bool hasMoved: false

				Component.onCompleted: {
					console.log("Rectangle MouseArea completed - enabled:", enabled, "interactive:", interactive)
				}

				onPressed: {
					console.log("Rectangle pressed - setting isDragging to true")
					console.log("Rectangle MouseArea - enabled:", enabled, "interactive:", interactive)
					startPos = Qt.point(mouse.x, mouse.y)
					startCenter = areaPlanEditor ? areaPlanEditor.areaCenter : null
					isDragging = true
					hasMoved = false
					console.log("Rectangle isDragging set to:", isDragging)
					// Get the coordinate at the press position
					if (mapControl) {
						startCoordinate = mapControl.toCoordinate(startPos, false)
						console.log("Rectangle drag started at coordinate:", startCoordinate.latitude, startCoordinate.longitude)
					}
				}

				onPositionChanged: {
					if (pressed && areaPlanEditor && mapControl) {
						hasMoved = true
						console.log("Rectangle dragging - isDragging:", isDragging)
						// Get current mouse position coordinate
						var currentPos = Qt.point(mouse.x, mouse.y)
						var currentCoordinate = mapControl.toCoordinate(currentPos, false)

						if (currentCoordinate.isValid) {
							// Move center directly to current mouse position
							console.log("Rectangle dragging to:", currentCoordinate.latitude, currentCoordinate.longitude)
							areaPlanEditor.setAreaCenter(currentCoordinate)
						}
					}
				}

				onReleased: {
					console.log("Rectangle released - setting isDragging to false")
					isDragging = false
					console.log("Rectangle isDragging set to:", isDragging)

					// If we didn't move, treat as a click
					if (!hasMoved && areaPlanEditor && mapControl) {
						var clickPos = Qt.point(mouse.x, mouse.y)
						var clickCoordinate = mapControl.toCoordinate(clickPos, false)

						if (clickCoordinate.isValid) {
							console.log("Rectangle clicked at:", clickCoordinate.latitude, clickCoordinate.longitude)

							// Set center point on first click or if center is not valid
							if (!areaPlanEditor.areaCenter.isValid ||
								(Math.abs(areaPlanEditor.areaCenter.latitude) < 0.001 && Math.abs(areaPlanEditor.areaCenter.longitude) < 0.001) ||
								areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {

								areaPlanEditor.setAreaCenter(clickCoordinate)
								console.log("Area center set to:", clickCoordinate.latitude, clickCoordinate.longitude)

								// Set default area size if not already set
								if (areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
									areaPlanEditor.setAreaWidth(10.0)
									areaPlanEditor.setAreaHeight(10.0)
									console.log("Set default area size: 10x10 meters")
								}
							} else {
								// Calculate new area size based on distance from center
								var center = areaPlanEditor.areaCenter
								var distance = center.distanceTo(clickCoordinate)
								var newWidth = Math.max(distance * 2, 10)
								var newHeight = Math.max(distance * 2, 10)

								// Limit maximum size
								newWidth = Math.min(newWidth, 1000)
								newHeight = Math.min(newHeight, 1000)

								areaPlanEditor.setAreaWidth(newWidth)
								areaPlanEditor.setAreaHeight(newHeight)
								console.log("Updated area size:", newWidth, "x", newHeight, "meters")
							}
						}
					}
				}

				onEntered: {
					parent.opacity = 0.9
				}

				onExited: {
					parent.opacity = interiorOpacity
				}
			}
		}
	}

	// Component for the center marker
	Component {
		id: centerMarkerComponent

		MapQuickItem {
			id: centerMarker
			coordinate: areaPlanEditor ? areaPlanEditor.areaCenter : QtPositioning.coordinate()
			z: _zorderCenterMarker
			anchorPoint.x: sourceItem.width / 2
			anchorPoint.y: sourceItem.height / 2

			sourceItem: Rectangle {
				id: centerMarkerRect
				width: ScreenTools.defaultFontPixelHeight * 3.0
				height: width
				radius: width / 2
				color: "#FFFF0000"
				border.color: "#FFFFFFFF"
				border.width: 4

				// Add a cross inside to make it more visible
				Rectangle {
					anchors.centerIn: parent
					width: parent.width * 0.5
					height: 4
					color: "#FFFFFFFF"
				}
				Rectangle {
					anchors.centerIn: parent
					width: 4
					height: parent.height * 0.5
					color: "#FFFFFFFF"
				}

				// Smooth scale animation
				Behavior on scale {
					NumberAnimation { duration: 150 }
				}

				// MouseArea for dragging the center marker
				MouseArea {
					id: centerMouseArea
					anchors.fill: parent
					enabled: false  // Disabled since we have map area MouseArea handling everything
					hoverEnabled: true
					preventStealing: true

					property point startPos
					property var startCenter
					property var startCoordinate

					Component.onCompleted: {
						console.log("Center marker MouseArea completed - enabled:", enabled, "interactive:", interactive)
					}

					onPressed: {
						console.log("Center marker pressed - setting isDragging to true")
						console.log("Center marker MouseArea - enabled:", enabled, "interactive:", interactive)
						startPos = Qt.point(mouse.x, mouse.y)
						startCenter = areaPlanEditor ? areaPlanEditor.areaCenter : null
						isDragging = true
						console.log("Center marker isDragging set to:", isDragging)
						// Get the coordinate at the press position
						if (mapControl) {
							startCoordinate = mapControl.toCoordinate(startPos, false)
							console.log("Center marker drag started at coordinate:", startCoordinate.latitude, startCoordinate.longitude)
						}
					}

					onPositionChanged: {
						if (pressed && areaPlanEditor && mapControl) {
							console.log("Center marker dragging - isDragging:", isDragging)
							// Get current mouse position coordinate
							var currentPos = Qt.point(mouse.x, mouse.y)
							var currentCoordinate = mapControl.toCoordinate(currentPos, false)

							if (currentCoordinate.isValid) {
								// Move center directly to current mouse position
								console.log("Center marker dragging to:", currentCoordinate.latitude, currentCoordinate.longitude)
								areaPlanEditor.setAreaCenter(currentCoordinate)
							}
						}
					}

					onReleased: {
						console.log("Center marker released - setting isDragging to false")
						isDragging = false
						console.log("Center marker isDragging set to:", isDragging)
					}

					onEntered: {
						parent.scale = 1.2
					}

					onExited: {
						parent.scale = 1.0
					}
				}
			}
		}
	}

	// Component for grid lines
	Component {
		id: gridLineComponent

        MapPolyline {
			id: missionLines
            line.color: (qgcPal && qgcPal.colorGreen) ? qgcPal.colorGreen : "#43A047"
			line.width: Math.max(1, Math.round(ScreenTools.defaultFontPixelWidth * 0.5))
			z: _zorderLines
		}
	}

	// Component for waypoint markers
	Component {
		id: waypointMarkerComponent

		MapQuickItem {
			id: missionPoints
			z: _zorderPoints
			anchorPoint.x: sourceItem.width / 2
			anchorPoint.y: sourceItem.height / 2
			// Per-marker properties
			property int droneIndex: 0
			property real altitudeOffset: 0

			sourceItem: Item {
                width: waypointBaseSize
                height: width

                // Main waypoint circle
                Rectangle {
                    id: waypointCircle
                    anchors.fill: parent
                    radius: width / 2
                    color: droneColors[Math.min(missionPoints.droneIndex, droneColors.length - 1)]
                    border.color: waypointBorderColor
                    border.width: Math.max(2, Math.round(ScreenTools.defaultFontPixelWidth * 0.5))
                    opacity: 0.8

                    // Inner ring showing altitude band
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 0.6
                        height: width
                        radius: width / 2
                        color: "transparent"
                        border.color: _altitudeColor(missionPoints.altitudeOffset)
                        border.width: _altitudeThickness(missionPoints.altitudeOffset)
                        opacity: 0.8

                        // Altitude indicator
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width * 0.8
                            height: width
                            radius: width / 2
                            color: _altitudeColor(missionPoints.altitudeOffset)
                            opacity: 0.25
                        }
                    }

                    // Drone index label
                    Text {
                        anchors.centerIn: parent
                        text: (missionPoints.droneIndex).toString()
                        color: waypointBorderColor
                        font.pixelSize: waypointLabelSize
                        font.bold: true
                        style: Text.Outline
                        styleColor: "black"
                    }

                    // Smooth scale animation
                    Behavior on scale {
                        NumberAnimation { duration: 150 }
                    }

                    // Hover effect
                    states: State {
                        name: "hover"
                        PropertyChanges {
                            target: waypointCircle
                            scale: 1.2
                            opacity: 1.0
                        }
                    }
                }
            }
		}
	}

	// Calculate waypoint positions based on area parameters
	property var waypointPositions: {
		if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.lineSpacing || !areaPlanEditor.numPoints) {
			return []
		}

		var center = areaPlanEditor.areaCenter
		var width = areaPlanEditor.areaWidth
		var height = areaPlanEditor.areaHeight
		var lineSpacing = areaPlanEditor.lineSpacing
		var numPoints = areaPlanEditor.numPoints
		var rotation = areaPlanEditor.areaRotation || 0.0

		var waypoints = []

		try {
			// Calculate number of lines based on height and spacing
			var numLines = Math.max(1, Math.floor(height / lineSpacing))
			console.log("AreaPlanMapVisuals: Creating waypoints for", numLines, "lines with", numPoints, "points per line")
			console.log("AreaPlanMapVisuals: Area dimensions:", width, "x", height, "spacing:", lineSpacing, "rotation:", rotation)

			for (var i = 0; i < numLines; i++) {
				// Calculate offset from center for this line (perpendicular to rotation)
				var offset = (-height/2) + (i + 0.5) * lineSpacing

				// Calculate line center point (perpendicular to rotation direction)
				var lineCenter = areaPlanEditor.calculateOffsetCoordinate(center, offset, rotation + 180)
				console.log("AreaPlanMapVisuals: Line", i, "center at:", lineCenter.latitude, lineCenter.longitude, "offset:", offset)

				// Calculate waypoints along this line (parallel to rotation direction)
				for (var j = 0; j < numPoints; j++) {
					// Calculate position along the line (0 to 1)
					var fraction = (j + 0.5) / numPoints

					// Calculate offset from line center (-width/2 to width/2)
					var pointOffset = (fraction - 0.5) * width

					// Calculate waypoint position (parallel to rotation direction)
					var waypoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, pointOffset, rotation + 90)

					waypoints.push({
						coordinate: waypoint,
						lineIndex: i,
						pointIndex: j
					})

					console.log("AreaPlanMapVisuals: Waypoint", i, "-", j, "fraction:", fraction, "offset:", pointOffset, "at:", waypoint.latitude, waypoint.longitude)
				}
			}
		} catch (e) {
			console.log("Error calculating waypoint positions:", e)
			return []
		}

		return waypoints
	}

	// Calculate grid lines based on area parameters
	property var gridLines: {
		if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.lineSpacing) {
			return []
		}

		var center = areaPlanEditor.areaCenter
		var width = areaPlanEditor.areaWidth
		var height = areaPlanEditor.areaHeight
		var lineSpacing = areaPlanEditor.lineSpacing
		var rotation = areaPlanEditor.areaRotation || 0.0

		var lines = []

		try {
			// Calculate number of lines based on height and spacing
			var numLines = Math.max(1, Math.floor(height / lineSpacing))
			console.log("AreaPlanMapVisuals: Creating", numLines, "grid lines with rotation:", rotation)

			for (var i = 0; i < numLines; i++) {
				// Calculate offset from center for this line (perpendicular to rotation)
				var offset = (-height/2) + (i + 0.5) * lineSpacing

				// Calculate line center point (perpendicular to rotation direction)
				var lineCenter = areaPlanEditor.calculateOffsetCoordinate(center, offset, rotation + 180)

				// Calculate line endpoints (parallel to rotation direction)
				var startPoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, width/2, rotation + 270)
				var endPoint = areaPlanEditor.calculateOffsetCoordinate(lineCenter, width/2, rotation + 90)

				lines.push({
					start: startPoint,
					end: endPoint
				})

				console.log("AreaPlanMapVisuals: Line", i, "- start:", startPoint.latitude, startPoint.longitude, "end:", endPoint.latitude, endPoint.longitude)
			}
		} catch (e) {
			console.log("Error calculating grid lines:", e)
			return []
		}

		return lines
	}

	// Functions to manage map items following QGC patterns
	function addMapItems() {
		console.log("AreaPlanMapVisuals: Adding map items")
		console.log("Component visible:", visible)
		console.log("Component opacity:", opacity)
		console.log("Component interactive:", interactive)

		// Always try to add map items
		console.log("AreaPlanMapVisuals: Adding map items, visible:", visible, "opacity:", opacity)

        console.log("Rectangle corners count:", rectangleCorners.length)
        lastRectangleCornerCount = rectangleCorners.length
        console.log("Area center valid:", areaPlanEditor ? areaPlanEditor.areaCenter.isValid : false)
		console.log("Area dimensions:", areaPlanEditor ? areaPlanEditor.areaWidth + "x" + areaPlanEditor.areaHeight : "null")

			// Create all map items
	if (!areaPlanEditor || !mapControl) {
		console.log("AreaPlanMapVisuals: Skipping map item creation - missing editor or map")
		return
	}

	// Remove existing items to prevent duplicates
	removeMapItems()
	
	console.log("AreaPlanMapVisuals: Creating map items with mapControl:", mapControl)
	console.log("AreaPlanMapVisuals: Map control parent:", mapControl.parent)
	
	// Create the area rectangle
	var areaRect = _objMgrRectangle.createObject(areaRectangleComponent, mapControl, false)
	if (areaRect) {
		areaRect.parent = mapControl
		console.log("Area rectangle created and parented to map")
	} else {
		console.log("Failed to create area rectangle")
	}

	// Create center marker
	var centerMarker = _objMgrCenterMarker.createObject(centerMarkerComponent, mapControl, false)
	if (centerMarker) {
		centerMarker.parent = mapControl
		console.log("Center marker created and parented to map")
	} else {
		console.log("Failed to create center marker")
	}

		// Add grid lines
		addGridLines()

        // Add waypoint markers
        addWaypointMarkers()
        // Per-drone overlays
        addPerDroneOverlays()
		console.log("AreaPlanMapVisuals: Map items added successfully")
	}

	function addGridLines() {
		// Remove existing grid lines first
		removeGridLines()

        console.log("AreaPlanMapVisuals: Creating", gridLines.length, "grid lines")
        lastGridLineCount = gridLines.length

		// Create new grid lines based on current parameters
		for (var i = 0; i < gridLines.length; i++) {
			var lineData = gridLines[i]
			var gridLine = _objMgrGridLines.createObject(gridLineComponent, mapControl, true)
			if (gridLine) {
				gridLine.path = [lineData.start, lineData.end]
				console.log("AreaPlanMapVisuals: Created grid line", i, "from:", lineData.start.latitude, lineData.start.longitude, "to:", lineData.end.latitude, lineData.end.longitude)
				console.log("AreaPlanMapVisuals: Grid line visible:", gridLine.visible)
			} else {
				console.log("AreaPlanMapVisuals: Failed to create grid line", i)
			}
		}

		console.log("AreaPlanMapVisuals: Grid lines creation completed")
	}

	function removeGridLines() {
		// Remove all grid line objects
		_objMgrGridLines.destroyObjects()
	}

	function addWaypointMarkers() {
		// Remove existing waypoint markers first
		removeWaypointMarkers()

		if (!areaPlanEditor || !areaPlanEditor.computePerDroneWaypointPreview) {
			console.log("AreaPlanMapVisuals: No editor or preview function available")
			return
		}

		// Get per-drone waypoint data
        var perDroneData = areaPlanEditor.computePerDroneWaypointPreview()
        console.log("AreaPlanMapVisuals: Creating waypoints for", perDroneData.length, "drones")
        lastWaypointMarkerCount = 0

		// Create waypoint markers for each drone
		for (var droneIndex = 0; droneIndex < perDroneData.length; droneIndex++) {
			var droneData = perDroneData[droneIndex]
			var waypoints = droneData.waypoints || []
			var altOffset = droneData.altitudeOffsetM || 0

			console.log("AreaPlanMapVisuals: Creating", waypoints.length, "waypoints for drone", droneIndex)

			for (var i = 0; i < waypoints.length; i++) {
				var waypointMarker = _objMgrWaypointMarkers.createObject(waypointMarkerComponent, mapControl, true)
                if (waypointMarker) {
                    waypointMarker.coordinate = waypoints[i]
                    waypointMarker.droneIndex = droneIndex
                    waypointMarker.altitudeOffset = altOffset
                    lastWaypointMarkerCount += 1
					console.log("AreaPlanMapVisuals: Created waypoint", i, "for drone", droneIndex, 
						"at:", waypoints[i].latitude, waypoints[i].longitude, 
						"alt:", waypoints[i].altitude)
				} else {
					console.log("AreaPlanMapVisuals: Failed to create waypoint marker", i, "for drone", droneIndex)
				}
			}
		}

		console.log("AreaPlanMapVisuals: Waypoint markers creation completed")
	}

	function removeWaypointMarkers() {
		// Remove all waypoint marker objects
		_objMgrWaypointMarkers.destroyObjects()
	}

    // Create per-drone overlays (markers and optional lines) based on preview
    function addPerDroneOverlays() {
        if (!areaPlanEditor || !areaPlanEditor.computePerDroneWaypointPreview || !mapControl) {
            return
        }
        removePerDroneOverlays()
        // Ensure preview data
        if (!perDronePreview || perDronePreview.length === 0) {
            perDronePreview = areaPlanEditor.computePerDroneWaypointPreview()
        }
        // Build markers per visible drone
        lastPerDroneMarkerCount = 0
        for (var d = 0; d < perDronePreview.length; d++) {
            if (droneVisibility && droneVisibility.length > d && droneVisibility[d] === false) {
                continue
            }
            var group = perDronePreview[d]
            if (!group || !group.waypoints) continue
            var markersForDrone = []
            for (var i = 0; i < group.waypoints.length; i++) {
                var wp = group.waypoints[i]
                var marker = _objMgrPerDroneMarkers.createObject(waypointMarkerComponent, mapControl, true)
                if (marker) {
                    marker.coordinate = wp
                    marker.droneIndex = group.droneIndex || d
                    marker.altitudeOffset = group.altitudeOffsetM || 0
                    lastPerDroneMarkerCount += 1
                    markersForDrone.push(marker)
                }
            }
            _perDroneMarkerObjects.push(markersForDrone)
        }
    }

    function removePerDroneOverlays() {
        _objMgrPerDroneMarkers.destroyObjects()
        _objMgrPerDroneGrid.destroyObjects()
        _perDroneMarkerObjects = []
        _perDroneGridObjects = []
    }

    function removeMapItems() {
		console.log("AreaPlanMapVisuals: Removing all map items")
		_objMgrRectangle.destroyObjects()
		_objMgrCenterMarker.destroyObjects()
		removeGridLines()
		removeWaypointMarkers()
	}

    // Force a full rebuild of map items and overlays
    function forceRefreshAll() {
        console.log("AreaPlanMapVisuals: Force refresh initiated")
        removePerDroneOverlays()
        removeMapItems()
        addMapItems()
        addPerDroneOverlays()
        console.log("AreaPlanMapVisuals: Force refresh completed")
    }

	// Monitor visibility and property changes
	onVisibleChanged: {
		console.log("AreaPlanMapVisuals: Visibility changed to:", visible)
		if (visible) {
			console.log("AreaPlanMapVisuals: Component visible - adding map items")
			// Add items when becoming visible
			addMapItems()
		}
	}

	onMapControlChanged: {
		console.log("AreaPlanMapVisuals: Map control changed")
		if (mapControl) {
			addMapItems()
		}
	}

	onAreaPlanEditorChanged: {
		console.log("AreaPlanMapVisuals: Area plan editor changed")
		if (areaPlanEditor) {
			addMapItems()
		}
	}

	// Monitor area property changes and trigger map updates
	Connections {
		target: areaPlanEditor

		function onAreaWidthChanged() {
			console.log("AreaPlanMapVisuals: Area width changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
            // Refresh grid lines to reflect new geometry
            addGridLines()
		}

		function onAreaHeightChanged() {
			console.log("AreaPlanMapVisuals: Area height changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
            addGridLines()
		}

		function onAreaCenterChanged() {
			console.log("AreaPlanMapVisuals: Area center changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
            addGridLines()
		}

		function onAreaRotationChanged() {
			console.log("AreaPlanMapVisuals: Area rotation changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
            addGridLines()
		}

		function onLineSpacingChanged() {
			console.log("AreaPlanMapVisuals: Line spacing changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
            addGridLines()
		}

		function onNumPointsChanged() {
			console.log("AreaPlanMapVisuals: Number of points changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
		}
		function onDroneCountChanged() {
            console.log("AreaPlanMapVisuals: Drone count changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onAltitudeBandStartChanged() {
            console.log("AreaPlanMapVisuals: Altitude band start changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onAltitudeBandStepChanged() {
            console.log("AreaPlanMapVisuals: Altitude band step changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onTimeOffsetPerDroneChanged() {
            console.log("AreaPlanMapVisuals: Time offset per drone changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onMissionAltitudeChanged() {
            console.log("AreaPlanMapVisuals: Mission altitude changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onPerTargetSeparationSChanged() {
            console.log("AreaPlanMapVisuals: Per-target separation changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
        function onLandAtTargetReturnChanged() {
            console.log("AreaPlanMapVisuals: Land-at-target policy changed")
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
        }
	}

	// Monitor drawing mode changes
	onIsDrawingModeChanged: {
		console.log("AreaPlanMapVisuals: isDrawingMode changed to:", isDrawingMode)
	}

	// Monitor C++ backend drawing mode changes
	Connections {
		target: areaPlanEditor

		function onIsDrawingModeChanged() {
			console.log("C++ backend isDrawingMode changed to:", areaPlanEditor.isDrawingMode)
		}
	}

    // Simple legend + visibility toggles for per-drone overlays
    Column {
        id: legend
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelHeight * 0.25
        visible: (perDronePreview && perDronePreview.length > 0)
                 && (_seriesColors && _seriesColors.length > 0)
                 && (droneVisibility && droneVisibility.length === perDronePreview.length)

        Repeater {
            model: perDronePreview ? perDronePreview.length : 0
            delegate: Row {
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                Rectangle {
                    width: ScreenTools.defaultFontPixelHeight
                    height: width
                    radius: width/2
                    color: (_seriesColors && _seriesColors.length > 0)
                           ? (_seriesColors[index % _seriesColors.length] || "#00FF00")
                           : "#00FF00"
                }
                QGCSwitch {
                    checked: (droneVisibility && droneVisibility.length > index)
                             ? (droneVisibility[index] !== false)
                             : true
                    onToggled: {
                        if (droneVisibility) {
                            while (droneVisibility.length <= index) droneVisibility.push(true)
                            droneVisibility[index] = checked
                        }
                        addPerDroneOverlays()
                    }
                }
                QGCLabel {
                    text: {
                        var group = perDronePreview && perDronePreview.length > index ? perDronePreview[index] : null
                        var alt = group && group.altitudeOffsetM !== undefined ? group.altitudeOffsetM : 0
                        var sign = alt > 0 ? "+" : ""
                        return qsTr("Aircraft %1 — Altitude offset: %2%3 m").arg(index).arg(sign).arg(Math.round(alt))
                    }
                }
            }
        }
    }

    // Force refresh overlay button
    Row {
        id: refreshRow
        anchors.right: parent.right
        anchors.top: legend.visible ? legend.bottom : parent.top
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelWidth * 0.5
        QGCButton {
            text: qsTr("Force Refresh")
            onClicked: forceRefreshAll()
        }
    }

    // MouseArea covering the map. Handles recentering on click even when not in drawing mode.
    MouseArea {
		id: mapAreaMouseArea
		anchors.fill: parent
        enabled: interactive && isDrawingMode  // Only enable shape manipulation in drawing mode; map remains pannable otherwise
		hoverEnabled: true
		preventStealing: true
		z: 999  // High z-order but below the rectangle MouseArea
        acceptedButtons: Qt.LeftButton

		// Add a visible background for debugging (semi-transparent red)
		Rectangle {
			anchors.fill: parent
			color: "red"
			opacity: 0.1
			visible: interactive && isDrawingMode
		}

		property point startPos
		property var startCenter
		property var startCoordinate
		property bool hasMoved: false

		Component.onCompleted: {
			console.log("Map area MouseArea completed - enabled:", enabled, "interactive:", interactive, "isDrawingMode:", isDrawingMode)
			console.log("Map area MouseArea - z-order:", z)
		}

		// Monitor property changes
		onEnabledChanged: {
			console.log("Map area MouseArea enabled changed to:", enabled)
		}

        onPressed: function(mouse) {
            console.log("Map area pressed")
            console.log("Map area MouseArea - enabled:", enabled, "interactive:", interactive, "isDrawingMode:", isDrawingMode)
            console.log("Mouse position:", mouse.x, mouse.y)
            
            if (mapControl && areaPlanEditor) {
                // Get click coordinate
                var p = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(p.x, p.y), false)
                
                if (clickCoordinate.isValid) {
                    console.log("Map area clicked at coordinate:", clickCoordinate.latitude, clickCoordinate.longitude)
                    
                    // Always set the center immediately on first click
                    if (!areaPlanEditor.areaCenter.isValid ||
                        (Math.abs(areaPlanEditor.areaCenter.latitude) < 0.001 && Math.abs(areaPlanEditor.areaCenter.longitude) < 0.001) ||
                        areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
                        
                        areaPlanEditor.setAreaCenter(clickCoordinate)
                        console.log("Area center set to:", clickCoordinate.latitude, clickCoordinate.longitude)
                        
                        // Set default area size if not already set
                        if (areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
                            areaPlanEditor.setAreaWidth(10.0)
                            areaPlanEditor.setAreaHeight(10.0)
                            console.log("Set default area size: 10x10 meters")
                        }
                    }
                }
                
                // Store start position for potential dragging
                startPos = Qt.point(mouse.x, mouse.y)
                startCenter = areaPlanEditor.areaCenter
                startCoordinate = clickCoordinate
                isDragging = true
                hasMoved = false
            } else {
                console.log("ERROR: mapControl or areaPlanEditor is null!")
            }
        }

        onPositionChanged: function(mouse) {
			if (pressed && areaPlanEditor && mapControl) {
				hasMoved = true
				console.log("Map area dragging - isDragging:", isDragging)
				// Get current mouse position coordinate
                var mapped = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var currentCoordinate = mapControl.toCoordinate(Qt.point(mapped.x, mapped.y), false)

				if (currentCoordinate.isValid) {
					// Move center directly to current mouse position
					console.log("Map area dragging to:", currentCoordinate.latitude, currentCoordinate.longitude)
					areaPlanEditor.setAreaCenter(currentCoordinate)
        }
			}
		}

        onReleased: function(mouse) {
			console.log("Map area released - setting isDragging to false")
			isDragging = false
			console.log("Map area isDragging set to:", isDragging)

			// If we didn't move, treat as a click
			if (!hasMoved && areaPlanEditor && mapControl) {
                var mappedClick = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                var clickCoordinate = mapControl.toCoordinate(Qt.point(mappedClick.x, mappedClick.y), false)

				if (clickCoordinate.isValid) {
					console.log("Map area clicked at:", clickCoordinate.latitude, clickCoordinate.longitude)

					// Set center point on first click or if center is not valid
					if (!areaPlanEditor.areaCenter.isValid ||
						(Math.abs(areaPlanEditor.areaCenter.latitude) < 0.001 && Math.abs(areaPlanEditor.areaCenter.longitude) < 0.001) ||
						areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {

						areaPlanEditor.setAreaCenter(clickCoordinate)
						console.log("Area center set to:", clickCoordinate.latitude, clickCoordinate.longitude)

						// Set default area size if not already set
						if (areaPlanEditor.areaWidth <= 0 || areaPlanEditor.areaHeight <= 0) {
							areaPlanEditor.setAreaWidth(10.0)
							areaPlanEditor.setAreaHeight(10.0)
							console.log("Set default area size: 10x10 meters")
        }
					} else {
						// Move center to clicked point
						areaPlanEditor.setAreaCenter(clickCoordinate)
						console.log("Moved area center to:", clickCoordinate.latitude, clickCoordinate.longitude)
					}
				}
			}
		}
	}

    Component.onDestruction: {
        removeMapItems()
        removePerDroneOverlays()
    }
}