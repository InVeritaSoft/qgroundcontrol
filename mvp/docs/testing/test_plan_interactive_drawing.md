# Interactive Drawing Functionality Test Plan

## Test Environment Setup
- **QGroundControl Version**: Latest build with AreaPlanEditor integration
- **Test Mode**: `testMode: true` in AreaPlanMapVisuals.qml (enables MouseArea for testing)
- **Console Logging**: All debug logs enabled
- **Visual Indicators**: Blue border/background when test mode active

## Test Categories

### 1. Component Initialization Tests

#### 1.1 AreaPlanEditor C++ Backend
- [ ] **Test**: Verify `QGroundControl.areaPlanEditor` is accessible from QML
- [ ] **Expected**: `areaPlanEditor` object is not null
- [ ] **Console Check**: No "Type AreaPlanEditor unavailable" errors
- [ ] **Method**: Check in AreaPlanEditor.qml `Component.onCompleted`

#### 1.2 AreaPlanMapVisuals QML Component
- [ ] **Test**: Verify `mapControl` property is properly passed
- [ ] **Expected**: `mapControlValid: true` in debug properties
- [ ] **Console Check**: "MouseArea enabled changed to: true" on startup
- [ ] **Visual Check**: Blue border appears around map area

#### 1.3 Property Bindings
- [ ] **Test**: Verify `isDrawingMode` property binding from C++ to QML
- [ ] **Expected**: Button text changes when drawing mode toggled
- [ ] **Console Check**: "AreaPlanMapVisuals: isDrawingMode changed to: true/false"
- [ ] **Visual Check**: Red border appears when drawing mode active

### 2. MouseArea Functionality Tests

#### 2.1 MouseArea Event Reception
- [ ] **Test**: Click anywhere on map in test mode
- [ ] **Expected**: Console logs "MouseArea pressed" and mouse coordinates
- [ ] **Console Check**: 
  ```
  MouseArea pressed - interactive: true isDrawingMode: false mapControl: true
  Mouse position: [x] [y]
  Test mode - Raw mouse position: [x] [y]
  Test mode - Converted coordinate: [lat], [lon]
  ```
- [ ] **Failure Mode**: If no logs appear, MouseArea is not receiving events

#### 2.2 Coordinate Conversion
- [ ] **Test**: Verify `mapControl.toCoordinate()` works correctly
- [ ] **Expected**: Valid coordinates returned for map clicks
- [ ] **Console Check**: "Converted coordinate: [valid lat/lon]" not "invalid"
- [ ] **Failure Mode**: If "invalid" appears, coordinate conversion is broken

#### 2.3 MouseArea Z-Order
- [ ] **Test**: Verify MouseArea is on top of other map elements
- [ ] **Expected**: MouseArea receives events even when map items are present
- [ ] **Method**: Click on areas with existing map items
- [ ] **Console Check**: Mouse events still logged when clicking over map items

### 3. Drawing Mode Tests

#### 3.1 Drawing Mode Toggle
- [ ] **Test**: Click "Start Drawing Mode" button
- [ ] **Expected**: 
  - Button text changes to "Stop Drawing Mode"
  - Visual indicator changes to red
  - Console logs drawing mode change
- [ ] **Console Check**:
  ```
  Drawing mode button clicked - current mode: false new mode: true
  Drawing mode set to: true
  AreaPlanMapVisuals: isDrawingMode changed to: true
  ```

#### 3.2 Drawing Mode Visual Feedback
- [ ] **Test**: Verify visual indicators when drawing mode active
- [ ] **Expected**: 
  - Red border around map area
  - "DRAWING MODE" text overlay
  - Cross cursor when hovering over map
- [ ] **Visual Check**: Red border and text appear when drawing mode enabled

#### 3.3 Drawing Mode Mouse Events
- [ ] **Test**: Click on map when drawing mode active
- [ ] **Expected**: 
  - First click sets area center
  - Subsequent clicks start drag operation
- [ ] **Console Check**:
  ```
  Mouse pressed at: [lat] [lon]
  Area center set to: [lat] [lon]
  ```

### 4. Area Center Setting Tests

