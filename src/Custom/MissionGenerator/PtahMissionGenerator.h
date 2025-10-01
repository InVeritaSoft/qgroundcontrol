#pragma once

#include <QObject>
#include <QList>
#include <QtPositioning/QGeoCoordinate>

class PtahMissionGenerator : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PtahMissionGenerator(QObject* parent = nullptr);
    
    // Calculate the middle point (centroid) from a set of coordinates
    Q_INVOKABLE QGeoCoordinate calculateMiddlePoint(const QList<QGeoCoordinate>& coordinates);
    
    // Calculate new coordinates based on bearing and distance from a reference point
    Q_INVOKABLE QGeoCoordinate calculateNewCoordinates(const QGeoCoordinate& reference, double bearing, double distanceMeters);
    
    // Generate coordinates in both directions from a reference point with specified gap
    Q_INVOKABLE QList<QGeoCoordinate> generateCoordinatesInBothDirections(const QGeoCoordinate& reference, double bearing, double gapMeters, double totalDistanceMeters);
    
    // Calculate bearing between two coordinates
    Q_INVOKABLE double calculateBearing(const QGeoCoordinate& from, const QGeoCoordinate& to);
    
    // Distribute waypoints among multiple drones
    Q_INVOKABLE QList<QList<QGeoCoordinate>> distributeWaypointsAmongDrones(const QList<QGeoCoordinate>& waypoints, int droneCount);
    
    // Calculate safe observation position for drone ID 1
    Q_INVOKABLE QGeoCoordinate calculateSafeObservationPosition(const QGeoCoordinate& missionCenter, const QList<QGeoCoordinate>& waypoints, int altitude);
    
    // Calculate observation position with configurable distance from mission center
    Q_INVOKABLE QGeoCoordinate calculateObservationPosition(const QGeoCoordinate& missionCenter, double observationDistanceMeters, int altitude);
    
    // Calculate bearing from observer position to mission center
    Q_INVOKABLE double calculateBearingToMissionCenter(const QGeoCoordinate& observerPosition, const QGeoCoordinate& missionCenter);
    
    // Generate enhanced waypoints with payload drop pattern
    Q_INVOKABLE QList<QGeoCoordinate> generatePayloadDropWaypoints(const QGeoCoordinate& reference, double bearing, double gapMeters, double totalDistanceMeters, int loiterTimeSeconds = 50);
    
    // Create mission items for payload drop pattern
    Q_INVOKABLE QList<QObject*> createPayloadDropMissionItems(const QList<QGeoCoordinate>& waypoints, int altitude, int loiterTimeSeconds = 50);
    
    // Validate coordinates to prevent waypoints in oceans or invalid locations
    Q_INVOKABLE bool isValidLandCoordinate(const QGeoCoordinate& coordinate);
};