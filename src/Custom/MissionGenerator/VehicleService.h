#pragma once

#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>
#include <QVariantList>

class MultiVehicleManager;
class Vehicle;

class VehicleService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit VehicleService(QObject* parent = nullptr);

    void getAllVehicles();
    void getAllVehicleCoordinates();

signals:
    void vehicleInfoReady(const QString& vehicleInfo);
    void vehicleDataReady(const QVariantList& vehicleList);
    void vehicleCoordinatesReady(const QList<QGeoCoordinate>& coordinates);

private:
    MultiVehicleManager* getVehicleManager();
    QList<Vehicle*> getConnectedVehicles();
};
