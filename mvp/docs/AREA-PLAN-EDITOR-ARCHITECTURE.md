# Area Plan Editor - Comprehensive Architecture & Code Flow

## System Overview

This document provides a comprehensive visualization of the QGroundControl Area Plan Editor architecture using Mermaid diagrams. The system enables multi-drone area planning with interactive drawing, mission generation, and vehicle coordination.

## 1. High-Level System Architecture

```mermaid
graph TB
    subgraph "QML Frontend Layer"
        QML[AreaPlanEditor.qml]
        MapViz[AreaPlanMapVisuals.qml]
        PlanView[PlanView.qml]
    end
    
    subgraph "C++ Backend Layer"
        APE[AreaPlanEditor.cc/h]
        AP[AreaPartition.h]
        DA[DroneAssignment.h]
    end
    
    subgraph "QGroundControl Core"
        MC[MissionController]
        MM[MissionManager]
        VM[Vehicle Manager]
        QGC[QGCApplication]
    end
    
    subgraph "External Systems"
        MAV[MAVLink Protocol]
        Vehicle[UAV Vehicles]
    end
    
    QML --> APE
    MapViz --> APE
    APE --> AP
    APE --> DA
    APE --> MC
    MC --> MM
    APE --> VM
    VM --> Vehicle
    Vehicle --> MAV
```

## 2. Class Structure & Relationships

```mermaid
classDiagram
    class AreaPlanEditor {
        +QObject parent
        +qreal areaWidth
        +qreal areaHeight
        +qreal lineSpacing
        +int numPoints
        +qreal missionAltitude
        +int droneCount
        +QGeoCoordinate areaCenter
        +qreal areaRotation
        +generateWaypoints()
        +addWaypointsToMission()
        +uploadToVehicle()
        +setAreaWidth(qreal)
        +setAreaHeight(qreal)
        +setLineSpacing(qreal)
        +setNumPoints(int)
        +setMissionAltitude(qreal)
        +setAreaCenter(QGeoCoordinate)
        +setAreaRotation(qreal)
    }
    
    class AreaPartition {
        +struct Point { double x, y }
        +struct Line { Point a, b }
        +splitIntoStripes(cx, cy, width, height, stripeCount, alongShortAxis, rotationDeg)
        +assignStripesRoundRobin(droneCount, lineCount)
    }
    
    class DroneAssignment {
        +int droneIndex
        +double altitudeOffsetM
        +double timeOffsetS
        +vector<int> lineIndices
    }
    
    class MissionController {
        +insertSimpleMissionItem()
        +removeVisualItem()
        +sendToVehicle()
        +visualItems()
    }
    
    class Vehicle {
        +id()
        +coordinate()
        +armed()
        +flightMode()
        +missionManager()
        +setArmed(bool)
        +startMission()
        +guidedModeTakeoff()
    }
    
    AreaPlanEditor --> AreaPartition : uses
    AreaPlanEditor --> DroneAssignment : creates
    AreaPlanEditor --> MissionController : controls
    AreaPlanEditor --> Vehicle : manages
```

## 3. Mission Generation Flow

```mermaid
flowchart TD
    A[User Configures Area] --> B{Validate Parameters}
    B -->|Invalid| C[Show Error]
    B -->|Valid| D[Calculate Line Count]
    
    D --> E[Generate Stripes]
    E --> F[Apply Round-Robin Assignment]
    F --> G[Calculate Per-Drone Waypoints]
    
    G --> H{Multi-Drone Mode?}
    H -->|Yes| I[Generate Altitude Bands]
    H -->|No| J[Single Drone Waypoints]
    
    I --> K[Apply Time Offsets]
    K --> L[Insert Mission Items]
    
    J --> L
    L --> M[Save Mission File]
    M --> N[Upload to Vehicle]
    
    C --> O[Return to Input]
    O --> A
```

## 4. Multi-Drone Coordination Flow

