# AreaPlanEditor Controls List

This document provides a comprehensive list of all controls and parameters available in the QGroundControl AreaPlanEditor system for multi-drone area planning and mission coordination.

## **Area Configuration Controls**

### **Basic Area Parameters**
- **Area Width** (`QGCTextField`) - Width of the planning area in meters
- **Area Height** (`QGCTextField`) - Height of the planning area in meters  
- **Line Spacing** (`QGCTextField`) - Distance between survey lines in meters
- **Number of Points** (`QGCTextField`) - Points per survey line
- **Mission Altitude** (`QGCTextField`) - Flight altitude in meters
- **Area Rotation** (`QGCTextField`) - Rotation angle in degrees (0-360°)

### **Area Positioning Controls**
- **North Movement** (`QGCButton`) - Move area north
- **South Movement** (`QGCButton`) - Move area south  
- **East Movement** (`QGCButton`) - Move area east
- **West Movement** (`QGCButton`) - Move area west
- **Center Area** (`QGCButton`) - Center area on current location
- **Rotate Clockwise** (`QGCButton`) - Rotate +15°
- **Rotate Counterclockwise** (`QGCButton`) - Rotate -15°
- **Reset Rotation** (`QGCButton`) - Reset to 0°

## **Multi-Drone Configuration Controls**

### **Drone Setup**
- **Drone Count** (`QGCTextField`) - Number of drones in the swarm
- **Altitude Band Start** (`QGCTextField`) - Starting altitude offset in meters
- **Altitude Band Step** (`QGCTextField`) - Altitude increment per drone in meters
- **Time Offset Per Drone** (`QGCTextField`) - Stagger time between drones in seconds

### **Mission Policies**
- **RTL After Every Waypoint** (`QGCSwitch`) - Return to launch after each waypoint
- **Loiter After RTL** (`QGCSwitch`) - Loiter at home after RTL
- **Land at Target Return** (`QGCSwitch`) - Land at target then return home

### **Business Flow Parameters**
- **Target Hold Time** (`QGCTextField`) - Time to hold at each target in seconds
- **Home Turnaround Wait** (`QGCTextField`) - Wait time at home between trips in seconds
- **Payload Release Enabled** (`QGCSwitch`) - Enable payload release functionality
- **Takeoff Height** (`QGCTextField`) - Takeoff altitude in meters
- **Per Target Separation** (`QGCTextField`) - Time separation between targets in seconds

## **Drawing and Interaction Controls**

### **Drawing Mode**
- **Drawing Mode Toggle** (`QGCButton`) - Enter/exit interactive drawing mode
- **Map Click Interaction** - Click to set area center point
- **Drag Interaction** - Drag to resize area boundaries

## **Mission Generation Controls**

### **Waypoint Generation**
- **Generate Mission** (`QGCButton`) - Generate waypoints and add to mission tab
- **Save Mission File** (`QGCButton`) - Save mission to WPL file
- **Clear Mission Items** (`QGCButton`) - Clear current mission
- **Clear All Missions** (`QGCButton`) - Clear missions from all vehicles

### **Per-Drone Mission Controls**
- **Drone Index Input** (`QGCTextField`) - Select specific drone for operations
- **Insert Single Drone** (`QGCButton`) - Insert waypoints for selected drone
- **Insert All Drones** (`QGCButton`) - Insert waypoints for all drones
- **Save Per-Drone Files** (`QGCButton`) - Save separate WPL files for each drone

## **Vehicle Management Controls**

### **Vehicle Selection**
- **Vehicle Combo Box** (`QGCComboBox`) - Select target vehicle for operations
- **Lock Mapping Switch** (`QGCSwitch`) - Lock vehicle-to-drone mapping
- **Vehicle Mapping Grid** - Map each drone to a specific vehicle

### **Mission Upload Controls**
- **Upload Drone Index** (`QGCTextField`) - Select drone for upload
- **Write Mission to Vehicle** (`QGCButton`) - Upload mission to selected vehicle
- **Write Missions to All Vehicles** (`QGCButton`) - Upload to all mapped vehicles
- **Sync Missions (All Mapped)** (`QGCButton`) - Synchronize all vehicle missions

### **Mission Execution Controls**
- **Stagger Time Input** (`QGCTextField`) - Time delay between vehicle starts
- **Start All Mapped (Staggered)** (`QGCButton`) - Start missions with time stagger

## **Individual Vehicle Controls**

### **Per-Vehicle Operations**
- **Arm Vehicle** (`QGCButton`) - Arm/disarm specific vehicle
- **Takeoff Vehicle** (`QGCButton`) - Command vehicle takeoff
- **Start Mission** (`QGCButton`) - Start mission on vehicle
- **Pause Mission** (`QGCButton`) - Pause vehicle mission
- **Continue Mission** (`QGCButton`) - Resume vehicle mission
- **Land Vehicle** (`QGCButton`) - Command vehicle to land
- **RTL Vehicle** (`QGCButton`) - Return to launch

