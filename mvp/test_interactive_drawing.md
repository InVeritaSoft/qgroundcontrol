# Interactive Drawing Test Plan - Updated

## Test Environment Setup
1. Build and run QGroundControl
2. Navigate to Plan View
3. Select the "Area" tab
4. Ensure the Area Plan Editor panel is visible

## Test 1: Basic Functionality Verification

### 1.1 Component Loading
- [ ] Area Plan Editor panel loads without errors
- [ ] C++ backend is accessible (check console for "AreaPlanEditor backend: true")
- [ ] Default values are set (100x100m area, 10m line spacing, 1 point)
- [ ] Console shows "AreaPlanMapVisuals: Component completed"

### 1.2 Debug Button Test
- [ ] Click "Debug: Force Map Items" button
- [ ] Verify console shows current area state
- [ ] Check that property updates are triggered
- [ ] Verify map items are created/updated

### 1.3 Reset Button
- [ ] Click "Reset Area" button
- [ ] Verify console shows "Reset button clicked" and "Area reset completed"
- [ ] Check that all values reset to defaults
- [ ] Verify area center is set to default coordinates

## Test 2: Drawing Mode

### 2.1 Start Drawing Mode
- [ ] Click "Start Drawing Mode" button
- [ ] Verify button text changes to "Stop Drawing Mode"
- [ ] Check status indicator shows "DRAWING MODE ACTIVE" in red
- [ ] Verify console shows "isDrawingMode changed to: true"

### 2.2 Map Interaction
- [ ] With drawing mode active, click on the map
- [ ] Verify console shows "Area Plan: Map clicked at [coordinates]"
- [ ] Check that area center is set to clicked location
- [ ] Verify default area size (100x100m) is set

### 2.3 Area Visualization
- [ ] After setting center, verify red rectangle appears on map
- [ ] Check that rectangle has red border and semi-transparent red interior
- [ ] Verify center marker (red circle with white cross) appears
- [ ] Confirm rectangle corners are calculated correctly (console logs)

## Test 3: Interactive Drawing

### 3.1 Center Marker Dragging
- [ ] Click and drag the center marker (red circle)
- [ ] Verify console shows "Center marker pressed" and "Dragging center to [coordinates]"
- [ ] Check that area center updates in real-time
- [ ] Verify rectangle moves with the center marker

### 3.2 Rectangle Dragging
- [ ] Click and drag the rectangle itself
- [ ] Verify console shows "Rectangle pressed" and "Dragging rectangle to [coordinates]"
- [ ] Check that area center updates in real-time
- [ ] Verify entire area moves with the rectangle

### 3.3 Area Resizing
- [ ] With drawing mode active, click on map away from center
- [ ] Verify area size updates based on distance from center
- [ ] Check console shows "Updated area size: [width] x [height] meters"
- [ ] Confirm rectangle resizes accordingly

### 3.4 Grid Lines and Waypoints
- [ ] After setting area, verify green grid lines appear
- [ ] Check that waypoint markers (green circles) appear at intersections
- [ ] Verify number of lines and points matches area parameters

## Test 4: Property Updates

### 4.1 Manual Parameter Changes
- [ ] Change area width/height in text inputs
- [ ] Verify rectangle updates immediately
- [ ] Check that grid lines and waypoints update
- [ ] Confirm no duplication of map items

### 4.2 Line Spacing Changes
- [ ] Modify line spacing value
- [ ] Verify grid lines update with new spacing
- [ ] Check waypoint positions recalculate

### 4.3 Number of Points Changes
- [ ] Change number of points
- [ ] Verify waypoint markers update
- [ ] Check total waypoint count calculation

## Test 5: Mission Generation

### 5.1 Waypoint Generation
- [ ] Click "Test Mission Generation" button
- [ ] Verify console shows waypoint generation process
- [ ] Check that waypoints are calculated correctly
- [ ] Confirm mission file is saved

### 5.2 Mission Validation
- [ ] Verify generated waypoints match visible markers
- [ ] Check mission file contains correct coordinates
- [ ] Confirm mission can be loaded in QGC

## Test 6: Error Handling

### 6.1 Invalid Inputs
- [ ] Enter negative values for dimensions
- [ ] Verify validation errors are shown
- [ ] Check that invalid inputs don't break visualization

