#pragma once

#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>
#include "MAVLinkProtocol.h"
#include "MAVLinkLib.h"

class Vehicle;
class MissionItem;

class MissionUploadService : public QObject
{
    Q_OBJECT

public:
    explicit MissionUploadService(QObject* parent = nullptr);

    void uploadMissionsToAllVehicles(const QList<QGeoCoordinate>& waypoints, int altitude);
    void uploadMissionToVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude);
    void uploadLoiterMissionToVehicle(Vehicle* vehicle, const QGeoCoordinate& loiterPosition, int altitude, const QGeoCoordinate& missionCenter = QGeoCoordinate());
    void uploadPayloadDropMissionToVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude, int loiterTimeSeconds, int bendHeight, double payloadDropHeight, int servoDelaySeconds);
    void clearMission(Vehicle* vehicle);

signals:
    void missionUploadCompleted(bool success, const QString& message);
    void missionUploadProgress(int current, int total);

private:
    bool createMissionForVehicle(Vehicle* vehicle, const QList<QGeoCoordinate>& waypoints, int altitude);
    QList<Vehicle*> getConnectedVehicles();
    
    // Human-readable mission item logging
    QString getHumanReadableCommandDescription(MAV_CMD command, const QList<double>& params, int sequenceNumber);
    QString getShortMissionDescription(const QString& fullDescription);
    void logMissionItem(const QString& vehicleId, int sequenceNumber, MAV_CMD command, const QList<double>& params, const QString& description);
    void logMissionItemToVehicle(Vehicle* vehicle, int sequenceNumber, const QString& description);
    void exportHumanReadableMission(const QString& vehicleId, const QList<MissionItem*>& missionItems, const QString& filename = QString());
};