#### 4.1 Initial Center Point
- [ ] **Test**: Click on map when no center is set
- [ ] **Expected**: 
  - `areaPlanEditor.setAreaCenter()` called
  - Center marker appears on map
  - Area rectangle appears
- [ ] **Console Check**: "Area center set to: [lat] [lon]"
- [ ] **Visual Check**: Red center marker appears at clicked location

#### 4.2 Center Point Validation
- [ ] **Test**: Verify center point is properly stored
- [ ] **Expected**: 
  - `areaPlanEditor.areaCenter.isValid` returns true
  - Coordinates match clicked location
- [ ] **Method**: Check area center coordinates in UI statistics

### 5. Area Resizing Tests

#### 5.1 Drag Operation
- [ ] **Test**: Click and drag after center is set
- [ ] **Expected**: 
  - `isDragging` becomes true
  - Area dimensions update in real-time
  - Rectangle size changes on map
- [ ] **Console Check**:
  ```
  Started dragging from: [lat] [lon]
  Drag distance: [meters] bearing: [degrees]
  Updated area size: [width] x [height]
  ```

#### 5.2 Real-time Updates
- [ ] **Test**: Verify area updates during drag
- [ ] **Expected**: 
  - Rectangle size changes as mouse moves
  - Grid lines update automatically
  - Waypoint markers update
- [ ] **Visual Check**: Rectangle and grid update smoothly during drag

#### 5.3 Drag Constraints
- [ ] **Test**: Verify minimum and maximum size constraints
- [ ] **Expected**: 
  - Minimum size: 10 meters
  - Maximum size: 1000 meters
- [ ] **Method**: Try dragging to very small/large areas

### 6. Map Visualization Tests

#### 6.1 Area Rectangle Display
- [ ] **Test**: Verify area rectangle appears correctly
- [ ] **Expected**: 
  - Red semi-transparent rectangle on map
  - Rectangle corners calculated correctly
  - Rectangle updates when area parameters change
- [ ] **Visual Check**: Rectangle appears and updates properly

#### 6.2 Grid Lines Display
- [ ] **Test**: Verify grid lines appear within area
- [ ] **Expected**: 
  - Green lines parallel to area width
  - Lines spaced according to `lineSpacing` parameter
  - Lines update when area changes
- [ ] **Visual Check**: Grid lines appear and update

#### 6.3 Waypoint Markers
- [ ] **Test**: Verify waypoint markers appear
- [ ] **Expected**: 
  - Green circular markers at calculated positions
  - Markers update when parameters change
  - Hover effects work
- [ ] **Visual Check**: Waypoint markers appear and respond to hover

### 7. Integration Tests

#### 7.1 C++ to QML Communication
- [ ] **Test**: Verify property changes propagate correctly
- [ ] **Expected**: 
  - UI updates when C++ properties change
  - Map visualizations update automatically
  - No binding errors in console
- [ ] **Method**: Change area parameters and verify updates

#### 7.2 Mission Generation
- [ ] **Test**: Verify waypoint generation works
- [ ] **Expected**: 
  - `generateWaypoints()` returns valid waypoint list
  - Waypoints match visual markers
  - Mission statistics update correctly
- [ ] **Console Check**: No errors during waypoint generation

### 8. Error Handling Tests

#### 8.1 Invalid Coordinates
- [ ] **Test**: Handle invalid coordinate conversion
- [ ] **Expected**: 
  - Graceful handling of invalid coordinates
  - Error messages logged
  - No crashes or freezes
- [ ] **Console Check**: "Invalid coordinate conversion" logged

#### 8.2 Missing Components
- [ ] **Test**: Handle missing mapControl or areaPlanEditor
- [ ] **Expected**: 
  - Graceful degradation
  - Error messages logged
  - No crashes
- [ ] **Console Check**: Appropriate error messages

### 9. Performance Tests

#### 9.1 Real-time Updates
- [ ] **Test**: Verify smooth updates during drag
- [ ] **Expected**: 
  - No lag during mouse movement
  - Smooth visual updates
  - Console logs not overwhelming
- [ ] **Method**: Drag area and observe smoothness