### 6.2 Edge Cases
- [ ] Test with very small area (1x1m)
- [ ] Test with very large area (1000x1000m)
- [ ] Verify system handles extreme values gracefully

## Test 7: Performance

### 7.1 Smooth Interaction
- [ ] Drag center marker smoothly
- [ ] Drag rectangle smoothly
- [ ] Verify no lag or stuttering
- [ ] Check that updates are responsive

### 7.2 Memory Management
- [ ] Repeatedly change area parameters
- [ ] Verify no memory leaks (check console for errors)
- [ ] Confirm old map items are properly cleaned up

## Test 8: Visual Feedback

### 8.1 Hover Effects
- [ ] Hover over center marker
- [ ] Verify it scales up and changes color
- [ ] Hover over rectangle
- [ ] Verify it changes opacity
- [ ] Check smooth animations

### 8.2 Status Indicators
- [ ] Verify step-by-step flow shows progress
- [ ] Check that current step is highlighted
- [ ] Confirm all steps complete when area is fully configured

## Expected Results

### Console Output Examples
```
AreaPlanEditor: Component completed
AreaPlanEditor backend: true
AreaPlanEditor properties: areaWidth: 100, areaHeight: 100, isDrawingMode: false
AreaPlanMapVisuals: Component completed
AreaPlanMapVisuals: Forcing initial map item creation
AreaPlanMapVisuals: Adding map items
AreaPlanMapVisuals: Rectangle corners count: 4
AreaPlanMapVisuals: Area rectangle created: true
AreaPlanMapVisuals: Center marker created: true
Area Plan: Map clicked at 49.82824897481479 24.033390804256005
Area center set to: 49.82824897481479 24.033390804256005
```

### Visual Elements
- Red rectangle with 5px border and 70% opacity
- Red center marker with white cross (3x font height)
- Green grid lines (3px width)
- Green waypoint markers (1.5x font height)

### Interactive Behavior
- Center marker draggable with hover effects
- Rectangle draggable with hover effects
- Map clicks set center or resize area
- Real-time updates without duplication
- Smooth animations and transitions

## Troubleshooting

### If Rectangle Not Visible
1. Check console for "Rectangle corners count: 4"
2. Verify area center is valid
3. Confirm area dimensions > 0
4. Check that map items are being created
5. Use "Debug: Force Map Items" button
6. Check map zoom level (rectangle might be too small)

### If Center Marker Not Visible
1. Verify center marker is created (console logs)
2. Check center marker coordinate is valid
3. Verify center marker is visible property
4. Check z-order and positioning

### If Rectangle/Center Not Draggable
1. Verify MouseArea is properly configured
2. Check that mapControl is valid
3. Confirm coordinate conversion is working
4. Look for console errors in onPositionChanged

### If UI Changes Don't Update Map
1. Check that property change signals are emitted
2. Verify Connections are working
3. Check that shouldUpdateMapItems is triggered
4. Use debug button to force updates

### If Duplication Occurs
1. Check that removeMapItems() is called before addMapItems()
2. Verify separate object managers for different item types
3. Confirm destroyObjects() is working properly

### If Reset Not Working
1. Verify resetArea() method is implemented in C++
2. Check that all properties are reset to defaults
3. Confirm signals are emitted properly
4. Look for console errors in reset button handler

### If Mission Generation Not Working
1. Check that generateWaypoints() returns valid data
2. Verify saveMissionFile() is called
3. Check file permissions for mission file writing
4. Verify all parameters are valid before generation

## Debug Commands

Use these console commands to debug:
```javascript
// Check area properties
console.log("Width:", areaPlanEditor.areaWidth)
console.log("Height:", areaPlanEditor.areaHeight)
console.log("Center:", areaPlanEditor.areaCenter.latitude, areaPlanEditor.areaCenter.longitude)

// Test waypoint generation
var waypoints = areaPlanEditor.generateWaypoints()
console.log("Generated:", waypoints.length, "waypoints")

// Reset to defaults
areaPlanEditor.resetArea()

// Force map updates
areaPlanEditor.setAreaWidth(areaPlanEditor.areaWidth + 0.1)
areaPlanEditor.setAreaWidth(areaPlanEditor.areaWidth - 0.1)
``` 