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
    Q_PROPERTY(bool  rtlAfterEveryWaypoint READ rtlAfterEveryWaypoint WRITE setRtlAfterEveryWaypoint NOTIFY rtlAfterEveryWaypointChanged)
    Q_PROPERTY(bool  loiterAfterRtl       READ loiterAfterRtl       WRITE setLoiterAfterRtl       NOTIFY loiterAfterRtlChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY validationErrorChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int progressValue READ progressValue NOTIFY progressValueChanged)
    Q_PROPERTY(QString progressMessage READ progressMessage NOTIFY progressMessageChanged)
    Q_PROPERTY(QString currentOperation READ currentOperation NOTIFY currentOperationChanged)
    Q_PROPERTY(bool isOptimized READ isOptimized NOTIFY isOptimizedChanged)
    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(bool isDrawingMode READ isDrawingMode WRITE setIsDrawingMode NOTIFY isDrawingModeChanged)
    Q_PROPERTY(QObject* planMasterController READ planMasterController WRITE setPlanMasterController NOTIFY planMasterControllerChanged)

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

    // Multi-drone getters
    int   droneCount() const { return _droneCount; }
    qreal altitudeBandStart() const { return _altitudeBandStart; }
    qreal altitudeBandStep() const { return _altitudeBandStep; }
    qreal timeOffsetPerDrone() const { return _timeOffsetPerDrone; }
    bool  rtlAfterEveryWaypoint() const { return _rtlAfterEveryWaypoint; }
    bool  loiterAfterRtl() const { return _loiterAfterRtl; }

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

    // Multi-drone setters
    Q_INVOKABLE void setDroneCount(int count);
    Q_INVOKABLE void setAltitudeBandStart(qreal startMeters);
    Q_INVOKABLE void setAltitudeBandStep(qreal stepMeters);
    Q_INVOKABLE void setTimeOffsetPerDrone(qreal seconds);
    Q_INVOKABLE void setRtlAfterEveryWaypoint(bool enabled);
    Q_INVOKABLE void setLoiterAfterRtl(bool enabled);

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
    // Insert a specific drone's waypoints into the MissionController (non-multi aggregation)
    Q_INVOKABLE void addPerDroneToMission(int droneIndex);
    Q_INVOKABLE void addAllDronesToMission();
    Q_INVOKABLE void addWaypointsToMission();
    Q_INVOKABLE void saveMissionFile();
    Q_INVOKABLE void uploadToVehicle();
    Q_INVOKABLE void startMission();
    Q_INVOKABLE void updateStatus(const QString& message);
    Q_INVOKABLE QGeoCoordinate calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const;
    
    // Mission upload helper methods
    Q_INVOKABLE Vehicle* getCurrentVehicle() const;
    Q_INVOKABLE MissionManager* getMissionManager() const;
    Q_INVOKABLE MissionController* getMissionController() const;
    
    // Mission file saving helper method
    void saveMissionToFile(const QList<MissionItem*>& missionItems, const QString& filename);
    void saveMissionToFile(MissionController* missionController, const QString& filename);
    
    // Mission workflow testing methods
    Q_INVOKABLE void testCompleteWorkflow();
    Q_INVOKABLE bool validateAreaParameters() const;
    Q_INVOKABLE bool validateWaypointGeneration();
    Q_INVOKABLE bool validateMissionUpload();
    Q_INVOKABLE bool validateMissionFileSaving();
    
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
    void currentOperationChanged();
    void isOptimizedChanged();
    void cacheSizeChanged();
    void isDrawingModeChanged();
    void planMasterControllerChanged();
    // Multi-drone signals
    void droneCountChanged();
    void altitudeBandStartChanged();
    void altitudeBandStepChanged();
    void timeOffsetPerDroneChanged();
    void rtlAfterEveryWaypointChanged();
    void loiterAfterRtlChanged();

private:

    // Default values
    static constexpr qreal _defaultAreaWidth = 10.0;   // Changed from 100.0 to 10.0
    static constexpr qreal _defaultAreaHeight = 10.0;  // Changed from 100.0 to 10.0
    static constexpr qreal _defaultLineSpacing = 10.0; // Increased from 3.0
    static constexpr int _defaultNumPoints = 1;
    static constexpr qreal _defaultAltitude = 10.0;
    static constexpr int   _defaultDroneCount = 2;
    static constexpr qreal _defaultAltitudeBandStart = 0.0;
    static constexpr qreal _defaultAltitudeBandStep  = 10.0;
    static constexpr qreal _defaultTimeOffsetPerDrone = 0.0; // seconds

    // Properties
    qreal _areaWidth = _defaultAreaWidth;
    qreal _areaHeight = _defaultAreaHeight;
    qreal _lineSpacing = _defaultLineSpacing;
    int _numPoints = _defaultNumPoints;
    qreal _missionAltitude = _defaultAltitude;
    QGeoCoordinate _areaCenter = QGeoCoordinate(49.82824897481479, 24.033390804256005);
    QGeoCoordinate _homeLocation = QGeoCoordinate(49.82824897481479, 24.033390804256005);
    qreal _areaRotation = 0.0;  // Rotation in degrees, 0 = North
    qreal _loiterTime = 10.0;   // Loiter time in seconds at each waypoint
    QString _validationError;
    bool _isProcessing = false;
    int _progressValue = 0;
    QString _progressMessage;
    QString _currentOperation;
    bool _isOptimized = false;
    int _cacheSize = 0;
    bool _isDrawingMode = false;
    QObject* _planMasterController = nullptr;
    // Multi-drone fields
    int   _droneCount = _defaultDroneCount;
    qreal _altitudeBandStart = _defaultAltitudeBandStart;
    qreal _altitudeBandStep  = _defaultAltitudeBandStep;
    qreal _timeOffsetPerDrone = _defaultTimeOffsetPerDrone;
    bool  _rtlAfterEveryWaypoint = false;
    bool  _loiterAfterRtl = false;
    
    // Performance optimization members
    QHash<QString, QVariantList> _waypointCache;
    QElapsedTimer _performanceTimer;
    QVariantMap _performanceMetrics;
    int _cacheHits = 0;
    int _cacheMisses = 0;
}; 