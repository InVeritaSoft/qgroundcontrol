/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>
#include <QList>
#include <QCache>
#include <QElapsedTimer>

// Forward declarations
class Vehicle;
class MissionManager;
class MissionItem;
class MissionController;

// Custom Mission Generator forward declarations
class MissionService;
class PtahMissionGenerator;
class MissionUploadService;
class VehicleService;
class CollisionDetectionService;

class AreaPlanEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit AreaPlanEditor(QObject* parent = nullptr);
    ~AreaPlanEditor() = default;

    // Properties
    Q_PROPERTY(qreal areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
    Q_PROPERTY(qreal areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int numPoints READ numPoints WRITE setNumPoints NOTIFY numPointsChanged)
    Q_PROPERTY(qreal missionAltitude READ missionAltitude WRITE setMissionAltitude NOTIFY missionAltitudeChanged)
    Q_PROPERTY(QGeoCoordinate areaCenter READ areaCenter WRITE setAreaCenter NOTIFY areaCenterChanged)
    Q_PROPERTY(QGeoCoordinate homeLocation READ homeLocation WRITE setHomeLocation NOTIFY homeLocationChanged)
    Q_PROPERTY(qreal areaRotation READ areaRotation WRITE setAreaRotation NOTIFY areaRotationChanged)
    Q_PROPERTY(qreal loiterTime READ loiterTime WRITE setLoiterTime NOTIFY loiterTimeChanged)
    // Multi-drone planning properties
    Q_PROPERTY(int   droneCount           READ droneCount           WRITE setDroneCount           NOTIFY droneCountChanged)
    Q_PROPERTY(qreal altitudeBandStart    READ altitudeBandStart    WRITE setAltitudeBandStart    NOTIFY altitudeBandStartChanged)
    Q_PROPERTY(qreal altitudeBandStep     READ altitudeBandStep     WRITE setAltitudeBandStep     NOTIFY altitudeBandStepChanged)
    Q_PROPERTY(qreal timeOffsetPerDrone   READ timeOffsetPerDrone   WRITE setTimeOffsetPerDrone   NOTIFY timeOffsetPerDroneChanged)
    Q_PROPERTY(qreal perTargetSeparationS READ perTargetSeparationS WRITE setPerTargetSeparationS NOTIFY perTargetSeparationSChanged)
    Q_PROPERTY(bool  rtlAfterEveryWaypoint READ rtlAfterEveryWaypoint WRITE setRtlAfterEveryWaypoint NOTIFY rtlAfterEveryWaypointChanged)
    Q_PROPERTY(bool  loiterAfterRtl       READ loiterAfterRtl       WRITE setLoiterAfterRtl       NOTIFY loiterAfterRtlChanged)
    // Waypoint preview and pre-distribution
    Q_PROPERTY(QVariantList waypointPreview READ waypointPreview WRITE setWaypointPreview NOTIFY waypointPreviewChanged)
    Q_PROPERTY(bool hasPreDistribution READ hasPreDistribution NOTIFY hasPreDistributionChanged)
    // Business-flow timings
    Q_PROPERTY(qreal targetHoldTimeS READ targetHoldTimeS WRITE setTargetHoldTimeS NOTIFY targetHoldTimeSChanged)
    Q_PROPERTY(qreal homeTurnaroundWaitS READ homeTurnaroundWaitS WRITE setHomeTurnaroundWaitS NOTIFY homeTurnaroundWaitSChanged)
    Q_PROPERTY(bool  payloadReleaseEnabled READ payloadReleaseEnabled WRITE setPayloadReleaseEnabled NOTIFY payloadReleaseEnabledChanged)
    Q_PROPERTY(qreal takeoffHeight READ takeoffHeight WRITE setTakeoffHeight NOTIFY takeoffHeightChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY validationErrorChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int progressValue READ progressValue NOTIFY progressValueChanged)
    Q_PROPERTY(QString progressMessage READ progressMessage NOTIFY progressMessageChanged)
    Q_PROPERTY(QString currentOperation READ currentOperation NOTIFY currentOperationChanged)
    Q_PROPERTY(bool isOptimized READ isOptimized NOTIFY isOptimizedChanged)
    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(bool isDrawingMode READ isDrawingMode WRITE setIsDrawingMode NOTIFY isDrawingModeChanged)
    Q_PROPERTY(QObject* planMasterController READ planMasterController WRITE setPlanMasterController NOTIFY planMasterControllerChanged)
    // Policy: land at target then return home (base)
    Q_PROPERTY(bool landAtTargetReturn READ landAtTargetReturn WRITE setLandAtTargetReturn NOTIFY landAtTargetReturnChanged)
    
    // Swarm configuration properties
    // Swarm configuration properties are already declared above

    // Property getters
    qreal areaWidth() const { return _areaWidth; }
    qreal areaHeight() const { return _areaHeight; }
    qreal lineSpacing() const { return _lineSpacing; }
    int numPoints() const { return _numPoints; }
    qreal missionAltitude() const { return _missionAltitude; }
    QGeoCoordinate areaCenter() const { return _areaCenter; }
    QGeoCoordinate homeLocation() const { return _homeLocation; }
    qreal areaRotation() const { return _areaRotation; }
    qreal loiterTime() const { return _loiterTime; }
    QString validationError() const { return _validationError; }
    bool isProcessing() const { return _isProcessing; }
    int progressValue() const { return _progressValue; }
    QString progressMessage() const { return _progressMessage; }
    QString currentOperation() const { return _currentOperation; }
    bool isOptimized() const { return _isOptimized; }
    int cacheSize() const { return _cacheSize; }
    bool isDrawingMode() const { return _isDrawingMode; }
    QObject* planMasterController() const { return _planMasterController; }
    bool landAtTargetReturn() const { return _landAtTargetReturn; }

    // Multi-drone getters
    int   droneCount() const { return _droneCount; }
    qreal altitudeBandStart() const { return _altitudeBandStart; }
    qreal altitudeBandStep() const { return _altitudeBandStep; }
    qreal timeOffsetPerDrone() const { return _timeOffsetPerDrone; }
    qreal perTargetSeparationS() const { return _perTargetSeparationS; }
    bool  rtlAfterEveryWaypoint() const { return _rtlAfterEveryWaypoint; }
    bool  loiterAfterRtl() const { return _loiterAfterRtl; }
    qreal targetHoldTimeS() const { return _targetHoldTimeS; }
    qreal homeTurnaroundWaitS() const { return _homeTurnaroundWaitS; }
    bool  payloadReleaseEnabled() const { return _payloadReleaseEnabled; }
    qreal takeoffHeight() const { return _takeoffHeight; }

    // Property setters
    Q_INVOKABLE void setAreaWidth(qreal width);
    Q_INVOKABLE void setAreaHeight(qreal height);
    Q_INVOKABLE void setLineSpacing(qreal spacing);
    Q_INVOKABLE void setNumPoints(int points);
    Q_INVOKABLE void setMissionAltitude(qreal altitude);
    Q_INVOKABLE void setAreaCenter(const QGeoCoordinate& center);
    Q_INVOKABLE void setHomeLocation(const QGeoCoordinate& location);
    Q_INVOKABLE void setAreaRotation(qreal rotation);
    Q_INVOKABLE void setLoiterTime(qreal time);
    Q_INVOKABLE void setIsDrawingMode(bool drawingMode);
    Q_INVOKABLE void setPlanMasterController(QObject* controller);
    Q_INVOKABLE void setLandAtTargetReturn(bool enabled);

    // Multi-drone setters
    Q_INVOKABLE void setDroneCount(int count);
    Q_INVOKABLE void setAltitudeBandStart(qreal startMeters);
    Q_INVOKABLE void setAltitudeBandStep(qreal stepMeters);
    Q_INVOKABLE void setTimeOffsetPerDrone(qreal seconds);
    Q_INVOKABLE void setPerTargetSeparationS(qreal seconds);
    Q_INVOKABLE void setRtlAfterEveryWaypoint(bool enabled);
    Q_INVOKABLE void setLoiterAfterRtl(bool enabled);
    // Business-flow setters
    Q_INVOKABLE void setTargetHoldTimeS(qreal seconds);
    Q_INVOKABLE void setHomeTurnaroundWaitS(qreal seconds);
    Q_INVOKABLE void setPayloadReleaseEnabled(bool enabled);
    Q_INVOKABLE void setTakeoffHeight(qreal height);

    // Mission planning functions
    Q_INVOKABLE void moveAreaNorth();
    Q_INVOKABLE void moveAreaSouth();
    Q_INVOKABLE void moveAreaEast();
    Q_INVOKABLE void moveAreaWest();
    Q_INVOKABLE void rotateAreaClockwise();
    Q_INVOKABLE void rotateAreaCounterClockwise();
    Q_INVOKABLE void centerArea();
    Q_INVOKABLE void resetArea();  // Add reset functionality
    Q_INVOKABLE int calculateTotalWaypoints() const;
    Q_INVOKABLE int calculateFlightTime() const;
    Q_INVOKABLE QVariantList generateWaypoints();
    Q_INVOKABLE QVariantList computePartitionStripes() const;
    Q_INVOKABLE QVariantList computeRoundRobinAssignments() const;
    Q_INVOKABLE QVariantList computeDroneAssignments() const;
    Q_INVOKABLE QVariantMap computePerDroneCounts() const;
    // Preview per-drone waypoints without mutating MissionController
    // Returns a QVariantList of maps: { droneIndex, altitudeOffsetM, timeOffsetS, waypoints: [QGeoCoordinate, ...] }
    Q_INVOKABLE QVariantList computePerDroneWaypointPreview() const;
    // Generate only the waypoint coordinates for a specific drone index
    Q_INVOKABLE QVariantList generatePerDroneWaypoints(int droneIndex) const;
    // Generate all waypoints without drone distribution for New Mission Generator
    Q_INVOKABLE QVariantList generateAllWaypointsForNewMissionGenerator() const;
    // Insert a specific drone's waypoints into the MissionController (non-multi aggregation)
    Q_INVOKABLE void addPerDroneToMission(int droneIndex);
    Q_INVOKABLE void addAllDronesToMission();
    Q_INVOKABLE void addWaypointsToMission();
    Q_INVOKABLE void saveMissionFile();
    // Save separate WPL files per-drone using current multi-drone configuration
    Q_INVOKABLE void savePerDroneMissionFiles();
    Q_INVOKABLE void clearMission();
    // Clear missions from all vehicles and current plan
    Q_INVOKABLE void clearAllMissions();
    Q_INVOKABLE void uploadToVehicle();
    // Upload a specific drone's mission to a selected vehicle (or active vehicle if null)
    Q_INVOKABLE void uploadPerDroneMissionToVehicle(int droneIndex, QObject* vehicleObject = nullptr);
    // Upload missions to all connected drones in a coordinated way
    Q_INVOKABLE void uploadToAllDrones();
    // Get list of available vehicles for mission upload
    Q_INVOKABLE QVariantList getAvailableVehicles() const;
    Q_INVOKABLE void startMission();
    Q_INVOKABLE void updateStatus(const QString& message);
    Q_INVOKABLE QGeoCoordinate calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const;
    
    // Custom Mission Generator methods
    Q_INVOKABLE void useNewMissionGenerator(int vehicleId = -1); // -1 = NON ID 1 vehicles, 1 = Vehicle ID 1
    Q_INVOKABLE void initializeMissionGeneratorServices();
    Q_INVOKABLE QObject* getMissionService() const;
    Q_INVOKABLE QList<QGeoCoordinate> getDrawnAreaCoordinates() const;
    Q_INVOKABLE QList<QGeoCoordinate> getDrawnAreaCoordinatesWithPreDistribution() const;
    Q_INVOKABLE bool hasPreDistribution() const;
    
    // Waypoint preview getter/setter
    QVariantList waypointPreview() const;
    void setWaypointPreview(const QVariantList& waypointPreview);
    void updatePreDistributionStatus();
    
    // Per-vehicle mission and control helpers
    Q_INVOKABLE void armVehicle(QObject* vehicleObject, bool arm);
    Q_INVOKABLE void takeoffVehicle(QObject* vehicleObject, qreal altitude);
    Q_INVOKABLE void landVehicle(QObject* vehicleObject);
    Q_INVOKABLE void startMissionOnVehicle(QObject* vehicleObject);
    Q_INVOKABLE void pauseMissionOnVehicle(QObject* vehicleObject);
    Q_INVOKABLE void continueMissionOnVehicle(QObject* vehicleObject);
    Q_INVOKABLE void rtlVehicle(QObject* vehicleObject);
    Q_INVOKABLE QVariantMap getVehicleStatus(QObject* vehicleObject) const;
    // Insert helper commands
    void insertGripperRelease(MissionController* mission, const QGeoCoordinate& atCoord);
    
    // Mission upload helper methods
    Q_INVOKABLE Vehicle* getCurrentVehicle() const;
    Q_INVOKABLE MissionManager* getMissionManager() const;
    Q_INVOKABLE MissionController* getMissionController() const;
    
    // Formation types
    enum FormationType {
        NoFormation,
        VFormation,
        LineFormation,
        CircleFormation,
        GridFormation
    };
    Q_ENUM(FormationType)

    // Swarm coordination methods
    Q_INVOKABLE bool startCoordinatedTakeoff();
    Q_INVOKABLE bool startCoordinatedMission();
    Q_INVOKABLE bool abortCoordinatedMission();
    Q_INVOKABLE bool setFormationType(FormationType type);
    Q_INVOKABLE bool adjustFormationSpacing(qreal spacing);
    Q_INVOKABLE bool assignFormationRoles();
    Q_INVOKABLE bool startFormationTransition();
    
    // Swarm status properties
    Q_PROPERTY(bool isSwarmReady READ isSwarmReady NOTIFY swarmStatusChanged)
    Q_PROPERTY(QString swarmStatus READ swarmStatus NOTIFY swarmStatusChanged)
    Q_PROPERTY(bool isCoordinatedMissionActive READ isCoordinatedMissionActive NOTIFY coordinatedMissionStatusChanged)
    Q_PROPERTY(FormationType currentFormation READ currentFormation NOTIFY formationChanged)
    Q_PROPERTY(qreal formationSpacing READ formationSpacing WRITE setFormationSpacing NOTIFY formationSpacingChanged)
    Q_PROPERTY(bool isFormationTransitioning READ isFormationTransitioning NOTIFY formationTransitioningChanged)
    
    // Swarm status getters
    bool isSwarmReady() const;
    QString swarmStatus() const;
    bool isCoordinatedMissionActive() const;
    FormationType currentFormation() const;
    qreal formationSpacing() const { return _formationSpacing; }
    void setFormationSpacing(qreal spacing);
    bool isFormationTransitioning() const;
    
    // Mission workflow testing methods (public for testing)
    Q_INVOKABLE void testCompleteWorkflow();
    Q_INVOKABLE bool validateAreaParameters();
    Q_INVOKABLE bool validateWaypointGeneration();
    Q_INVOKABLE bool validateMissionUpload();
    Q_INVOKABLE bool validateMissionFileSaving();
    
private:
    // Swarm coordination helpers
    bool checkSwarmReadiness() const;
    void updateSwarmStatus(const QString& status);
    void sendSwarmCommand(uint16_t command, const QList<Vehicle*>& vehicles);
    void handleSwarmResponse(Vehicle* vehicle, uint16_t command, uint8_t result);
    
    // Formation control methods
    void calculateFormationPositions();
    void updateFormationOffsets();
    void sendFormationCommands();
    void handleFormationResponse(Vehicle* vehicle, uint16_t command, uint8_t result);
    QGeoCoordinate calculateFormationPosition(int vehicleIndex, FormationType type) const;
    
    // Swarm state
    QString _swarmStatus;
    bool _isCoordinatedMissionActive = false;
    QMap<int, bool> _vehicleReadyStatus;  // Maps vehicle ID to ready status
    
    // Formation state
    FormationType _currentFormation = NoFormation;
    qreal _formationSpacing = 5.0;  // meters
    bool _isFormationTransitioning = false;
    Vehicle* _leaderVehicle = nullptr;
    QMap<int, QGeoCoordinate> _formationOffsets;  // Maps vehicle ID to formation position offset
    QMap<int, int> _formationRoles;  // Maps vehicle ID to formation role index
    
    // Mission file saving helper method
    void saveMissionToFile(const QList<MissionItem*>& missionItems, const QString& filename);
    void saveMissionToFile(MissionController* missionController, const QString& filename);
    
    // Mission workflow testing methods moved to public section

    // Swarm configuration methods
    Q_INVOKABLE QVariantMap getDroneAllocationStats(int droneIndex) const;
    Q_INVOKABLE bool validateSwarmConfiguration() const;
    // These methods are already declared above
    
    // Error handling and validation methods
    Q_INVOKABLE QString validateInput(const QString& fieldName, const QVariant& value) const;
    Q_INVOKABLE bool isInputValid(const QString& fieldName, const QVariant& value) const;
    Q_INVOKABLE QString getValidationError() const;
    Q_INVOKABLE void clearValidationError();
    Q_INVOKABLE void logError(const QString& errorMessage, const QString& context = QString());
    Q_INVOKABLE void handleError(const QString& errorMessage, const QString& recoverySuggestion = QString());
    
    // Progress indicator methods
    Q_INVOKABLE void startProgress(const QString& operation, const QString& message = QString());
    Q_INVOKABLE void updateProgress(int value, const QString& message = QString());
    Q_INVOKABLE void finishProgress(const QString& message = QString());
    Q_INVOKABLE void cancelProgress();
    Q_INVOKABLE void setProgressOperation(const QString& operation);
    
    // Performance optimization methods
    Q_INVOKABLE void enableOptimizations();
    Q_INVOKABLE void disableOptimizations();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void optimizeWaypointGeneration();
    Q_INVOKABLE void setCacheSize(int size);
    Q_INVOKABLE void profilePerformance();
    Q_INVOKABLE QVariantMap getPerformanceMetrics() const;

signals:
    void areaWidthChanged();
    // Emitted when a per-drone mission upload is initiated/completed for a vehicle
    void missionUploaded(int droneIndex, QObject* vehicle);
    // Emitted when waypoints are generated by the custom mission generator
    void waypointsGenerated(const QVariantList& waypoints);
    void areaHeightChanged();
    void lineSpacingChanged();
    void numPointsChanged();
    void missionAltitudeChanged();
    void areaCenterChanged();
    void homeLocationChanged();
    void areaRotationChanged();
    void loiterTimeChanged();
    void statusChanged(const QString& message);
    void validationErrorChanged();
    void isProcessingChanged();
    void progressValueChanged();
    void progressMessageChanged();
    void waypointPreviewChanged();
    void hasPreDistributionChanged();
    void currentOperationChanged();
    void isOptimizedChanged();
    void cacheSizeChanged();
    void isDrawingModeChanged();
    void planMasterControllerChanged();
    void landAtTargetReturnChanged();
    
    // Swarm coordination signals
    void swarmStatusChanged();
    void coordinatedMissionStatusChanged();
    
    // Formation signals
    void formationChanged();
    void formationSpacingChanged();
    void formationTransitioningChanged();
    void formationRolesChanged();
    void leaderVehicleChanged();
    void formationPositionsChanged();
    
    // Swarm configuration signals
    void isValidChanged();
    void droneCountChanged();
    void altitudeBandStartChanged();
    void altitudeBandStepChanged();
    void timeOffsetPerDroneChanged();
    void perTargetSeparationSChanged();
    // Multi-drone signals
    void rtlAfterEveryWaypointChanged();
    void loiterAfterRtlChanged();
    // Business-flow signals
    void targetHoldTimeSChanged();
    void homeTurnaroundWaitSChanged();
    void payloadReleaseEnabledChanged();
    void takeoffHeightChanged();

private slots:
    // Mission generation signal handlers
    void onMissionGenerationStarted();
    void onMissionGenerationCompleted(bool success, const QString& message);
    void onWaypointsGenerated(const QList<QGeoCoordinate>& waypoints);

private:

    // Default values
    static constexpr qreal _defaultAreaWidth = 10.0;   // Changed from 100.0 to 10.0
    static constexpr qreal _defaultAreaHeight = 10.0;  // Changed from 100.0 to 10.0
    static constexpr qreal _defaultLineSpacing = 1.0; // per defaults UI
    static constexpr int _defaultNumPoints = 1;
    static constexpr qreal _defaultAltitude = 5.0;
    static constexpr int   _defaultDroneCount = 3;
    static constexpr qreal _defaultAltitudeBandStart = 2.0;
    static constexpr qreal _defaultAltitudeBandStep  = 2.0;
    static constexpr qreal _defaultTimeOffsetPerDrone = 2.0; // seconds
    static constexpr qreal _defaultPerTargetSeparationS = 60.0; // seconds

    // Properties
    qreal _areaWidth = _defaultAreaWidth;
    qreal _areaHeight = _defaultAreaHeight;
    qreal _lineSpacing = _defaultLineSpacing;
    int _numPoints = _defaultNumPoints;
    qreal _missionAltitude = _defaultAltitude;
    QGeoCoordinate _areaCenter = QGeoCoordinate(49.82824897481479, 24.033390804256005);
    QGeoCoordinate _homeLocation = QGeoCoordinate();
    
    // Waypoint preview and pre-distribution
    QVariantList _waypointPreview;
    bool _hasPreDistribution = false;

    // Cached allocation statistics
    struct DroneStats {
        int waypointCount = 0;
        qreal areaSize = 0.0;
        qreal lineLength = 0.0;
        qreal totalDistance = 0.0;
        qreal estimatedTime = 0.0;
        qreal altitudeOffset = 0.0;
        qreal timeOffset = 0.0;
        int lineCount = 0;
        QList<QGeoCoordinate> waypoints;
        QList<int> lineIndices;
    };
    QMap<int, DroneStats> _droneStats;
    qreal _areaRotation = 0.0;  // Rotation in degrees, 0 = North
    qreal _loiterTime = 2.0;    // Loiter time in seconds at each waypoint
    bool _isProcessing = false;
    int _progressValue = 0;
    QString _progressMessage;
    QString _currentOperation;
    bool _isOptimized = false;
    int _cacheSize = 0;
    bool _isDrawingMode = false;
    QObject* _planMasterController = nullptr;
    bool _landAtTargetReturn = false;
    // Multi-drone fields
    int   _droneCount = _defaultDroneCount;
    qreal _altitudeBandStart = _defaultAltitudeBandStart;
    qreal _altitudeBandStep = _defaultAltitudeBandStep;
    qreal _timeOffsetPerDrone = _defaultTimeOffsetPerDrone;
    qreal _perTargetSeparationS = _defaultPerTargetSeparationS;
    bool  _rtlAfterEveryWaypoint = false;
    bool  _loiterAfterRtl = false;
    QString _validationError;
    
    // Performance optimization members
    QHash<QString, QVariantList> _waypointCache;
    QElapsedTimer _performanceTimer;
    QVariantMap _performanceMetrics;
    int _cacheHits = 0;
    int _cacheMisses = 0;
    
    // Business-flow fields
    qreal _targetHoldTimeS = 10.0;         // default 10s hold at target
    qreal _homeTurnaroundWaitS = 30.0;     // default 30s wait at home between trips
    bool  _payloadReleaseEnabled = false;  // default disabled
    qreal _takeoffHeight = 5.0;            // default takeoff height in meters
    
    // Custom Mission Generator services
    class MissionService* _missionService = nullptr;
    class PtahMissionGenerator* _ptahMissionGenerator = nullptr;
    class MissionUploadService* _missionUploadService = nullptr;
    class VehicleService* _vehicleService = nullptr;
    class CollisionDetectionService* _collisionDetectionService = nullptr;
};
