#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QList>
#include <QVariant>
#include <QVariantList>
#include <QPointF>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class PlanMasterController;

class AreaPlanEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit AreaPlanEditor(QObject* parent = nullptr);

    // Drawing mode control
    Q_PROPERTY(bool isDrawingMode READ isDrawingMode WRITE setIsDrawingMode NOTIFY isDrawingModeChanged)

    // Area definition properties
    Q_PROPERTY(qreal areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
    Q_PROPERTY(qreal areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int numPoints READ numPoints WRITE setNumPoints NOTIFY numPointsChanged)
    Q_PROPERTY(qreal missionAltitude READ missionAltitude WRITE setMissionAltitude NOTIFY missionAltitudeChanged)
    Q_PROPERTY(QGeoCoordinate areaCenter READ areaCenter WRITE setAreaCenter NOTIFY areaCenterChanged)
    Q_PROPERTY(qreal areaRotation READ areaRotation WRITE setAreaRotation NOTIFY areaRotationChanged)

    // Multi-drone planning properties
    Q_PROPERTY(int droneCount READ droneCount WRITE setDroneCount NOTIFY droneCountChanged)
    Q_PROPERTY(qreal altitudeBandStart READ altitudeBandStart WRITE setAltitudeBandStart NOTIFY altitudeBandStartChanged)
    Q_PROPERTY(qreal altitudeBandStep READ altitudeBandStep WRITE setAltitudeBandStep NOTIFY altitudeBandStepChanged)

    // Plan master controller
    Q_PROPERTY(PlanMasterController* planMasterController READ planMasterController WRITE setPlanMasterController NOTIFY planMasterControllerChanged)

    // Getters
    bool isDrawingMode() const { return _isDrawingMode; }
    qreal areaWidth() const { return _areaWidth; }
    qreal areaHeight() const { return _areaHeight; }
    qreal lineSpacing() const { return _lineSpacing; }
    int numPoints() const { return _numPoints; }
    qreal missionAltitude() const { return _missionAltitude; }
    QGeoCoordinate areaCenter() const { return _areaCenter; }
    qreal areaRotation() const { return _areaRotation; }
    int droneCount() const { return _droneCount; }
    qreal altitudeBandStart() const { return _altitudeBandStart; }
    qreal altitudeBandStep() const { return _altitudeBandStep; }
    PlanMasterController* planMasterController() const { return _planMasterController; }

    // Setters
    Q_INVOKABLE void setIsDrawingMode(bool drawingMode);
    Q_INVOKABLE void setAreaWidth(qreal width);
    Q_INVOKABLE void setAreaHeight(qreal height);
    Q_INVOKABLE void setLineSpacing(qreal spacing);
    Q_INVOKABLE void setNumPoints(int points);
    Q_INVOKABLE void setMissionAltitude(qreal altitude);
    Q_INVOKABLE void setAreaCenter(const QGeoCoordinate& center);
    Q_INVOKABLE void setAreaRotation(qreal rotation);
    Q_INVOKABLE void setDroneCount(int count);
    Q_INVOKABLE void setAltitudeBandStart(qreal start);
    Q_INVOKABLE void setAltitudeBandStep(qreal step);
    void setPlanMasterController(PlanMasterController* controller);

    // Area manipulation
    Q_INVOKABLE void moveAreaNorth();
    Q_INVOKABLE void moveAreaSouth();
    Q_INVOKABLE void moveAreaEast();
    Q_INVOKABLE void moveAreaWest();
    Q_INVOKABLE void rotateAreaClockwise();
    Q_INVOKABLE void rotateAreaCounterClockwise();
    Q_INVOKABLE void centerArea();

    // Mission generation
    Q_INVOKABLE QVariantList generateWaypoints();
    Q_INVOKABLE QVariantList computePerDroneWaypointPreview() const;
    Q_INVOKABLE void addWaypointsToMission();
    Q_INVOKABLE void saveMissionFile();

signals:
    void isDrawingModeChanged();
    void areaWidthChanged();
    void areaHeightChanged();
    void lineSpacingChanged();
    void numPointsChanged();
    void missionAltitudeChanged();
    void areaCenterChanged();
    void areaRotationChanged();
    void droneCountChanged();
    void altitudeBandStartChanged();
    void altitudeBandStepChanged();
    void planMasterControllerChanged();
    void waypointsAddedToMission(int count);
    void missionSaved();

private:
    QGeoCoordinate calculateOffsetCoordinate(const QGeoCoordinate& coordinate, qreal distanceMeters, qreal bearingDegrees) const;

    // Properties
    bool _isDrawingMode = false;
    qreal _areaWidth = 0.0;
    qreal _areaHeight = 0.0;
    qreal _lineSpacing = 10.0;
    int _numPoints = 5;
    qreal _missionAltitude = 50.0;
    QGeoCoordinate _areaCenter;
    qreal _areaRotation = 0.0;
    int _droneCount = 1;
    qreal _altitudeBandStart = 0.0;
    qreal _altitudeBandStep = 10.0;
    PlanMasterController* _planMasterController = nullptr;

    // Constants
    static constexpr qreal _moveStepMeters = 10.0;
    static constexpr qreal _rotationStepDegrees = 15.0;
    static constexpr qreal _timeOffsetPerDrone = 30.0; // seconds
};
