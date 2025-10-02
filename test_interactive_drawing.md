# Area Plan Editor Interactive Drawing Test Plan

## Overview
This test plan covers the interactive drawing functionality for the Area Plan Editor in QGroundControl. The component allows users to set a center point, draw areas, and generate waypoints for mission planning.

## Key Components
- **AreaPlanEditor.qml**: Main UI component with controls and buttons
- **AreaPlanMapVisuals.qml**: Map visualization component for area rectangle, center marker, grid lines, and waypoints
- **AreaPlanEditor.h/cc**: C++ backend for calculations and mission generation
- **PlanView.qml**: Integration with QGC's mission planning system

## Recent Fixes Applied
1. **Fixed TypeError**: Removed problematic `MissionItemIndicatorDrag` usage that was causing `TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed`
2. **Re-implemented Direct MouseArea Dragging**: Added `MouseArea` components directly to rectangle and center marker for proper dragging functionality
3. **Removed Debug Rectangle**: Removed the temporary debug rectangle that was interfering with visibility
4. **Enhanced Mission Generation**: Added `addWaypointsToMission()` method to better integrate with QGC's mission system
5. **Improved Error Handling**: Enhanced logging and validation throughout the system

## Test Categories

### 1. Basic Functionality Tests

#### 1.1 Component Loading
- [ ] **Test**: Verify AreaPlanEditor.qml loads without errors
- [ ] **Test**: Verify AreaPlanMapVisuals.qml loads without errors
- [ ] **Test**: Verify C++ backend (AreaPlanEditor) is accessible from QML
- [ ] **Expected**: No console errors, all components visible in UI

#### 1.2 Property Access
- [ ] **Test**: Click "Test C++ Backend" button
- [ ] **Test**: Verify all properties are readable and writable
- [ ] **Test**: Check that default values are set correctly
- [ ] **Expected**: Console logs show valid property values, no null errors

### 2. Interactive Drawing Tests

#### 2.1 Drawing Mode Toggle
- [ ] **Test**: Click "Start Drawing Mode" button
- [ ] **Test**: Verify button text changes to "Stop Drawing Mode"
- [ ] **Test**: Verify status indicator shows "DRAWING MODE ACTIVE"
- [ ] **Test**: Click again to stop drawing mode
- [ ] **Expected**: Button text and status indicator update correctly

#### 2.2 Center Point Setting
- [ ] **Test**: Enable drawing mode
- [ ] **Test**: Click on map to set center point
- [ ] **Test**: Verify center marker appears at clicked location
- [ ] **Test**: Verify center marker is visible and properly sized
- [ ] **Expected**: Center marker appears as red circle with white cross, positioned at clicked location

#### 2.3 Area Rectangle Visibility
- [ ] **Test**: Set center point
- [ ] **Test**: Verify area rectangle appears around center
- [ ] **Test**: Check rectangle opacity and border width
- [ ] **Test**: Verify rectangle corners are calculated correctly
- [ ] **Expected**: Red rectangle with 70% opacity and 5px border, covering the defined area

#### 2.4 Area Resizing
- [ ] **Test**: Click and drag on map after setting center
- [ ] **Test**: Verify area dimensions update in real-time
- [ ] **Test**: Check that area width and height change appropriately
- [ ] **Expected**: Area rectangle resizes smoothly as you drag

### 3. Dragging Functionality Tests

#### 3.1 Center Marker Dragging
- [ ] **Test**: Click and drag the center marker (red circle with cross)
- [ ] **Test**: Verify center marker moves with mouse
- [ ] **Test**: Check that area rectangle moves with center
- [ ] **Test**: Verify hover effects (scaling) work
- [ ] **Expected**: Center marker is draggable, area follows center movement

#### 3.2 Rectangle Dragging
- [ ] **Test**: Click and drag anywhere on the area rectangle
- [ ] **Test**: Verify entire area moves with drag
- [ ] **Test**: Check that center marker moves with rectangle
- [ ] **Test**: Verify hover effects (opacity change) work
- [ ] **Expected**: Rectangle is draggable, center marker follows

