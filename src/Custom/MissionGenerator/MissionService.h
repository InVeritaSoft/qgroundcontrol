#pragma once

#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>

class PtahMissionGenerator;
class MissionUploadService;
class VehicleService;
class Vehicle;

class MissionService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MissionService(QObject* parent = nullptr);
    ~MissionService();

    Q_INVOKABLE void generateMission(const QString& missionType, 
                                    int areaSize, 
                                    int altitude, 
                                    double speed, 
                                    const QString& description,
                                    double frontDistance = 10.0,
                                    bool payloadDropMode = false,
                                    int loiterTimeSeconds = 50,
                                    int bendHeight = 10,
                                    double payloadDropHeight = 1.5,
                                    int servoDelaySeconds = 3,
                                    double observationDistance = 100.0);

    // Tripod tracking methods for demining operations
    Q_INVOKABLE void reportTripodInstalled(int vehicleId);
    Q_INVOKABLE bool isExplodeButtonEnabled() const;
    Q_INVOKABLE void executeExplode();
    Q_INVOKABLE void resetTripodTracking();
    
    // Test method for development/debugging
    Q_INVOKABLE void testMarkAllPayloadsInstalled();

    // Water avoidance settings (read from global AppSettings)
    Q_INVOKABLE bool waterAvoidanceEnabled() const;

private:
    void generateWaypointsForActiveVehicle(Vehicle* vehicle, const QGeoCoordinate& vehicleCoord, double frontDistanceMeters, bool payloadDropMode = false, int loiterTimeSeconds = 50, int bendHeight = 10, double payloadDropHeight = 1.5, int servoDelaySeconds = 3, double observationDistance = 100.0);

signals:
    void missionGenerationStarted();
    void missionGenerationProgress(int current, int total);
    void missionGenerationCompleted(bool success, const QString& message);
    void waypointsGenerated(const QList<QGeoCoordinate>& waypoints);
    void tripodInstalled(int vehicleId, int tripodCount, int totalTripods);
    void allTripodsInstalled();
    void explodeButtonEnabled(bool enabled);
    void deminingSuccess();

private slots:
    void onVehicleDataReady(const QList<QGeoCoordinate>& vehicleCoordinates);
    void onMissionUploadCompleted(bool success, const QString& message);

private:
    void processMissionGeneration(const QString& missionType, 
                                int areaSize, 
                                int altitude, 
                                double speed, 
                                const QString& description,
                                double frontDistance = 10.0);
    
    void generateWaypointsFromPosition(const QGeoCoordinate& vehiclePosition,
                                      int areaSize, 
                                      int altitude,
                                      double frontDistanceMeters,
                                      bool payloadDropMode = false,
                                      int loiterTimeSeconds = 50,
                                      int bendHeight = 10,
                                      double payloadDropHeight = 1.5,
                                      int servoDelaySeconds = 3,
                                      double observationDistance = 100.0);
    
    PtahMissionGenerator* m_ptahMissionGenerator;
    MissionUploadService* m_uploadService;
    VehicleService* m_vehicleService;
    
    // Current mission parameters for callbacks
    QString m_currentMissionType;
    int m_currentAreaSize;
    int m_currentAltitude;
    double m_currentSpeed;
    QString m_currentDescription;
    
    // Tripod tracking for demining operations
    int m_totalTripods;
    int m_installedTripods;
    QMap<int, int> m_vehicleTripodCount; // vehicleId -> tripod count
    bool m_explodeButtonEnabled;
};
