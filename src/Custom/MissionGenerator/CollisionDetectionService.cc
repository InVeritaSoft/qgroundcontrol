#include "CollisionDetectionService.h"
#include "Vehicle/MultiVehicleManager.h"
#include "Vehicle/Vehicle.h"
#include "QGCLoggingCategory.h"
#include <QTimer>
#include <QDebug>
#include <QGeoCoordinate>
#include <QtMath>

QGC_LOGGING_CATEGORY(CollisionDetectionLog, "CollisionDetectionLog")

CollisionDetectionService::CollisionDetectionService(QObject* parent)
    : QObject(parent)
    , m_monitoringTimer(new QTimer(this))
    , m_minAltitudeSeparation(10.0)
    , m_minWaypointSeparation(5.0)
    , m_monitoringActive(false)
{
    // Set up monitoring timer
    m_monitoringTimer->setSingleShot(false);
    connect(m_monitoringTimer, &QTimer::timeout, this, &CollisionDetectionService::performCollisionCheck);
    
    qCDebug(CollisionDetectionLog) << "CollisionDetectionService initialized";
}

void CollisionDetectionService::startCollisionMonitoring()
{
    if (m_monitoringActive) {
        qCDebug(CollisionDetectionLog) << "Collision monitoring already active";
        return;
    }
    
    qCDebug(CollisionDetectionLog) << "Starting collision monitoring";
    m_monitoringActive = true;
    m_monitoringTimer->start(1000); // Check every 1 second
}

void CollisionDetectionService::stopCollisionMonitoring()
{
    if (!m_monitoringActive) {
        qCDebug(CollisionDetectionLog) << "Collision monitoring not active";
        return;
    }
    
    qCDebug(CollisionDetectionLog) << "Stopping collision monitoring";
    m_monitoringActive = false;
    m_monitoringTimer->stop();
}

bool CollisionDetectionService::checkCollisionBetweenVehicles(Vehicle* vehicle1, Vehicle* vehicle2)
{
    if (!vehicle1 || !vehicle2 || vehicle1 == vehicle2) {
        return false;
    }
    
    QGeoCoordinate pos1 = vehicle1->coordinate();
    QGeoCoordinate pos2 = vehicle2->coordinate();
    
    if (!pos1.isValid() || !pos2.isValid()) {
        return false;
    }
    
    // Check altitude collision
    if (checkAltitudeCollision(pos1, pos2, m_minAltitudeSeparation)) {
        qCWarning(CollisionDetectionLog) << "Altitude collision detected between vehicle" << vehicle1->id() << "and vehicle" << vehicle2->id();
        emit collisionDetected(QString::number(vehicle1->id()), QString::number(vehicle2->id()), "Altitude");
        return true;
    }
    
    // Check horizontal proximity
    double horizontalDistance = pos1.distanceTo(pos2);
    if (horizontalDistance < m_minWaypointSeparation) {
        qCWarning(CollisionDetectionLog) << "Horizontal collision detected between vehicle" << vehicle1->id() << "and vehicle" << vehicle2->id() << "Distance:" << horizontalDistance << "m";
        emit collisionDetected(QString::number(vehicle1->id()), QString::number(vehicle2->id()), "Horizontal");
        return true;
    }
    
    return false;
}

bool CollisionDetectionService::checkAltitudeCollision(const QGeoCoordinate& pos1, const QGeoCoordinate& pos2, double minSeparation)
{
    if (!pos1.isValid() || !pos2.isValid()) {
        return false;
    }
    
    double altitudeDiff = qAbs(pos1.altitude() - pos2.altitude());
    bool collision = altitudeDiff < minSeparation;
    
    if (collision) {
        qCDebug(CollisionDetectionLog) << "Altitude collision check - Alt1:" << pos1.altitude() << "Alt2:" << pos2.altitude() << "Diff:" << altitudeDiff << "MinSep:" << minSeparation;
    }
    
    return collision;
}