### 4. Grid Lines and Waypoints Tests

#### 4.1 Grid Lines Visibility
- [ ] **Test**: Set area center and dimensions
- [ ] **Test**: Verify green grid lines appear within rectangle
- [ ] **Test**: Check line spacing matches input value
- [ ] **Test**: Verify lines are properly positioned
- [ ] **Expected**: Green lines spaced according to lineSpacing parameter

#### 4.2 Waypoint Markers
- [ ] **Test**: Set area parameters (width, height, line spacing, num points)
- [ ] **Test**: Verify green waypoint markers appear at calculated positions
- [ ] **Test**: Check marker count matches expected total
- [ ] **Test**: Verify markers are properly sized and visible
- [ ] **Expected**: Green circular markers with white centers at waypoint positions

### 5. Mission Generation Tests

#### 5.1 Generate Waypoints Button
- [ ] **Test**: Set up area with valid parameters
- [ ] **Test**: Click "Generate Waypoints" button
- [ ] **Test**: Verify console logs show waypoint generation
- [ ] **Test**: Check that waypoint count matches expected total
- [ ] **Expected**: Console shows "Generated X waypoints - ready for mission"

#### 5.2 Mission Integration
- [ ] **Test**: Generate waypoints
- [ ] **Test**: Check if waypoints appear on first tab (Mission tab)
- [ ] **Test**: Verify waypoints are properly formatted for QGC
- [ ] **Expected**: Waypoints should be visible in mission list (if properly integrated)

#### 5.3 Save Mission File
- [ ] **Test**: Generate waypoints
- [ ] **Test**: Click "Save Mission File" button
- [ ] **Test**: Verify file is created in expected location
- [ ] **Test**: Check file format is correct
- [ ] **Expected**: Mission file saved as "area_mission.waypoints"

### 6. Reset and Debug Tests

#### 6.1 Reset Area Button
- [ ] **Test**: Modify area parameters
- [ ] **Test**: Click "Reset Area" button
- [ ] **Test**: Verify all parameters return to defaults
- [ ] **Test**: Check that map items are cleared and recreated
- [ ] **Expected**: All parameters reset to default values, map items update

#### 6.2 Debug Buttons
- [ ] **Test**: Click "Test Mission Generation" button
- [ ] **Test**: Verify detailed logging of mission generation process
- [ ] **Test**: Click "Debug: Force Map Items" button
- [ ] **Test**: Check that map items are forced to update
- [ ] **Expected**: Console logs show detailed information about generation process

### 7. Step-by-Step Flow Tests

#### 7.1 Flow Indicator
- [ ] **Test**: Verify step indicators show current progress
- [ ] **Test**: Check that steps update as you complete them
- [ ] **Test**: Verify visual feedback for completed steps
- [ ] **Expected**: Green indicators for completed steps, grey for incomplete

#### 7.2 Complete Workflow
- [ ] **Test**: Follow the complete workflow:
  1. Set center point (should turn green)
  2. Define area size (should turn green)
  3. Generate waypoints (should turn green)
  4. Save mission (should turn green)
- [ ] **Expected**: All steps complete successfully, indicators show progress

### 8. Error Handling Tests

#### 8.1 Invalid Parameters
- [ ] **Test**: Set negative values for area dimensions
- [ ] **Test**: Set invalid coordinates
- [ ] **Test**: Try to generate waypoints with invalid parameters
- [ ] **Expected**: Proper error messages, validation prevents invalid operations

#### 8.2 Missing Dependencies
- [ ] **Test**: Try to use features without setting center
- [ ] **Test**: Try to generate waypoints without valid area
- [ ] **Expected**: Clear error messages guide user to complete required steps

### 9. Performance Tests

#### 9.1 Large Area Generation
- [ ] **Test**: Set large area dimensions (e.g., 1000m x 1000m)
- [ ] **Test**: Set small line spacing (e.g., 5m)
- [ ] **Test**: Set high number of points (e.g., 20)
- [ ] **Test**: Generate waypoints
- [ ] **Expected**: System handles large waypoint sets without freezing

