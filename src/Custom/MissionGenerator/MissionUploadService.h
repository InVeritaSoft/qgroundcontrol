#pragma once

#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>

class Vehicle;
class MissionItem;

class MissionUploadService : public QObject
{
    Q_OBJECT

public:
    explicit MissionUploadService(QObject* parent = nullptr);

    void uploadMissionsToAllVehicles(const QList<QGeoCoordinate>& waypoints, int altitude);
    void uploadMissionToVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude);
    void uploadLoiterMissionToVehicle(Vehicle* vehicle, const QGeoCoordinate& loiterPosition, int altitude);

signals:
    void missionUploadCompleted(bool success, const QString& message);
    void missionUploadProgress(int current, int total);

private:
    bool createMissionForVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude);
    QList<Vehicle*> getConnectedVehicles();
};
