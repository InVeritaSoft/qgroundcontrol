#ifndef COLLISIONDETECTIONSERVICE_H
#define COLLISIONDETECTIONSERVICE_H

#include <QObject>
#include <QGeoCoordinate>
#include <QList>
#include <QTimer>
#include <QString>

class Vehicle;
class QGeoCoordinate;

class CollisionDetectionService : public QObject
{
    Q_OBJECT

public:
    explicit CollisionDetectionService(QObject* parent = nullptr);
    
    // Collision detection methods
    Q_INVOKABLE void startCollisionMonitoring();
    Q_INVOKABLE void stopCollisionMonitoring();
    Q_INVOKABLE bool checkCollisionBetweenVehicles(Vehicle* vehicle1, Vehicle* vehicle2);
    Q_INVOKABLE bool checkAltitudeCollision(const QGeoCoordinate& pos1, const QGeoCoordinate& pos2, double minSeparation = 10.0);
    Q_INVOKABLE bool checkWaypointCollision(const QGeoCoordinate& waypoint1, const QGeoCoordinate& waypoint2, double minSeparation = 5.0);
    
    // Alert system
    Q_INVOKABLE void showCollisionAlert(const QString& message);
    Q_INVOKABLE void clearCollisionAlerts();
    
    // Configuration
    Q_INVOKABLE void setMinAltitudeSeparation(double meters);
    Q_INVOKABLE void setMinWaypointSeparation(double meters);
    Q_INVOKABLE void setMonitoringInterval(int milliseconds);

signals:
    void collisionDetected(const QString& vehicle1Id, const QString& vehicle2Id, const QString& collisionType);
    void collisionAlert(const QString& message);
    void collisionCleared();

private slots:
    void performCollisionCheck();

private:
    QTimer* m_monitoringTimer;
    double m_minAltitudeSeparation;
    double m_minWaypointSeparation;
    bool m_monitoringActive;
    
    QList<Vehicle*> getActiveVehicles();
    QString formatCollisionMessage(const QString& vehicle1Id, const QString& vehicle2Id, const QString& collisionType);
};

#endif // COLLISIONDETECTIONSERVICE_H