```mermaid
sequenceDiagram
    participant UI as QML UI
    participant APE as AreaPlanEditor
    participant AP as AreaPartition
    participant MC as MissionController
    participant VM as VehicleManager
    participant V as Vehicle
    
    UI->>APE: setDroneCount(N)
    APE->>AP: assignStripesRoundRobin(N, lineCount)
    AP-->>APE: droneAssignments[]
    
    UI->>APE: generatePerDroneWaypoints(droneIndex)
    APE->>AP: splitIntoStripes(...)
    AP-->>APE: stripes[]
    APE->>APE: calculateOffsetCoordinate()
    APE-->>UI: waypoints[]
    
    UI->>APE: addPerDroneToMission(droneIndex)
    APE->>MC: insertSimpleMissionItem()
    MC-->>APE: missionItem
    
    UI->>APE: uploadPerDroneMissionToVehicle(droneIndex, vehicle)
    APE->>V: missionManager.writeMissionItems()
    V-->>APE: uploadComplete
    
    UI->>APE: startMissionOnVehicle(vehicle)
    APE->>V: startMission()
    V-->>APE: missionStarted
```

## 5. Interactive Drawing System Flow

```mermaid
stateDiagram-v2
    [*] --> Ready
    Ready --> DrawingMode : Activate Drawing Mode
    DrawingMode --> SelectingCenter : User Clicks Map
    SelectingCenter --> DrawingArea : Center Selected
    DrawingArea --> AreaDefined : User Completes Drawing
    
    AreaDefined --> Configuring : User Adjusts Parameters
    Configuring --> AreaDefined : Parameters Updated
    AreaDefined --> GeneratingMission : Generate Waypoints
    GeneratingMission --> MissionReady : Mission Created
    
    MissionReady --> Uploading : Upload to Vehicle
    Uploading --> MissionActive : Mission Active
    MissionActive --> MissionComplete : Mission Finished
    
    MissionComplete --> Ready : Return to Ready State
    
    DrawingMode --> Ready : Deactivate Drawing Mode
    AreaDefined --> DrawingMode : Modify Area
    Configuring --> DrawingMode : Redraw Area
```

## 6. Data Flow Architecture

```mermaid
graph LR
    subgraph "Input Layer"
        A[Area Dimensions]
        B[Line Spacing]
        C[Drone Count]
        D[Altitude Bands]
        E[Time Offsets]
    end
    
    subgraph "Processing Layer"
        F[Geometry Calculations]
        G[Stripe Generation]
        H[Round-Robin Assignment]
        I[Coordinate Transformations]
    end
    
    subgraph "Output Layer"
        J[Waypoint Arrays]
        K[Mission Items]
        L[CSV Files]
        M[Vehicle Uploads]
    end
    
    A --> F
    B --> F
    C --> H
    D --> I
    E --> I
    
    F --> G
    G --> H
    H --> I
    I --> J
    
    J --> K
    J --> L
    K --> M
```

## 7. Error Handling & Validation Flow

```mermaid
flowchart TD
    A[User Input] --> B{Input Validation}
    B -->|Invalid| C[Show Error Message]
    B -->|Valid| D[Parameter Processing]
    
    C --> E[Clear Error]
    E --> A
    
    D --> F{Geometry Validation}
    F -->|Invalid| G[Show Geometry Error]
    F -->|Valid| H[Waypoint Generation]
    
    G --> E
    
    H --> I{Generation Success?}
    I -->|No| J[Show Generation Error]
    I -->|Yes| K[Mission Creation]
    
    J --> E
    
    K --> L{Upload Validation}
    L -->|Failed| M[Show Upload Error]
    L -->|Success| N[Mission Active]
    
    M --> E
```

## 8. Performance Optimization Flow

