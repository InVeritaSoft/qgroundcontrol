/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>

class AreaPlanEditor : public QObject
{
    Q_OBJECT

public:
    explicit AreaPlanEditor(QObject* parent = nullptr);
    ~AreaPlanEditor() = default;

    // Properties
    Q_PROPERTY(qreal areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
    Q_PROPERTY(qreal areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int numPoints READ numPoints WRITE setNumPoints NOTIFY numPointsChanged)
    Q_PROPERTY(qreal missionAltitude READ missionAltitude WRITE setMissionAltitude NOTIFY missionAltitudeChanged)
    Q_PROPERTY(QGeoCoordinate areaCenter READ areaCenter WRITE setAreaCenter NOTIFY areaCenterChanged)
    Q_PROPERTY(QGeoCoordinate homeLocation READ homeLocation WRITE setHomeLocation NOTIFY homeLocationChanged)

    // Property getters
    qreal areaWidth() const { return _areaWidth; }
    qreal areaHeight() const { return _areaHeight; }
    qreal lineSpacing() const { return _lineSpacing; }
    int numPoints() const { return _numPoints; }
    qreal missionAltitude() const { return _missionAltitude; }
    QGeoCoordinate areaCenter() const { return _areaCenter; }
    QGeoCoordinate homeLocation() const { return _homeLocation; }

    // Property setters
    void setAreaWidth(qreal width);
    void setAreaHeight(qreal height);
    void setLineSpacing(qreal spacing);
    void setNumPoints(int points);
    void setMissionAltitude(qreal altitude);
    void setAreaCenter(const QGeoCoordinate& center);
    void setHomeLocation(const QGeoCoordinate& location);

    // Mission planning functions
    Q_INVOKABLE void moveAreaNorth();
    Q_INVOKABLE void moveAreaSouth();
    Q_INVOKABLE void moveAreaEast();
    Q_INVOKABLE void moveAreaWest();
    Q_INVOKABLE void centerArea();
    Q_INVOKABLE int calculateTotalWaypoints() const;
    Q_INVOKABLE int calculateFlightTime() const;
    Q_INVOKABLE QVariantList generateWaypoints();
    Q_INVOKABLE void saveMissionFile();
    Q_INVOKABLE void uploadToVehicle();
    Q_INVOKABLE void startMission();
    Q_INVOKABLE void updateStatus(const QString& message);

signals:
    void areaWidthChanged();
    void areaHeightChanged();
    void lineSpacingChanged();
    void numPointsChanged();
    void missionAltitudeChanged();
    void areaCenterChanged();
    void homeLocationChanged();
    void statusChanged(const QString& message);

private:
    QGeoCoordinate calculateOffsetCoordinate(const QGeoCoordinate& coord, qreal meters, qreal bearing) const;

    // Default values
    static constexpr qreal _defaultAreaWidth = 30.0;
    static constexpr qreal _defaultAreaHeight = 90.0;
    static constexpr qreal _defaultLineSpacing = 3.0;
    static constexpr int _defaultNumPoints = 1;
    static constexpr qreal _defaultAltitude = 10.0;

    // Properties
    qreal _areaWidth = _defaultAreaWidth;
    qreal _areaHeight = _defaultAreaHeight;
    qreal _lineSpacing = _defaultLineSpacing;
    int _numPoints = _defaultNumPoints;
    qreal _missionAltitude = _defaultAltitude;
    QGeoCoordinate _areaCenter = QGeoCoordinate(49.82824897481479, 24.033390804256005);
    QGeoCoordinate _homeLocation = QGeoCoordinate(49.82824897481479, 24.033390804256005);
}; 