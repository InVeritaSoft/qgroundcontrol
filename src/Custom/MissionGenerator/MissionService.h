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
                                    const QString& description);

private:
    void generateWaypointsForActiveVehicle(Vehicle* vehicle, const QGeoCoordinate& vehicleCoord, double frontDistanceMeters);

signals:
    void missionGenerationStarted();
    void missionGenerationProgress(int current, int total);
    void missionGenerationCompleted(bool success, const QString& message);
    void waypointsGenerated(const QList<QGeoCoordinate>& waypoints);

private slots:
    void onVehicleDataReady(const QList<QGeoCoordinate>& vehicleCoordinates);
    void onMissionUploadCompleted(bool success, const QString& message);

private:
    void processMissionGeneration(const QString& missionType, 
                                int areaSize, 
                                int altitude, 
                                double speed, 
                                const QString& description);
    
    void generateWaypointsFromPosition(const QGeoCoordinate& vehiclePosition,
                                      int areaSize, 
                                      int altitude,
                                      double frontDistanceMeters);
    
    PtahMissionGenerator* m_ptahMissionGenerator;
    MissionUploadService* m_uploadService;
    VehicleService* m_vehicleService;
    
    // Current mission parameters for callbacks
    QString m_currentMissionType;
    int m_currentAreaSize;
    int m_currentAltitude;
    double m_currentSpeed;
    QString m_currentDescription;
};