```mermaid
graph TD
    A[User Request] --> B{Cache Hit?}
    B -->|Yes| C[Return Cached Result]
    B -->|No| D[Generate New Result]
    
    D --> E[Store in Cache]
    E --> F[Return Result]
    
    C --> G[Update Cache Stats]
    F --> G
    
    G --> H{Cache Full?}
    H -->|Yes| I[Evict Old Entries]
    H -->|No| J[Continue]
    
    I --> J
    J --> K[Performance Metrics]
    K --> L[Optimization Suggestions]
```

## 9. File Structure & Dependencies

```mermaid
graph TB
    subgraph "Source Files"
        A[src/QmlControls/AreaPlanEditor.qml]
        B[src/QmlControls/AreaPlanEditor.cc]
        C[src/QmlControls/AreaPlanEditor.h]
        D[src/MissionManager/AreaPartition.h]
        E[src/MissionManager/DroneAssignment.h]
    end
    
    subgraph "Dependencies"
        F[Qt Core]
        G[Qt Positioning]
        H[QGroundControl Core]
        I[MAVLink Protocol]
    end
    
    subgraph "Generated Files"
        J[CMake Build System]
        K[QML Registration]
        L[Binary Executable]
    end
    
    A --> B
    B --> C
    C --> D
    C --> E
    
    B --> F
    B --> G
    B --> H
    B --> I
    
    C --> J
    J --> K
    K --> L
```

## 10. Testing & Validation Flow

```mermaid
flowchart TD
    A[Test Start] --> B[Unit Tests]
    B --> C{Geometry Tests}
    C -->|Pass| D[Integration Tests]
    C -->|Fail| E[Debug Geometry]
    
    D --> F{Mission Generation Tests}
    F -->|Pass| G[Vehicle Integration Tests]
    F -->|Fail| H[Debug Mission Logic]
    
    G --> I{Upload Tests}
    I -->|Pass| J[End-to-End Tests]
    I -->|Fail| K[Debug Vehicle Communication]
    
    J --> L{All Tests Pass?}
    L -->|Yes| M[Test Complete]
    L -->|No| N[Debug Issues]
    
    E --> B
    H --> B
    K --> B
    N --> B
```

## Code References

### Key Functions in AreaPlanEditor.cc

- **`generateWaypoints()`** (Line 650): Core waypoint generation algorithm
- **`computePartitionStripes()`** (Line 700): Stripe calculation using AreaPartition
- **`computeRoundRobinAssignments()`** (Line 720): Drone assignment algorithm
- **`addPerDroneToMission()`** (Line 800): Mission insertion logic
- **`uploadPerDroneMissionToVehicle()`** (Line 900): Vehicle upload implementation

### Key Functions in AreaPartition.h

- **`splitIntoStripes()`** (Line 25): Core geometry splitting algorithm
- **`assignStripesRoundRobin()`** (Line 65): Round-robin assignment logic

### Key Properties in AreaPlanEditor.h

- **Area Configuration**: `areaWidth`, `areaHeight`, `lineSpacing`, `numPoints`
- **Multi-Drone**: `droneCount`, `altitudeBandStart`, `altitudeBandStep`, `timeOffsetPerDrone`
- **Mission Control**: `missionAltitude`, `areaCenter`, `areaRotation`

### QML Integration Points

- **Property Binding**: All C++ properties exposed as QML properties
- **Signal Handling**: Status updates, validation errors, progress indicators
- **Method Invocation**: QML calls C++ methods for all operations

## Performance Considerations

1. **Caching**: Waypoint generation results cached to avoid recalculation
2. **Lazy Loading**: Mission items generated only when requested
3. **Batch Operations**: Multiple waypoints inserted in single operations
4. **Memory Management**: Efficient use of Qt containers and smart pointers

## Security & Validation

1. **Input Validation**: All user inputs validated before processing
2. **Bounds Checking**: Geometric calculations include safety bounds
3. **Error Handling**: Comprehensive error reporting and recovery
4. **Vehicle Safety**: Mission validation before upload to vehicles

This architecture provides a robust, scalable foundation for multi-drone area planning with interactive drawing capabilities, comprehensive mission generation, and seamless vehicle integration.
