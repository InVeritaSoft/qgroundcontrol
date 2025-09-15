/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file AreaPlanEditor.qml
 * @brief Multi-Drone Area Planning Mission Editor for QGroundControl
 * 
 * This component provides a comprehensive interface for planning and executing
 * area coverage missions using single or multiple drones. It integrates with
 * the QGC mission system to generate, visualize, and upload coordinated missions.
 * 
 * @section Architecture
 * - Frontend: QML-based UI with property bindings to C++ backend
 * - Backend: AreaPlanEditor C++ class (src/QmlControls/AreaPlanEditor.h/.cc)
 * - Integration: MissionController, Vehicle Manager, and QGC core systems
 * 
 * @section Key Features
 * - Interactive area definition with real-time visualization
 * - Multi-drone mission planning with altitude bands and time staggering
 * - Configurable mission policies (RTL, loiter, payload release)
 * - Per-drone mission generation and vehicle mapping
 * - Mission file export and direct vehicle upload
 * 
 * @section Usage Flow
 * 1. Define area center and dimensions
 * 2. Configure line spacing and waypoint density
 * 3. Set multi-drone parameters (count, altitude bands, timing)
 * 4. Generate and preview waypoints
 * 5. Save missions and upload to vehicles
 * 
 * @section Dependencies
 * - Qt 6.8.3+ with QtLocation and QtPositioning
 * - QGroundControl core components
 * - AreaPlanMapVisuals.qml for map visualization
 * - C++ backend for calculations and mission generation
 * 
 * @section Developer Notes
 * - All user-facing text uses qsTr() for internationalization
 * - Sizing follows ScreenTools patterns for consistency
 * - Colors use QGCPalette for theme compatibility
 * - Property bindings ensure real-time UI updates
 * - Error handling with user-friendly messages
 * 
 * @author QGroundControl Team
 * @version 1.0
 * @date 2025
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning
import Qt.labs.settings 1.1

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

/**
 * @brief Main Area Plan Editor component
 * 
 * This is the root component that provides the complete area planning interface.
 * It manages the connection between the QML UI and the C++ backend, handles
 * vehicle mapping, and coordinates all mission planning operations.
 */