#### 9.2 Large Areas
- [ ] **Test**: Handle large area calculations
- [ ] **Expected**: 
  - No performance issues with large areas
  - Grid calculations complete quickly
  - Memory usage reasonable
- [ ] **Method**: Create large area and verify performance

## Test Execution Instructions

### Step 1: Enable Test Mode
1. Open `src/QmlControls/AreaPlanMapVisuals.qml`
2. Ensure `property bool testMode: true`
3. Build and run QGroundControl

### Step 2: Open Console
1. Open browser developer tools (if using web version)
2. Or use QGroundControl's built-in console
3. Clear console and prepare to monitor logs

### Step 3: Execute Tests
1. **Component Initialization**: Check for startup logs
2. **MouseArea Testing**: Click on map and verify logs
3. **Drawing Mode**: Toggle drawing mode and verify changes
4. **Area Setting**: Click to set center point
5. **Area Resizing**: Drag to resize area
6. **Visual Updates**: Verify all visual elements update

### Step 4: Document Results
- [ ] **Pass**: All expected behaviors work
- [ ] **Partial**: Some features work, others need fixing
- [ ] **Fail**: Major functionality broken, needs investigation

## Expected Console Output (Successful Test)

```
MouseArea enabled changed to: true interactive: true isDrawingMode: false
AreaPlanMapVisuals: isDrawingMode changed to: false
MouseArea enabled: false mapControl valid: true

// Click on map (test mode)
MouseArea pressed - interactive: true isDrawingMode: false mapControl: true
Mouse position: 400 300
Test mode - Raw mouse position: 400 300
Test mode - Converted coordinate: 37.123456, -122.654321

// Enable drawing mode
Drawing mode button clicked - current mode: false new mode: true
Drawing mode set to: true
AreaPlanMapVisuals: isDrawingMode changed to: true
MouseArea enabled: true mapControl valid: true

// Click to set center
MouseArea pressed - interactive: true isDrawingMode: true mapControl: true
Mouse position: 400 300
Raw mouse position: 400 300
Converted coordinate: 37.123456, -122.654321
Mouse pressed at: 37.123456 -122.654321
Area center set to: 37.123456 -122.654321

// Drag to resize
Started dragging from: 37.123456 -122.654321
Drag distance: 150.5 bearing: 45.2
Updated area size: 301.0 x 301.0
Drag finished
```

## Failure Analysis

### Common Issues and Solutions

#### Issue 1: No MouseArea Events
**Symptoms**: No console logs when clicking on map
**Possible Causes**:
- MouseArea not enabled
- Z-order issues
- Parent component blocking events
**Solutions**:
- Check `enabled` property
- Verify `z` property is high enough
- Check parent component event handling

#### Issue 2: Invalid Coordinate Conversion
**Symptoms**: "Converted coordinate: invalid" in logs
**Possible Causes**:
- `mapControl` is null
- `toCoordinate()` method not working
- Map not properly initialized
**Solutions**:
- Verify `mapControl` is passed correctly
- Check map initialization
- Verify coordinate system

#### Issue 3: Drawing Mode Not Working
**Symptoms**: Button doesn't change state or visual indicators don't appear
**Possible Causes**:
- Property binding issues
- C++ backend not responding
- QML property not updating
**Solutions**:
- Check property bindings
- Verify C++ setter method
- Check signal connections

#### Issue 4: Visual Elements Not Appearing
**Symptoms**: No rectangle, grid lines, or waypoints visible
**Possible Causes**:
- Map items not being created
- Coordinate calculations wrong
- Z-order issues
**Solutions**:
- Check `addMapItems()` function
- Verify coordinate calculations
- Check map item creation

## Next Steps After Testing

1. **If All Tests Pass**: Interactive drawing is working correctly
2. **If Some Tests Fail**: Document specific failures and fix issues
3. **If Major Issues Found**: Revert to simpler implementation and debug step by step
4. **Performance Issues**: Optimize calculations and reduce update frequency

This comprehensive test plan will help identify exactly where the interactive drawing functionality is working or failing, allowing for targeted fixes and improvements. 