#### 9.2 Responsive UI
- [ ] **Test**: Drag center marker rapidly
- [ ] **Test**: Resize area rapidly
- [ ] **Test**: Toggle drawing mode rapidly
- [ ] **Expected**: UI remains responsive, no lag or freezing

### 10. Integration Tests

#### 10.1 Map Integration
- [ ] **Test**: Verify map clicks work correctly
- [ ] **Test**: Check that map zoom and pan don't interfere
- [ ] **Test**: Verify map items appear in correct z-order
- [ ] **Expected**: Map integration works seamlessly

#### 10.2 QGC Integration
- [ ] **Test**: Switch between different tabs
- [ ] **Test**: Verify Area Plan tab works independently
- [ ] **Test**: Check that other QGC features remain functional
- [ ] **Expected**: Area Plan Editor integrates properly with QGC

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue: Rectangle not visible
- **Check**: Area dimensions are > 0
- **Check**: Center coordinate is valid
- **Check**: Console for "Calculated corners" logs
- **Solution**: Use "Test C++ Backend" to set default values

#### Issue: Center marker not draggable
- **Check**: Interactive property is true
- **Check**: MouseArea is properly attached
- **Check**: Console for "Center marker pressed" logs
- **Solution**: Verify component structure in AreaPlanMapVisuals.qml

#### Issue: Waypoints not appearing on first tab
- **Check**: Mission controller integration
- **Check**: Console for waypoint generation logs
- **Check**: Mission file is created
- **Solution**: Use "Generate Waypoints" button and check console output

#### Issue: TypeError in QGCDynamicObjectManager
- **Check**: No longer using MissionItemIndicatorDrag incorrectly
- **Check**: Direct MouseArea implementation is used
- **Solution**: Fixed in recent update - should not occur anymore

#### Issue: Map items not updating
- **Check**: Property change signals are connected
- **Check**: addMapItems() is called when properties change
- **Check**: Console for "Adding map items" logs
- **Solution**: Use "Debug: Force Map Items" button

### Debug Commands
- **Console Logs**: Check for detailed logging of all operations
- **Debug Buttons**: Use provided debug buttons to test functionality
- **Property Monitoring**: Watch for property change signals
- **Error Messages**: Look for validation and error messages

## Expected Results Summary

### Visual Elements
- ✅ Red center marker with white cross (draggable)
- ✅ Red area rectangle with 70% opacity and 5px border (draggable)
- ✅ Green grid lines within rectangle
- ✅ Green waypoint markers at calculated positions
- ✅ Step-by-step flow indicators
- ✅ Status indicators for drawing mode

### Functionality
- ✅ Interactive drawing mode toggle
- ✅ Center point setting via map clicks
- ✅ Area resizing via map clicks
- ✅ Dragging of center marker and rectangle
- ✅ Real-time property updates
- ✅ Waypoint generation and logging
- ✅ Mission file saving
- ✅ Reset functionality
- ✅ Error handling and validation

### Integration
- ✅ QML-C++ communication
- ✅ Map integration
- ✅ QGC architecture compliance
- ✅ Performance optimization
- ✅ Debug and testing tools

## Test Execution Notes

1. **Start with Basic Tests**: Begin with component loading and property access tests
2. **Proceed to Interactive Tests**: Test drawing mode and center point setting
3. **Test Visual Elements**: Verify rectangle, center marker, grid lines, and waypoints
4. **Test Dragging**: Verify both center marker and rectangle dragging
5. **Test Mission Generation**: Verify waypoint generation and mission integration
6. **Test Error Conditions**: Verify proper error handling
7. **Test Performance**: Verify system handles large datasets
8. **Test Integration**: Verify QGC integration works properly

## Success Criteria

The Area Plan Editor is considered fully functional when:
- All visual elements are visible and properly positioned
- Interactive drawing works smoothly without errors
- Dragging functionality works for both center marker and rectangle
- Waypoint generation produces valid results
- Mission integration works (waypoints appear on first tab)
- Error handling provides clear feedback
- Performance is acceptable for typical use cases
- Integration with QGC is seamless 