## **Formation and Swarm Controls**

### **Formation Management**
- **Formation Type Selection** - V Formation, Line Formation, Circle Formation, Grid Formation
- **Formation Spacing** (`QGCTextField`) - Distance between vehicles in formation
- **Start Coordinated Takeoff** (`QGCButton`) - Coordinated takeoff sequence
- **Start Coordinated Mission** (`QGCButton`) - Start synchronized mission
- **Abort Coordinated Mission** (`QGCButton`) - Emergency abort all missions

## **System Integration Controls**

### **Testing and Validation**
- **Verify System Integration** (`QGCButton`) - Test C++ backend connectivity
- **Validate Mission Generation** (`QGCButton`) - Test waypoint generation
- **Refresh Map Display** (`QGCButton`) - Force map visual refresh
- **Validate Area Centering** (`QGCButton`) - Test area positioning
- **Reset to Default Parameters** (`QGCButton`) - Reset all parameters to defaults

### **Performance and Optimization**
- **Enable Optimizations** (`QGCButton`) - Enable performance optimizations
- **Clear Cache** (`QGCButton`) - Clear waypoint generation cache
- **Profile Performance** (`QGCButton`) - Run performance analysis

## **Status and Feedback Controls**

### **Progress Indicators**
- **Progress Bar** - Visual progress indicator for operations
- **Status Messages** - Text feedback for current operations
- **Validation Error Display** - Show input validation errors
- **Processing Indicator** - Show when system is processing

### **Information Displays**
- **Total Waypoint Count** - Display calculated waypoint count
- **Flight Time Estimate** - Show estimated mission duration
- **Drone Allocation Stats** - Show per-drone mission statistics
- **Vehicle Status Grid** - Real-time vehicle status information

## **Advanced Configuration Controls**

### **Cache Management**
- **Cache Size Control** - Adjust waypoint generation cache size
- **Optimization Toggle** - Enable/disable performance optimizations

### **Error Handling**
- **Validation Error Clearing** - Clear validation error messages
- **Error Logging** - Log system errors and warnings
- **Recovery Suggestions** - Display error recovery options

## **Control Categories Summary**

| Category | Control Count | Primary Functions |
|----------|---------------|-------------------|
| **Area Configuration** | 14 | Define area dimensions, positioning, rotation |
| **Multi-Drone Setup** | 8 | Configure swarm parameters and policies |
| **Mission Generation** | 8 | Generate and manage waypoint missions |
| **Vehicle Management** | 12 | Vehicle selection, mapping, and coordination |
| **Individual Vehicle Ops** | 7 | Per-vehicle control and mission execution |
| **Formation Control** | 5 | Swarm formation and coordination |
| **System Integration** | 8 | Testing, validation, and optimization |
| **Status & Feedback** | 8 | Progress indicators and information display |
| **Advanced Config** | 6 | Cache management and error handling |

## **Default Values**

### **Area Parameters**
- Area Width: 10.0 meters
- Area Height: 10.0 meters
- Line Spacing: 1.0 meters
- Number of Points: 1
- Mission Altitude: 5.0 meters
- Area Rotation: 0.0 degrees

### **Multi-Drone Parameters**
- Drone Count: 3
- Altitude Band Start: 2.0 meters
- Altitude Band Step: 2.0 meters
- Time Offset Per Drone: 2.0 seconds
- Per Target Separation: 60.0 seconds

### **Business Flow Parameters**
- Target Hold Time: 10.0 seconds
- Home Turnaround Wait: 30.0 seconds
- Takeoff Height: 5.0 meters
- Payload Release: Disabled
- RTL After Every Waypoint: Disabled
- Loiter After RTL: Disabled

## **Control Types Used**

- **QGCTextField** - Text input for numeric values and parameters
- **QGCButton** - Action buttons for operations and commands
- **QGCComboBox** - Dropdown selection for vehicle and option selection
- **QGCSwitch** - Toggle switches for boolean options
- **QGCLabel** - Text display for information and status
- **Progress Indicators** - Visual feedback for long-running operations
- **Grid Layouts** - Organized display of related controls

## **Integration Points**

### **QGroundControl Core Integration**
- **MissionController** - Mission item management and waypoint generation
- **VehicleManager** - Multi-vehicle coordination and communication
- **PlanView** - Integration with main planning interface
- **MapVisuals** - Real-time map display and interaction

### **MAVLink Protocol**
- **Mission Upload/Download** - Vehicle communication for mission transfer
- **Vehicle Commands** - Arm, takeoff, land, RTL commands
- **Status Monitoring** - Real-time vehicle status and telemetry

This comprehensive control set provides complete functionality for multi-drone area planning, mission generation, vehicle coordination, and mission execution within the QGroundControl environment.

---

*Generated from QGroundControl AreaPlanEditor codebase analysis*
*Last Updated: 2024*