bool CollisionDetectionService::checkWaypointCollision(const QGeoCoordinate& waypoint1, const QGeoCoordinate& waypoint2, double minSeparation)
{
    if (!waypoint1.isValid() || !waypoint2.isValid()) {
        return false;
    }
    
    double distance = waypoint1.distanceTo(waypoint2);
    bool collision = distance < minSeparation;
    
    if (collision) {
        qCDebug(CollisionDetectionLog) << "Waypoint collision check - Distance:" << distance << "MinSep:" << minSeparation;
    }
    
    return collision;
}

void CollisionDetectionService::showCollisionAlert(const QString& message)
{
    qCWarning(CollisionDetectionLog) << "COLLISION ALERT:" << message;
    emit collisionAlert(message);
}

void CollisionDetectionService::clearCollisionAlerts()
{
    qCDebug(CollisionDetectionLog) << "Clearing collision alerts";
    emit collisionCleared();
}

void CollisionDetectionService::setMinAltitudeSeparation(double meters)
{
    m_minAltitudeSeparation = meters;
    qCDebug(CollisionDetectionLog) << "Minimum altitude separation set to:" << meters << "meters";
}

void CollisionDetectionService::setMinWaypointSeparation(double meters)
{
    m_minWaypointSeparation = meters;
    qCDebug(CollisionDetectionLog) << "Minimum waypoint separation set to:" << meters << "meters";
}

void CollisionDetectionService::setMonitoringInterval(int milliseconds)
{
    if (m_monitoringActive) {
        m_monitoringTimer->stop();
        m_monitoringTimer->start(milliseconds);
    }
    qCDebug(CollisionDetectionLog) << "Monitoring interval set to:" << milliseconds << "milliseconds";
}

void CollisionDetectionService::performCollisionCheck()
{
    if (!m_monitoringActive) {
        return;
    }
    
    qCDebug(CollisionDetectionLog) << "Performing collision check";
    
    QList<Vehicle*> vehicles = getActiveVehicles();
    if (vehicles.size() < 2) {
        return; // Need at least 2 vehicles to check for collisions
    }
    
    // Check all vehicle pairs for collisions
    for (int i = 0; i < vehicles.size(); i++) {
        for (int j = i + 1; j < vehicles.size(); j++) {
            Vehicle* vehicle1 = vehicles[i];
            Vehicle* vehicle2 = vehicles[j];
            
            if (checkCollisionBetweenVehicles(vehicle1, vehicle2)) {
                QString message = formatCollisionMessage(
                    QString::number(vehicle1->id()),
                    QString::number(vehicle2->id()),
                    "Proximity"
                );
                showCollisionAlert(message);
            }
        }
    }
}

QList<Vehicle*> CollisionDetectionService::getActiveVehicles()
{
    QList<Vehicle*> vehicles;
    
    MultiVehicleManager* vehicleManager = MultiVehicleManager::instance();
    if (!vehicleManager) {
        qCWarning(CollisionDetectionLog) << "MultiVehicleManager not available";
        return vehicles;
    }
    
    QmlObjectListModel* vehicleModel = vehicleManager->vehicles();
    if (!vehicleModel) {
        qCWarning(CollisionDetectionLog) << "Vehicles model not available";
        return vehicles;
    }
    
    int vehicleCount = vehicleModel->count();
    for (int i = 0; i < vehicleCount; i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleModel->get(i));
        if (vehicle && vehicle->coordinate().isValid()) {
            vehicles.append(vehicle);
        }
    }
    
    return vehicles;
}

QString CollisionDetectionService::formatCollisionMessage(const QString& vehicle1Id, const QString& vehicle2Id, const QString& collisionType)
{
    return QString("COLLISION DETECTED: Vehicle %1 and Vehicle %2 - %3 Collision")
           .arg(vehicle1Id)
           .arg(vehicle2Id)
           .arg(collisionType);
}
