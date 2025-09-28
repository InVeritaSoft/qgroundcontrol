#include "VehicleService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "QGCLoggingCategory.h"
#include <QObject>
#include <QString>
#include <QGeoCoordinate>
#include <QList>
#include <QVariantList>
#include <QVariantMap>

QGC_LOGGING_CATEGORY(VehicleServiceLog, "VehicleServiceLog")

VehicleService::VehicleService(QObject* parent)
    : QObject(parent)
{
}

MultiVehicleManager* VehicleService::getVehicleManager()
{
    return MultiVehicleManager::instance();
}

QList<Vehicle*> VehicleService::getConnectedVehicles()
{
    QList<Vehicle*> vehicles;
    
    MultiVehicleManager* vehicleManager = getVehicleManager();
    if (!vehicleManager) {
        qCWarning(VehicleServiceLog) << "MultiVehicleManager not available";
        return vehicles;
    }
    
    QmlObjectListModel* vehicleModel = vehicleManager->vehicles();
    if (!vehicleModel) {
        qCWarning(VehicleServiceLog) << "Vehicles model not available";
        return vehicles;
    }
    
    int vehicleCount = vehicleModel->count();
    for (int i = 0; i < vehicleCount; i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle) {
            vehicles.append(vehicle);
        }
    }
    
    return vehicles;
}

void VehicleService::getAllVehicles()
{
    qCDebug(VehicleServiceLog) << "Getting all vehicles";
    
    QList<Vehicle*> vehicles = getConnectedVehicles();
    
    QString vehicleInfo;
    QVariantList vehicleList;
    int vehicleCount = vehicles.size();
    
    qCDebug(VehicleServiceLog) << "Total vehicles:" << vehicleCount;
    
    if (vehicleCount > 0) {
        for (int i = 0; i < vehicleCount; i++) {
            Vehicle* vehicle = vehicles[i];
            QGeoCoordinate coord = vehicle->coordinate();
            
            // Create structured data for each vehicle
            QVariantMap vehicleData;
            vehicleData["index"] = i;
            vehicleData["id"] = vehicle->id();
            vehicleData["latitude"] = coord.latitude();
            vehicleData["longitude"] = coord.longitude();
            vehicleData["altitude"] = coord.altitude();
            vehicleData["coordinateString"] = coord.toString();
            
            vehicleList.append(vehicleData);
            
            // Also create formatted text for display
            QString vehicleText = QString("Vehicle %1: ID=%2, Lat=%3, Lng=%4, Alt=%5m")
                .arg(i)
                .arg(vehicle->id())
                .arg(coord.latitude(), 0, 'f', 7)
                .arg(coord.longitude(), 0, 'f', 7)
                .arg(coord.altitude(), 0, 'f', 2);
            
            qCDebug(VehicleServiceLog) << vehicleText;
            vehicleInfo += vehicleText + "\n";
        }
        vehicleInfo += QString("Total vehicles: %1").arg(vehicleCount);
    } else {
        qCDebug(VehicleServiceLog) << "No vehicles available";
        vehicleInfo = "No vehicles available";
    }
    
    // Emit both the formatted string and structured data
    emit vehicleInfoReady(vehicleInfo);
    emit vehicleDataReady(vehicleList);
}

void VehicleService::getAllVehicleCoordinates()
{
    // GenCall12: VehicleService::getAllVehicleCoordinates() - Collect vehicle positions
    qCDebug(VehicleServiceLog) << "GenCall12: VehicleService::getAllVehicleCoordinates() - Collecting vehicle positions";
    qCDebug(VehicleServiceLog) << "Getting all vehicle coordinates";
    
    // GenCall13: Get connected vehicles
    qCDebug(VehicleServiceLog) << "GenCall13: Getting connected vehicles";
    QList<Vehicle*> vehicles = getConnectedVehicles();
    QList<QGeoCoordinate> coordinates;
    
    // GenCall14: Extract coordinates from each vehicle
    qCDebug(VehicleServiceLog) << "GenCall14: Extracting coordinates from each vehicle";
    for (Vehicle* vehicle : vehicles) {
        QGeoCoordinate coord = vehicle->coordinate();
        if (coord.isValid()) {
            coordinates.append(coord);
            qCDebug(VehicleServiceLog) << "Added vehicle coordinate:" << coord.toString();
            qCDebug(VehicleServiceLog) << "Vehicle ID:" << vehicle->id() << "Lat:" << coord.latitude() << "Lng:" << coord.longitude() << "Alt:" << coord.altitude();
        } else {
            qCDebug(VehicleServiceLog) << "Vehicle ID:" << vehicle->id() << "has invalid coordinates";
        }
    }
    
    // GenCall15: Emit collected coordinates
    qCDebug(VehicleServiceLog) << "GenCall15: Emitting collected coordinates";
    qCDebug(VehicleServiceLog) << "Collected" << coordinates.size() << "valid coordinates";
    emit vehicleCoordinatesReady(coordinates);
}
