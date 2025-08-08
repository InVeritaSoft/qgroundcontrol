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
import QGroundControl.ScreenTools
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
	property real _zorderCenterMarker:  QGroundControl.zOrderMapItems + 1
	property real _zorderWaypoints:     QGroundControl.zOrderMapItems + 3

	// Interactive drawing properties
	property bool isDrawingMode: areaPlanEditor ? areaPlanEditor.isDrawingMode : false
	property bool showGridLines: true
	property bool showWaypoints: true
    property bool isDragging: false
    // Per-drone overlays
    property var perDronePreview: (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) ? areaPlanEditor.computePerDroneWaypointPreview() : []
    property var droneVisibility: new Array(perDronePreview ? perDronePreview.length : 0).fill(true)
    // Robust color palette (fallback to hex constants to avoid undefined warnings)
    readonly property var _seriesColors: [
        (qgcPal && qgcPal.buttonHighlight) || "#FF9800",  // orange
        "#1E88E5",  // blue
        "#8E24AA",  // purple
        "#43A047",  // green
        "#E53935",  // red
        "#00ACC1",  // cyan
        "#FDD835",  // yellow
        "#EC407A"   // pink
    ]
    // Cache to avoid unnecessary preview recompute
    property string _lastPreviewKey: ""
    property var _lastPreviewData: []

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

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

	// Calculate rectangle corners based on area parameters
	property var rectangleCorners: {
		if (!areaPlanEditor || !areaPlanEditor.areaCenter || !areaPlanEditor.areaWidth || !areaPlanEditor.areaHeight) {
			console.log("AreaPlanMapVisuals: No areaPlanEditor or areaCenter")
			return []
		}

		var center = areaPlanEditor.areaCenter
		var width = areaPlanEditor.areaWidth
		var height = areaPlanEditor.areaHeight
		var rotation = areaPlanEditor.areaRotation || 0.0

		console.log("AreaPlanMapVisuals: Calculating corners - center:", center.latitude, center.longitude, "width:", width, "height:", height, "rotation:", rotation)
		console.log("AreaPlanMapVisuals: Center valid:", center.isValid)
		console.log("AreaPlanMapVisuals: Width > 0:", width > 0, "Height > 0:", height > 0)

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
			id: areaRectangle
			path: rectangleCorners
			color: interiorColor
			border.color: borderColor
			border.width: borderWidth
			opacity: interiorOpacity
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
			id: gridLine
            line.color: (qgcPal && qgcPal.colorGreen) ? qgcPal.colorGreen : "#43A047"
			line.width: Math.max(1, Math.round(ScreenTools.defaultFontPixelWidth * 0.5))
			z: QGroundControl.zOrderMapItems - 1
		}
	}

	// Component for waypoint markers
	Component {
		id: waypointMarkerComponent

		MapQuickItem {
			id: waypointMarker
			z: _zorderWaypoints
			anchorPoint.x: sourceItem.width / 2
			anchorPoint.y: sourceItem.height / 2

            sourceItem: Rectangle {
				width: ScreenTools.defaultFontPixelHeight * 1.5
				height: width
				radius: width / 2
                color: (qgcPal && qgcPal.colorGreen) ? qgcPal.colorGreen : "#43A047"
                border.color: (qgcPal && qgcPal.text) ? qgcPal.text : "#FFFFFF"
				border.width: Math.max(1, Math.round(ScreenTools.defaultFontPixelWidth * 0.5))

				// Add a small dot in the center
				Rectangle {
					anchors.centerIn: parent
					width: parent.width * 0.4
					height: width
					radius: width / 2
					color: qgcPal.text
				}

				// Smooth scale animation
				Behavior on scale {
					NumberAnimation { duration: 150 }
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

		// Don't add map items if component is not visible
		if (!visible || opacity === 0) {
			console.log("AreaPlanMapVisuals: Skipping map item creation - component not visible")
			return
		}

		console.log("Rectangle corners count:", rectangleCorners.length)
		console.log("Area center valid:", areaPlanEditor ? areaPlanEditor.areaCenter.isValid : false)
		console.log("Area dimensions:", areaPlanEditor ? areaPlanEditor.areaWidth + "x" + areaPlanEditor.areaHeight : "null")

		// Clear existing items first to prevent duplication
		removeMapItems()

		// Only add items if we have valid data
		if (!areaPlanEditor || !areaPlanEditor.areaCenter.isValid || rectangleCorners.length < 3) {
			console.log("AreaPlanMapVisuals: Skipping map item creation - invalid data")
			console.log("  areaPlanEditor:", !!areaPlanEditor)
			console.log("  areaCenter valid:", areaPlanEditor ? areaPlanEditor.areaCenter.isValid : false)
			console.log("  rectangleCorners length:", rectangleCorners.length)
			return
		}

		// Create area rectangle
		var areaRect = _objMgrRectangle.createObject(areaRectangleComponent, mapControl, true)
		console.log("Area rectangle created:", !!areaRect)
		if (areaRect) {
			console.log("Area rectangle path length:", areaRect.path.length)
			console.log("Area rectangle visible:", areaRect.visible)
			console.log("Area rectangle opacity:", areaRect.opacity)
		}

		// Create center marker
		var centerMarker = _objMgrCenterMarker.createObject(centerMarkerComponent, mapControl, true)
		console.log("Center marker created:", !!centerMarker)
		if (centerMarker) {
			console.log("Center marker coordinate:", centerMarker.coordinate.latitude, centerMarker.coordinate.longitude)
			console.log("Center marker visible:", centerMarker.visible)
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

		console.log("AreaPlanMapVisuals: Creating", waypointPositions.length, "waypoint markers")

		// Create new waypoint markers based on current parameters
		for (var i = 0; i < waypointPositions.length; i++) {
			var waypointData = waypointPositions[i]
			var waypointMarker = _objMgrWaypointMarkers.createObject(waypointMarkerComponent, mapControl, true)
			if (waypointMarker) {
				waypointMarker.coordinate = waypointData.coordinate
				console.log("AreaPlanMapVisuals: Created waypoint marker", i, "at:", waypointData.coordinate.latitude, waypointData.coordinate.longitude)
				console.log("AreaPlanMapVisuals: Marker coordinate valid:", waypointData.coordinate.isValid)
				console.log("AreaPlanMapVisuals: Marker visible:", waypointMarker.visible)
			} else {
				console.log("AreaPlanMapVisuals: Failed to create waypoint marker", i)
			}
		}

		console.log("AreaPlanMapVisuals: Waypoint markers creation completed")
	}

	function removeWaypointMarkers() {
		// Remove all waypoint marker objects
		_objMgrWaypointMarkers.destroyObjects()
	}

    // Per-drone overlays creation
    function addPerDroneOverlays() {
        var t0 = Date.now()
        // Initialize object arrays to match drone count
        if (!_perDroneMarkerObjects || _perDroneMarkerObjects.length !== (perDronePreview ? perDronePreview.length : 0)) {
            // Clean old
            removePerDroneOverlays()
            _perDroneMarkerObjects = []
            for (var k = 0; k < (perDronePreview ? perDronePreview.length : 0); k++) _perDroneMarkerObjects.push([])
        }

        var totalCreated = 0, totalUpdated = 0, totalDestroyed = 0

        // For each drone group
        if (!perDronePreview || perDronePreview.length === 0) {
            console.log("AreaPlanMapVisuals: no per-drone preview; clearing overlays")
            removePerDroneOverlays()
            return
        }

        for (var d = 0; d < perDronePreview.length; d++) {
            var color = _seriesColors[(d % _seriesColors.length)] || "#00FF00"
            var wps = droneVisibility[d] ? (perDronePreview[d].waypoints || []) : []
            var arr = _perDroneMarkerObjects[d]
            // Ensure enough objects
            for (var i = 0; i < wps.length; i++) {
                var obj
                if (i < arr.length) {
                    obj = arr[i]
                    if (obj) {
                        obj.coordinate = wps[i]
                        if (obj.sourceItem) obj.sourceItem.color = color
                        totalUpdated++
                    }
                } else {
                    obj = _objMgrPerDroneMarkers.createObject(waypointMarkerComponent, mapControl, true)
                    if (obj) {
                        obj.coordinate = wps[i]
                        if (obj.sourceItem) obj.sourceItem.color = color
                        arr.push(obj)
                        totalCreated++
                    }
                }
            }
            // Destroy extras
            for (var j = wps.length; j < arr.length; j++) {
                if (arr[j] && arr[j].destroy) {
                    arr[j].destroy()
                    totalDestroyed++
                }
            }
            if (arr.length > wps.length) arr.length = wps.length
        }
        console.log("AreaPlanMapVisuals: overlays updated in", (Date.now()-t0), "ms; created:", totalCreated, "updated:", totalUpdated, "destroyed:", totalDestroyed)
    }

    function removePerDroneOverlays() {
        _objMgrPerDroneGrid.destroyObjects()
        _objMgrPerDroneMarkers.destroyObjects()
        if (_perDroneMarkerObjects) {
            for (var d = 0; d < _perDroneMarkerObjects.length; d++) {
                var arr = _perDroneMarkerObjects[d]
                for (var i = 0; i < arr.length; i++) {
                    if (arr[i] && arr[i].destroy) arr[i].destroy()
                }
            }
            _perDroneMarkerObjects = []
        }
    }

	function removeMapItems() {
		console.log("AreaPlanMapVisuals: Removing all map items")
		_objMgrRectangle.destroyObjects()
		_objMgrCenterMarker.destroyObjects()
		removeGridLines()
		removeWaypointMarkers()
	}

	// Monitor visibility changes
	onVisibleChanged: {
		console.log("AreaPlanMapVisuals: Visibility changed to:", visible)
		if (!visible) {
			console.log("AreaPlanMapVisuals: Component not visible - removing all map items")
			removeMapItems()
		} else {
			console.log("AreaPlanMapVisuals: Component visible - adding map items")
			// Add items when becoming visible
			addMapItems()
		}
	}

	onOpacityChanged: {
		console.log("AreaPlanMapVisuals: Opacity changed to:", opacity)
		if (opacity === 0) {
			console.log("AreaPlanMapVisuals: Opacity is 0 - removing all map items")
			removeMapItems()
		} else if (visible) {
			console.log("AreaPlanMapVisuals: Opacity is non-zero and visible - adding map items")
			// Add items when opacity becomes non-zero and visible
			addMapItems()
		}
	}

	// Also monitor the interactive property changes
	onInteractiveChanged: {
		console.log("AreaPlanMapVisuals: Interactive changed to:", interactive)
		if (!interactive) {
			console.log("AreaPlanMapVisuals: Not interactive - removing map items")
			removeMapItems()
		} else if (visible && opacity > 0) {
			console.log("AreaPlanMapVisuals: Interactive and visible - adding map items")
			addMapItems()
		}
	}

	// Monitor area property changes and trigger map updates
	Connections {
		target: areaPlanEditor

		function onAreaWidthChanged() {
			console.log("AreaPlanMapVisuals: Area width changed, updating map items")
			addMapItems()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
		}

		function onAreaHeightChanged() {
			console.log("AreaPlanMapVisuals: Area height changed, updating map items")
			addMapItems()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
		}

		function onAreaCenterChanged() {
			console.log("AreaPlanMapVisuals: Area center changed, updating map items")
			addMapItems()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
		}

		function onAreaRotationChanged() {
			console.log("AreaPlanMapVisuals: Area rotation changed, updating map items")
			addMapItems()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                _updatePreviewIfChanged()
                _overlayDebounce.restart()
            }
		}

		function onLineSpacingChanged() {
			console.log("AreaPlanMapVisuals: Line spacing changed, updating map items")
			addGridLines()
			addWaypointMarkers()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                perDronePreview = areaPlanEditor.computePerDroneWaypointPreview()
                addPerDroneOverlays()
            }
		}

		function onNumPointsChanged() {
			console.log("AreaPlanMapVisuals: Number of points changed, updating map items")
			addWaypointMarkers()
            if (areaPlanEditor && areaPlanEditor.computePerDroneWaypointPreview) {
                perDronePreview = areaPlanEditor.computePerDroneWaypointPreview()
                addPerDroneOverlays()
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
                QGCLabel { text: qsTr("Drone %1").arg(index) }
            }
        }
    }

    // MouseArea covering the map. Handles recentering on click even when not in drawing mode.
    MouseArea {
		id: mapAreaMouseArea
		anchors.fill: parent
        enabled: interactive
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
			console.log("Map area pressed - setting isDragging to true")
			console.log("Map area MouseArea - enabled:", enabled, "interactive:", interactive, "isDrawingMode:", isDrawingMode)
			console.log("Mouse position:", mouse.x, mouse.y)
			startPos = Qt.point(mouse.x, mouse.y)
			startCenter = areaPlanEditor ? areaPlanEditor.areaCenter : null
			isDragging = true
			hasMoved = false
			console.log("Map area isDragging set to:", isDragging)
			// Get the coordinate at the press position
            if (mapControl) {
                var p = mapAreaMouseArea.mapToItem(mapControl, mouse.x, mouse.y)
                startCoordinate = mapControl.toCoordinate(Qt.point(p.x, p.y), false)
				console.log("Map area drag started at coordinate:", startCoordinate.latitude, startCoordinate.longitude)
			} else {
				console.log("ERROR: mapControl is null!")
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
	}

    Component.onDestruction: {
        removeMapItems()
        removePerDroneOverlays()
    }
}