Item {
	id: _root

	// ============================================================================
	// CORE BACKEND INTEGRATION
	// ============================================================================
	
	/**
	 * @brief Reference to the C++ backend AreaPlanEditor instance
	 * 
	 * This property provides access to all backend functionality including:
	 * - Area parameter management (width, height, spacing, rotation)
	 * - Waypoint generation algorithms
	 * - Mission creation and validation
	 * - Vehicle communication and upload
	 * 
	 * The backend is initialized by QGroundControl core and provides
	 * the Q_PROPERTY and Q_INVOKABLE API for all operations.
	 */
	property var areaPlanEditor: null
	// ============================================================================
	// VEHICLE MANAGEMENT & MAPPING
	// ============================================================================
	
	/**
	 * @brief Currently selected vehicle for mission upload
	 * 
	 * This property automatically tracks the active vehicle from the
	 * QGC multi-vehicle manager. It's used as the default target
	 * for single-drone mission uploads.
	 */
	property var selectedVehicle: QGroundControl.multiVehicleManager.activeVehicle
	
	/**
	 * @brief Mapping between drone indices and vehicle objects
	 * 
	 * This object maps each drone index (0, 1, 2, ...) to its assigned
	 * vehicle object. The mapping is persisted across sessions and
	 * automatically rebuilt when vehicles reconnect.
	 * 
	 * Structure: { "0": vehicleObj, "1": vehicleObj, ... }
	 */
	property var vehicleMapping: ({})
	
	/**
	 * @brief Per-drone waypoint preview data
	 * 
	 * This array contains preview information for each drone's mission,
	 * including waypoint coordinates, altitude offsets, and timing.
	 * 
	 * Structure: [
	 *   { droneIndex: 0, altitudeOffsetM: 0, timeOffsetS: 0, waypoints: [...] },
	 *   { droneIndex: 1, altitudeOffsetM: 10, timeOffsetS: 5, waypoints: [...] },
	 *   ...
	 * ]
	 */
	property var waypointPreview: []
	
	/**
	 * @brief Vehicle labels for UI dropdowns
	 * 
	 * Human-readable labels for each connected vehicle, used in
	 * the drone-to-vehicle mapping interface.
	 * 
	 * Example: ["Vehicle 1", "Vehicle 2", "Vehicle 3"]
	 */
	property var vehicleLabels: []
	
	/**
	 * @brief Vehicle objects for programmatic access
	 * 
	 * Parallel array to vehicleLabels containing the actual
	 * vehicle objects for each connected vehicle.
	 * 
	 * Example: [vehicleObj1, vehicleObj2, vehicleObj3]
	 */
	property var vehicleObjects: []
	
	/**
	 * @brief Derived count of mapped drones (reactive)
	 */
	property int mappedCount: 0
	
	/**
	 * @brief Upload status tracking for each drone
	 * 
	 * Tracks which drone missions have been successfully uploaded
	 * to their assigned vehicles. Used for visual feedback in the UI.
	 * 
	 * Structure: { "0": true, "1": false, "2": true }
	 */
	property var uploadedMap: ({})

	// ============================================================================
	// PERSISTENCE & SETTINGS
	// ============================================================================
	
	/**
	 * @brief Persistent storage for vehicle-drone mappings
	 * 
	 * This Settings object automatically persists the mapping between
	 * drone indices and vehicle IDs across application sessions.
	 * The data is stored in the user's QGC settings and automatically
	 * restored when the application restarts.
	 */
	Settings {
		id: areaMapSettings
		category: "AreaPlanEditor"
		
		/**
		 * @brief Saved mapping of drone indices to vehicle IDs
		 * 
		 * Structure: { "0": "vehicleId1", "1": "vehicleId2", ... }
		 * This mapping is automatically saved and restored across sessions.
		 */
		property var savedMap: ({})
	}

	// ============================================================================
	// VEHICLE MANAGEMENT HELPER FUNCTIONS
	// ============================================================================
	
	/**
	 * @brief Find vehicle object by its numeric ID
	 * 
	 * @param id The vehicle ID to search for
	 * @return The vehicle object if found, null otherwise
	 * 
	 * This function searches through the vehicleObjects array to find
	 * a vehicle with the specified ID. Used for rebuilding mappings
	 * from persisted data when vehicles reconnect.
	 */
	function getVehicleById(id) {
		if (id === undefined || id === null) return null
		for (var i = 0; i < vehicleObjects.length; i++) {
			var v = vehicleObjects[i]
			if (v && v.id === id) return v
		}
		return null
	}

	/**
	 * @brief Set the vehicle mapping for a specific drone index
	 * 
	 * @param droneIndex The drone index (0, 1, 2, ...) to map
	 * @param veh The vehicle object to assign, or null to clear mapping
	 * 
	 * This function updates both the runtime vehicleMapping object and
	 * the persisted settings. When a vehicle is assigned, it's automatically
	 * saved for future sessions.
	 */
	function setMappingForDrone(droneIndex, veh) {
		vehicleMapping[droneIndex] = veh || null
		var sm = areaMapSettings.savedMap || {}
		sm[droneIndex] = veh ? veh.id : null
		areaMapSettings.savedMap = sm
		// Recompute mapped count and refresh binding
		var count = 0
		for (var k in vehicleMapping) { if (vehicleMapping.hasOwnProperty(k) && vehicleMapping[k]) count++ }
		mappedCount = count
		var newMap = {}
		for (var kk in vehicleMapping) { if (vehicleMapping.hasOwnProperty(kk)) newMap[kk] = vehicleMapping[kk] }
		vehicleMapping = newMap
	}

	/**
	 * @brief Rebuild vehicle mappings from persisted data
	 * 
	 * This function is called when the application starts or when
	 * the vehicle list changes. It attempts to restore the previous
	 * drone-to-vehicle mappings by looking up vehicle objects by
	 * their persisted IDs.
	 * 
	 * @note This function handles cases where vehicles may have
	 * disconnected and reconnected with different connection orders.
	 */
	function rebindMappingsFromSaved() {
		var sm = areaMapSettings.savedMap
		if (!sm) return
		for (var k in sm) {
			if (!sm.hasOwnProperty(k)) continue
			var id = sm[k]
			var obj = getVehicleById(id)
			vehicleMapping[k] = obj
		}
		// Update derived state
		var count = 0
		for (var kk in vehicleMapping) { if (vehicleMapping[kk]) count++ }
		mappedCount = count
		var newMap = {}
		for (var k2 in vehicleMapping) { if (vehicleMapping.hasOwnProperty(k2)) newMap[k2] = vehicleMapping[k2] }
		vehicleMapping = newMap
	}

	/**
	 * @brief Refresh the list of available vehicles
	 * 
	 * This function queries the QGC multi-vehicle manager to get
	 * the current list of connected vehicles and updates both
	 * vehicleLabels and vehicleObjects arrays. After refreshing,
	 * it attempts to rebind any saved mappings to the current
	 * vehicle objects.
	 * 
	 * @note This function is called automatically when vehicles
	 * connect/disconnect and can be called manually to refresh.
	 */
	function refreshVehicleList() {
		var vModel = QGroundControl.multiVehicleManager.vehicles
		var labels = []
		var objs = []
		if (vModel && vModel.count !== undefined) {
			for (var i = 0; i < vModel.count; i++) {
				var v = vModel.get(i)
				if (v) { labels.push(qsTr("Vehicle %1").arg(i+1)); objs.push(v) }
			}
		}
		vehicleLabels = labels
		vehicleObjects = objs
		rebindMappingsFromSaved()
		var count = 0
		for (var k3 in vehicleMapping) { if (vehicleMapping[k3]) count++ }
		mappedCount = count
	}

	// ============================================================================
	// SIZING & LAYOUT HELPERS
	// ============================================================================
	
	/**
	 * @brief Height multiplier based on default font size
	 * 
	 * This property provides consistent height sizing throughout the UI
	 * by using ScreenTools.defaultFontPixelHeight as a base unit.
	 * All heights should be multiples of this value for consistency.
	 * 
	 * @note Never use hardcoded pixel values - always use _h * multiplier
	 */
	readonly property real _h: ScreenTools.defaultFontPixelHeight
	
	/**
	 * @brief Width multiplier based on default font size
	 * 
	 * This property provides consistent width sizing throughout the UI
	 * by using ScreenTools.defaultFontPixelWidth as a base unit.
	 * All widths should be multiples of this value for consistency.
	 * 
	 * @note Never use hardcoded pixel values - always use _w * multiplier
	 */
	readonly property real _w: ScreenTools.defaultFontPixelWidth

	// ============================================================================
	// COMPONENT LIFECYCLE
	// ============================================================================
	
	/**
	 * @brief Component initialization handler
	 * 
	 * This function is called when the component is fully loaded and
	 * ready for use. It performs essential setup tasks including:
	 * - Refreshing the vehicle list
	 * - Rebuilding vehicle mappings from persisted data
	 * - Setting up initial UI state
	 */
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
		function onTakeoffHeightChanged() { _refreshPreview() }
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

	// ============================================================================
	// THEME & STYLING
	// ============================================================================
	
	/**
	 * @brief QGC color palette for consistent theming
	 * 
	 * This palette provides access to all QGC theme colors including
	 * window backgrounds, text colors, button colors, and accent colors.
	 * The palette automatically adapts to light/dark themes and ensures
	 * consistent visual appearance across the application.
	 */
	QGCPalette {
		id: qgcPal
		colorGroupEnabled: enabled
	}

	// ============================================================================
	// MAIN UI LAYOUT
	// ============================================================================
	
	/**
	 * @brief Main background container
	 * 
	 * This Rectangle serves as the root container for all UI elements.
	 * It provides the base background color and contains the scrollable
	 * main content area. The background color automatically adapts to
	 * the current QGC theme.
	 */
	Rectangle {
		id: background
		anchors.fill: parent
		color: qgcPal.window
		
		/**
		 * @brief Scrollable container for main content
		 * 
		 * This ScrollView provides vertical scrolling for the main content
		 * while preventing horizontal scrolling. It automatically shows
		 * scroll bars when content exceeds the available height.
		 * 
		 * @note Horizontal scrolling is disabled to maintain consistent
		 * layout across different screen sizes and orientations.
		 */
		ScrollView {
			id: scrollView
			anchors.fill: parent
			ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
			ScrollBar.vertical.policy: ScrollBar.AsNeeded
			
			/**
			 * @brief Main content layout container
			 * 
			 * This Column arranges all UI sections vertically with consistent
			 * spacing. Each section (header, configuration, controls, etc.)
			 * is added as a child of this column. The spacing uses the
			 * standardized _h multiplier for consistent visual rhythm.
			 * 
			 * @note The width is bound to the scroll view width to ensure
			 * proper layout and scrolling behavior.
			 */
			Column {
				id: mainColumn
				width: scrollView.width
				spacing: _h
				anchors.margins: _h
				
				// ============================================================================
				// APPLICATION HEADER
				// ============================================================================
				
				/**
				 * @brief Main application title
				 * 
				 * This label displays the primary title for the Area Plan Editor.
				 * It uses the QGC label component for consistent styling and
				 * internationalization support.
				 */
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

				// ============================================================================
				// AREA CONFIGURATION SECTION
				// ============================================================================
				
				/**
				 * @brief Area configuration parameters container
				 * 
				 * This section contains all the core parameters for defining
				 * the operational area including dimensions, center coordinates,
				 * and geometric properties. The container uses a subtle background
				 * color and rounded corners to visually group related controls.
				 * 
				 * @note The height is dynamically calculated based on content
				 * to ensure proper spacing and visual hierarchy.
				 */
				Rectangle {
					width: parent.width
					height: areaConfigColumn.height + _h * 2
					color: qgcPal.windowShade
					radius: _w * 0.5
					
					/**
					 * @brief Vertical layout for area configuration controls
					 * 
					 * This column arranges the area configuration controls
					 * vertically with consistent spacing. It contains the
					 * section header and the parameter input grid.
					 * 
					 * @note The spacing uses _h * 0.8 for tighter grouping
					 * of related configuration elements.
					 */
					Column {
						id: areaConfigColumn
						anchors.left: parent.left
						anchors.right: parent.right
						anchors.top: parent.top
						anchors.margins: _h
						spacing: _h * 0.8

						/**
						 * @brief Section header label
						 * 
						 * This label provides a clear title for the area
						 * configuration section. It uses medium font size
						 * and bold styling to establish visual hierarchy.
						 */
						QGCLabel {
							text: qsTr("Area Configuration Parameters")
							font.pointSize: ScreenTools.mediumFontPointSize
							font.bold: true
							width: parent.width
							height: _h * 1.2
							verticalAlignment: Text.AlignVCenter
						}

						/**
						 * @brief Parameter input grid layout
						 * 
						 * This grid arranges parameter labels and input fields
						 * in a two-column layout for efficient space usage.
						 * Each row contains a label and its corresponding input
						 * control, with consistent spacing between elements.
						 * 
						 * @note The grid automatically handles alignment and
						 * spacing for optimal visual presentation.
						 */
						Grid {
							columns: 2
							width: parent.width
							rowSpacing: _h * 0.6
							columnSpacing: _w
							
							/**
							 * @brief Error color for validation feedback
							 * 
							 * This property defines the color used to highlight
							 * input fields with validation errors. It provides
							 * immediate visual feedback to users about invalid input.
							 */
							property color _err: '#E53935'

							/**
							 * @brief Area width parameter label
							 * 
							 * This label describes the area width input field.
							 * It specifies that the value should be in meters
							 * and uses consistent sizing with other parameter labels.
							 */
							QGCLabel { 
								text: qsTr("Area Width (Meters):")
								width: parent.width * 0.4
								height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							
							/**
							 * @brief Area width input field container
							 * 
							 * This column contains the width input field and
							 * its associated error message. The layout ensures
							 * proper spacing and alignment of the input control.
							 */
							Column {
								width: parent.width * 0.5
								spacing: _h * 0.2
								
								/**
								 * @brief Area width input field
								 * 
								 * This text field allows users to specify the
								 * width of the operational area in meters.
								 * It includes validation to ensure the value
								 * is within acceptable bounds (1-1000m).
								 * 
								 * @note The field automatically updates the
								 * backend areaPlanEditor.areaWidth property
								 * when editing is finished and validation passes.
								 */
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
								
								/**
								 * @brief Width input validation error display
								 * 
								 * This label displays validation errors for the
								 * width input field. It only appears when there
								 * are validation errors and uses the error color
								 * for immediate visual feedback.
								 */
								QGCLabel { 
									id: widthError
									color: parent.parent._err
									visible: text.length > 0
									text: ""
								}
							}

							// ============================================================================
							// MULTI-DRONE PARAMETERS
							// ============================================================================
							
							/**
							 * @brief Multi-drone parameters section header
							 * 
							 * This comment marks the beginning of the multi-drone
							 * configuration section. All parameters below this point
							 * control how multiple drones coordinate their missions.
							 */
							// --- Multi-drone parameters ---
							
							/**
							 * @brief Drone count parameter label
							 * 
							 * This label describes the drone count input field.
							 * It specifies the number of drones that will be
							 * used for the area coverage mission.
							 */
							QGCLabel {
								text: qsTr("Number of Drones:")
								width: parent.width * 0.4
								height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							/**
							 * @brief Drone count input field container
							 * 
							 * This column contains the drone count input field and
							 * its associated error message. The layout ensures
							 * proper spacing and alignment of the input control.
							 */
							Column {
								width: parent.width * 0.5
								spacing: _h * 0.2
								
								/**
								 * @brief Drone count input field
								 * 
								 * This text field allows users to specify the
								 * number of drones for the mission. It includes
								 * validation to ensure the value is within
								 * acceptable bounds (1-50 drones).
								 * 
								 * @note The field automatically updates the
								 * backend areaPlanEditor.droneCount property
								 * when editing is finished and validation passes.
								 */
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
								
								/**
								 * @brief Drone count validation error display
								 * 
								 * This label displays validation errors for the
								 * drone count input field. It only appears when
								 * there are validation errors and uses the error
								 * color for immediate visual feedback.
								 */
								QGCLabel { 
									id: droneCountError
									color: parent.parent._err
									visible: text.length > 0
									text: ""
								}
							}

							/**
							 * @brief Altitude band start parameter label
							 * 
							 * This label describes the altitude band start input field.
							 * It specifies the starting altitude for altitude banding
							 * in meters, which determines the vertical separation
							 * between drones.
							 */
							QGCLabel {
								text: qsTr("Altitude Band Start (m):")
								width: parent.width * 0.4
								height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							/**
							 * @brief Altitude band start input field container
							 * 
							 * This column contains the altitude band start input field
							 * and its associated error message. The layout ensures
							 * proper spacing and alignment of the input control.
							 */
							Column {
								width: parent.width * 0.5
								spacing: _h * 0.2
								
								/**
								 * @brief Altitude band start input field
								 * 
								 * This text field allows users to specify the
								 * starting altitude for altitude banding in meters.
								 * It includes validation to ensure the value is
								 * within acceptable bounds (0-10000m).
								 * 
								 * @note The field automatically updates the
								 * backend areaPlanEditor.altitudeBandStart property
								 * when editing is finished and validation passes.
								 * 
								 * @note This field uses the generic missionAltitude
								 * validator but accepts any non-negative value.
								 */
								QGCTextField {
									id: bandStartField
									height: _h * 1.6
									text: areaPlanEditor ? areaPlanEditor.altitudeBandStart.toString() : "0"
									validator: DoubleValidator { bottom: 1; top: 10000; decimals: 1 }
									onEditingFinished: {
										if (areaPlanEditor && text !== "") {
											var err = _safeValidate("missionAltitude", parseFloat(text))
											// missionAltitude validator is generic numeric; accept >=0 here
											bandStartError.text = ""  // no hard error at UI level
											areaPlanEditor.setAltitudeBandStart(parseFloat(text))
										}
									}
								}
								
								/**
								 * @brief Altitude band start validation error display
								 * 
								 * This label displays validation errors for the
								 * altitude band start input field. It only appears
								 * when there are validation errors and uses the
								 * error color for immediate visual feedback.
								 * 
								 * @note Currently this field doesn't show hard
								 * errors at the UI level, only soft validation.
								 */
								QGCLabel { 
									id: bandStartError
									color: parent.parent._err
									visible: text.length > 0
									text: ""
								}
							}

							/**
							 * @brief Altitude band step parameter label
							 * 
							 * This label describes the altitude band step input field.
							 * It specifies the vertical separation between drones
							 * in meters, which determines how much altitude difference
							 * exists between adjacent drones in the formation.
							 */
							QGCLabel {
								text: qsTr("Altitude Band Step (m):")
								width: parent.width * 0.4
								height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}
							/**
							 * @brief Altitude band step input field container
							 * 
							 * This column contains the altitude band step input field
							 * and its associated error message. The layout ensures
							 * proper spacing and alignment of the input control.
							 */
							Column {
								width: parent.width * 0.5
								spacing: _h * 0.2
								
								/**
								 * @brief Altitude band step input field
								 * 
								 * This text field allows users to specify the
								 * vertical separation between drones in meters.
								 * It includes validation to ensure the value is
								 * within acceptable bounds (0.1-10000m).
								 * 
								 * @note The field automatically updates the
								 * backend areaPlanEditor.altitudeBandStep property
								 * when editing is finished and validation passes.
								 * 
								 * @note The minimum value of 0.1m ensures that
								 * drones have sufficient vertical separation
								 * for safe operation.
								 */
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
                                    text: areaPlanEditor ? areaPlanEditor.timeOffsetPerDrone.toString() : "1"
                                    validator: DoubleValidator { bottom: 1; top: 3600; decimals: 1 }
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
                                text: areaPlanEditor ? areaPlanEditor.perTargetSeparationS.toString() : "60"
                                validator: DoubleValidator { bottom: 1; top: 3600; decimals: 1 }
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
								text: qsTr("Takeoff Height (Meters):")
								width: parent.width * 0.4
                                height: _h * 1.6
								verticalAlignment: Text.AlignVCenter
							}

							QGCTextField {
                                width: parent.width * 0.6
                                height: _h * 1.6
								text: areaPlanEditor ? areaPlanEditor.takeoffHeight : 3.0
								placeholderText: qsTr("3.0")
								inputMethodHints: Qt.ImhFormattedNumbersOnly
								validator: DoubleValidator {
									bottom: 0.5
									top: 100.0
									decimals: 1
									notation: DoubleValidator.StandardNotation
								}
								onEditingFinished: {
									if (areaPlanEditor && text !== "") {
										var height = parseFloat(text)
										if (!isNaN(height)) {
											areaPlanEditor.setTakeoffHeight(height)
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
								text: areaPlanEditor ? areaPlanEditor.loiterTime : 2.0
								placeholderText: qsTr("2.0")
								inputMethodHints: Qt.ImhFormattedNumbersOnly
								validator: DoubleValidator {
									bottom: 1.0
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
                                text: qsTr("1")
                                validator: IntValidator { bottom: 1; top: 99 }
                            }
                            QGCButton {
                                text: qsTr("Insert")
                                width: parent.width * 0.3
                                height: parent.height
                                onClicked: {
                                    if (areaPlanEditor) {
                                        var idx = parseInt(droneIndexField.text) - 1
                                        if (!isNaN(idx) && idx >= 0) {
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
                                text: qsTr("1")
                                validator: IntValidator { bottom: 1; top: 99 }
                            }
                            QGCButton {
                                text: qsTr("Upload")
                                width: parent.width * 0.2
                                height: parent.height
                                onClicked: {
                                    if (areaPlanEditor) {
                                        var idx = parseInt(uploadDroneIndexField.text) - 1
                                        if (!isNaN(idx) && idx >= 0) {
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
                                    var planned = areaPlanEditor ? areaPlanEditor.droneCount : 0
                                    return qsTr("Mapped: %1/%2").arg(mappedCount).arg(planned)
                                }
                                width: parent.width * 0.33
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: (function(){
                                    var planned = areaPlanEditor ? areaPlanEditor.droneCount : 0
                                    return (mappedCount < planned) ? qgcPal.colorOrange : qgcPal.text
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

                                // Keyboard shortcuts: A (arm/disarm all), T (takeoff all), L (land all), S (stabilize)
                                // Guarded by confirmation dialog to avoid accidental activation while typing
                                Shortcut {
                                    sequence: "A"
                                    context: Qt.ApplicationShortcut
                                    onActivated: {
                                        // Determine desired state: if any mapped vehicle is not armed, arm all; otherwise disarm all
                                        var anyUnarmed = false
                                        for (var i = 0; i < waypointPreview.length; i++) {
                                            var d = waypointPreview[i]
                                            var veh = vehicleMapping[d.droneIndex]
                                            if (veh && !Boolean(veh.armed)) { anyUnarmed = true; break }
                                        }
                                        var doArm = anyUnarmed
                                        perDroneMapColumn.confirmAndRun(doArm ? qsTr("ARM all mapped vehicles?") : qsTr("DISARM all mapped vehicles?"), function(){
                                            for (var j = 0; j < waypointPreview.length; j++) {
                                                var dj = waypointPreview[j]
                                                var v = vehicleMapping[dj.droneIndex]
                                                if (v && areaPlanEditor) areaPlanEditor.armVehicle(v, doArm)
                                            }
                                            if (areaPlanEditor) areaPlanEditor.updateStatus(doArm ? qsTr("Armed all mapped vehicles") : qsTr("Disarmed all mapped vehicles"))
                                        })
                                    }
                                }
                                Shortcut {
                                    sequence: "T"
                                    context: Qt.ApplicationShortcut
                                    onActivated: {
                                        perDroneMapColumn.confirmAndRun(qsTr("TAKEOFF all mapped vehicles?"), function(){
                                            for (var i = 0; i < waypointPreview.length; i++) {
                                                var d = waypointPreview[i]
                                                var veh = vehicleMapping[d.droneIndex]
                                                if (veh && areaPlanEditor) areaPlanEditor.takeoffVehicle(veh, areaPlanEditor ? areaPlanEditor.takeoffHeight : 3)
                                            }
                                            if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Requested takeoff for all mapped vehicles"))
                                        })
                                    }
                                }
                                Shortcut {
                                    sequence: "L"
                                    context: Qt.ApplicationShortcut
                                    onActivated: {
                                        perDroneMapColumn.confirmAndRun(qsTr("LAND all mapped vehicles?"), function(){
                                            for (var i = 0; i < waypointPreview.length; i++) {
                                                var d = waypointPreview[i]
                                                var veh = vehicleMapping[d.droneIndex]
                                                if (veh && areaPlanEditor) areaPlanEditor.landVehicle(veh)
                                            }
                                            if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Requested land for all mapped vehicles"))
                                        })
                                    }
                                }
                                Shortcut {
                                    sequence: "S"
                                    context: Qt.ApplicationShortcut
                                    onActivated: {
                                        perDroneMapColumn.confirmAndRun(qsTr("Set STABILIZE mode for all mapped (if supported)?"), function(){
                                            for (var i = 0; i < waypointPreview.length; i++) {
                                                var d = waypointPreview[i]
                                                var veh = vehicleMapping[d.droneIndex]
                                                if (veh && areaPlanEditor && veh.flightModeSetAvailable && veh.flightModeSetAvailable()) {
                                                    // Attempt to set generic 'Stabilize' mode; may be ignored if firmware doesn't support
                                                    veh.setFlightMode("Stabilize")
                                                }
                                            }
                                            if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Requested STABILIZE flight mode for all mapped"))
                                        })
                                    }
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
                                    height: Math.max(_h * 1.4, mappingSummary.implicitHeight)
                                    spacing: _w
                                    QGCSwitch {
                                        id: lockMappingSwitch
                                        checked: false
                                        text: qsTr("Lock Mapping")
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Prevent accidental mapping changes")
                                    }
                                    QGCLabel {
                                        id: mappingSummary
                                        text: {
                                            function labelForVehicle(veh) {
                                                if (!veh) return qsTr("None")
                                                for (var i = 0; i < vehicleObjects.length; i++) {
                                                    if (vehicleObjects[i] === veh) return vehicleLabels[i]
                                                }
                                                return qsTr("Vehicle ?")
                                            }
                                            var lines = []
                                            var planned = areaPlanEditor ? areaPlanEditor.droneCount : 0
                                            for (var i = 0; i < planned; i++) {
                                                var veh = vehicleMapping[i]
                                                var label = labelForVehicle(veh)
                                                lines.push(qsTr("AC %1 → %2").arg(i+1).arg(label))
                                            }
                                            return lines.join("   ")
                                        }
                                        wrapMode: Text.WordWrap
                                        color: qgcPal.colorGrey
                                        width: parent.width * 0.85
                                        height: parent.height
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                // Build rows dynamically from preview (one row per drone)
                                Repeater {
                                    model: waypointPreview
                                    delegate: Row {
                                        width: parent.width
                                        height: _h * 1.6
                                        spacing: _w * 0.5

                                        QGCLabel {
                                            text: qsTr("Aircraft %1").arg(modelData.droneIndex + 1)
                                            width: parent.width * 0.10
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        // Vehicle selector (guarded for empty vehicle list)
                                        Item {
                                            width: parent.width * 0.25
                                            height: parent.height
                                            visible: true
                                            property int _vehCount: vehicleLabels.length
                                            QGCComboBox {
                                                id: vehicleCombo
                                                anchors.fill: parent
                                                visible: parent._vehCount > 0
                                                model: vehicleLabels
                                                // Reflect mapped vehicle selection in the dropdown
                                                currentIndex: (function(){
                                                    if (parent._vehCount <= 0) return -1
                                                    var mapped = vehicleMapping[modelData.droneIndex]
                                                    if (!mapped) return 0
                                                    for (var i = 0; i < vehicleObjects.length; i++) {
                                                        if (vehicleObjects[i] === mapped) return i
                                                    }
                                                    return 0
                                                })()
                                                enabled: !lockMappingSwitch.checked
                                                onActivated: {
                                                    if (currentIndex >= 0 && currentIndex < vehicleObjects.length) {
                                                        var veh = vehicleObjects[currentIndex]
                                                        // When unlocked, immediately apply mapping to avoid confusion
                                                        if (!lockMappingSwitch.checked) {
                                                            setMappingForDrone(modelData.droneIndex, veh)
                                                        }
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
                                            width: parent.width * 0.07
                                            height: parent.height
                                            enabled: (function(){
                                                if (lockMappingSwitch.checked) return false
                                                var count = vehicleLabels.length
                                                return count > 0 && vehicleCombo.currentIndex >= 0 && vehicleCombo.currentIndex < vehicleObjects.length
                                            })()
                                            onClicked: {
                                                if (vehicleCombo.currentIndex >= 0 && vehicleCombo.currentIndex < vehicleObjects.length) {
                                                    var veh = vehicleObjects[vehicleCombo.currentIndex]
                                                    var vid = (veh && typeof veh.id !== 'undefined') ? veh.id : "?"
                                                    var link = veh && veh.activeLinkName ? veh.activeLinkName : "?"
                                                    perDroneMapColumn.confirmAndRun(qsTr("Map Aircraft %1 to Vehicle %2 (%3)?").arg(modelData.droneIndex+1).arg(vid).arg(link), function(){
                                                        setMappingForDrone(modelData.droneIndex, veh)
                                                        if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Mapped Aircraft %1 to Vehicle %2").arg(modelData.droneIndex+1).arg(vid))
                                                    })
                                                }
                                            }
                                            Keys.onReturnPressed: clicked()
                                            Keys.onEnterPressed: clicked()
                                            focus: true
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Assign selected vehicle to this aircraft")
                                        }

                                        // Status summary (live via vehicle properties)
                                        Row {
                                            width: parent.width * 0.20
                                            height: parent.height
                                            spacing: _w * 0.5
                                            // Mapping chip
                                            Rectangle {
                                                radius: _w * 0.2
                                                height: parent.height * 0.8
                                                width: Math.max(_w * 8, mappingText.implicitWidth + _w)
                                                anchors.verticalCenter: parent.verticalCenter
                                                color: vehicleMapping[modelData.droneIndex] ? "#2E7D32" : "#C62828" // green/red
                                                border.color: qgcPal.windowShade
                                                QGCLabel {
                                                    id: mappingText
                                                    anchors.centerIn: parent
                                                    color: "white"
                                                    text: vehicleMapping[modelData.droneIndex] ? qsTr("Mapped") : qsTr("Not mapped")
                                                }
                                                ToolTip.visible: hovered
                                                ToolTip.text: vehicleMapping[modelData.droneIndex] ? qsTr("This aircraft is linked to a vehicle") : qsTr("Please select a vehicle for this aircraft")
                                            }
                                            // Mission upload chip
                                            Rectangle {
                                                radius: _w * 0.2
                                                height: parent.height * 0.8
                                                width: Math.max(_w * 10, missionText.implicitWidth + _w)
                                                anchors.verticalCenter: parent.verticalCenter
                                                color: uploadedMap[modelData.droneIndex] === true ? "#2E7D32" : "#EF6C00" // green/orange
                                                border.color: qgcPal.windowShade
                                                QGCLabel {
                                                    id: missionText
                                                    anchors.centerIn: parent
                                                    color: "white"
                                                    text: uploadedMap[modelData.droneIndex] === true ? qsTr("Mission ready") : qsTr("Upload needed")
                                                }
                                                ToolTip.visible: hovered
                                                ToolTip.text: uploadedMap[modelData.droneIndex] === true ? qsTr("Mission is on the vehicle") : qsTr("Upload this aircraft's mission to proceed")
                                            }
                                            // Vehicle state chip
                                            Rectangle {
                                                radius: _w * 0.2
                                                height: parent.height * 0.8
                                                width: Math.max(_w * 10, vehicleText.implicitWidth + _w)
                                                anchors.verticalCenter: parent.verticalCenter
                                                color: (function(){
                                                    var v = vehicleMapping[modelData.droneIndex]
                                                    if (!v) return "#757575" // grey
                                                    if (Boolean(v.flying)) return "#1976D2" // blue
                                                    if (Boolean(v.armed)) return "#8E24AA" // purple
                                                    return "#616161" // dark grey
                                                })()
                                                border.color: qgcPal.windowShade
                                                QGCLabel {
                                                    id: vehicleText
                                                    anchors.centerIn: parent
                                                    color: "white"
                                                    text: (function(){
                                                        var v = vehicleMapping[modelData.droneIndex]
                                                        if (!v) return qsTr("No vehicle")
                                                        if (Boolean(v.flying)) return qsTr("In flight")
                                                        if (Boolean(v.armed)) return qsTr("Armed")
                                                        return qsTr("On ground")
                                                    })()
                                                }
                                                ToolTip.visible: hovered
                                                ToolTip.text: (function(){
                                                    var v = vehicleMapping[modelData.droneIndex]
                                                    if (!v) return qsTr("No vehicle linked")
                                                    if (Boolean(v.flying)) return qsTr("Vehicle is flying")
                                                    if (Boolean(v.armed)) return qsTr("Vehicle is armed")
                                                    return qsTr("Vehicle is on the ground")
                                                })()
                                            }
                                        }
                                        // Upload status indicator
                                        Rectangle {
                                            width: _w * 1.5
                                            height: _w * 1.5
                                            radius: _w
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: uploadedMap[modelData.droneIndex] ? "#2E7D32" : qgcPal.windowShade
                                            border.color: qgcPal.colorGrey
                                            border.width: 1
                                            ToolTip.visible: hovered
                                            ToolTip.text: uploadedMap[modelData.droneIndex] ? qsTr("Mission uploaded") : qsTr("Mission not uploaded")
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
                                            var planned = areaPlanEditor ? areaPlanEditor.droneCount : 0
                                            if (!waypointPreview || waypointPreview.length === 0) {
                                                if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("No waypoints to upload"))
                                                return
                                            }
                                            if (mappedCount < planned) {
                                                if (areaPlanEditor) areaPlanEditor.updateStatus(qsTr("Map all aircraft before bulk upload"))
                                                return
                                            }
                                            var summary = []
                                            for (var i = 0; i < waypointPreview.length; i++) {
                                                var d = waypointPreview[i]
                                                var veh = vehicleMapping[d.droneIndex]
                                                if (!veh) { summary.push(qsTr("AC %1 → (none)").arg(d.droneIndex+1)); continue }
                                                var vid = (veh && typeof veh.id !== 'undefined') ? veh.id : "?"
                                                var link = veh && veh.activeLinkName ? veh.activeLinkName : "?"
                                                summary.push(qsTr("AC %1 → Vehicle %2 (%3)").arg(d.droneIndex+1).arg(vid).arg(link))
                                            }
                                            perDroneMapColumn.confirmAndRun(qsTr("Upload missions to:\n%1").arg(summary.join("\n")), function(){
                                                for (var i2 = 0; i2 < waypointPreview.length; i2++) {
                                                    var d2 = waypointPreview[i2]
                                                    var v2 = vehicleMapping[d2.droneIndex]
                                                    if (v2) areaPlanEditor.uploadPerDroneMissionToVehicle(d2.droneIndex, v2)
                                                }
                                            })
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
                                        width: parent.width * 0.08
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
                                            QGCLabel { text: qsTr("Aircraft %1").arg(modelData.droneIndex + 1); width: parent.width * 0.15; verticalAlignment: Text.AlignVCenter }
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
                                                    if (veh && areaPlanEditor) areaPlanEditor.takeoffVehicle(veh, areaPlanEditor ? areaPlanEditor.takeoffHeight : 3)
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
                                                    return _canStartMissionFor(veh) && (uploadedMap[modelData.droneIndex] === true)
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) {
                                                        perDroneMapColumn.confirmAndRun(qsTr("Start mission on vehicle %1? (auto takeoff)").arg(veh.id), function(){
                                                            if (veh.takeoffVehicleSupported === true && veh.flying !== true) {
                                                                areaPlanEditor.takeoffVehicle(veh, areaPlanEditor ? areaPlanEditor.takeoffHeight : 3)
                                                            }
                                                            areaPlanEditor.startMissionOnVehicle(veh)
                                                        })
                                                    }
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (uploadedMap[modelData.droneIndex] !== true) return qsTr("Upload mission first")
                                                    return qsTr("Cannot start: ensure health checks pass and altitude is safe")
                                                }
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
                                                text: qsTr("Continue")
                                                enabled: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    return !!veh && veh.flying === true && veh.pauseVehicleSupported === true
                                                }
                                                onClicked: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (veh && areaPlanEditor) areaPlanEditor.continueMissionOnVehicle(veh)
                                                }
                                                ToolTip.visible: hovered && !enabled
                                                ToolTip.text: {
                                                    var veh = vehicleMapping[modelData.droneIndex]
                                                    if (!veh) return qsTr("No vehicle mapped")
                                                    if (veh.flying !== true) return qsTr("Vehicle not flying")
                                                    if (veh.pauseVehicleSupported !== true) return qsTr("Continue not supported")
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
						
						QGCButton {
							text: qsTr("Continue Mission")
							width: parent.width
                            height: _h * 2.2
							enabled: {
								var vehicle = QGroundControl.multiVehicleManager.activeVehicle
								return vehicle && vehicle.flying === true && vehicle.pauseVehicleSupported === true
							}
							onClicked: {
								var vehicle = QGroundControl.multiVehicleManager.activeVehicle
								if (vehicle && areaPlanEditor) areaPlanEditor.continueMissionOnVehicle(vehicle)
							}
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
