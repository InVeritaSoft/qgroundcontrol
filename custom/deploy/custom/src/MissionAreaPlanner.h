/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>

class MissionAreaPlanner : public QObject
{
    Q_OBJECT

public:
    explicit MissionAreaPlanner(QObject* parent = nullptr);

    Q_PROPERTY(QGeoCoordinate areaCenter READ areaCenter WRITE setAreaCenter NOTIFY areaCenterChanged)
    Q_PROPERTY(double areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
    Q_PROPERTY(double areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
    Q_PROPERTY(double lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int pointsPerLine READ pointsPerLine WRITE setPointsPerLine NOTIFY pointsPerLineChanged)
    Q_PROPERTY(QVariantList waypoints READ waypoints NOTIFY waypointsChanged)
    Q_PROPERTY(QVariantList gridLines READ gridLines NOTIFY gridLinesChanged)
    Q_PROPERTY(QVariantList areaCorners READ areaCorners NOTIFY areaCornersChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QColor statusColor READ statusColor NOTIFY statusColorChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

    // Property getters
    QGeoCoordinate areaCenter() const { return _areaCenter; }
    double areaWidth() const { return _areaWidth; }
    double areaHeight() const { return _areaHeight; }
    double lineSpacing() const { return _lineSpacing; }
    int pointsPerLine() const { return _pointsPerLine; }
    QVariantList waypoints() const { return _waypoints; }
    QVariantList gridLines() const { return _gridLines; }
    QVariantList areaCorners() const { return _areaCorners; }
    QString status() const { return _status; }
    QColor statusColor() const { return _statusColor; }
    bool busy() const { return _busy; }

    // Property setters
    Q_INVOKABLE void setAreaCenter(const QGeoCoordinate& center);
    Q_INVOKABLE void setAreaWidth(double width);
    Q_INVOKABLE void setAreaHeight(double height);
    Q_INVOKABLE void setLineSpacing(double spacing);
    Q_INVOKABLE void setPointsPerLine(int points);

    // Public methods
    Q_INVOKABLE void updateArea();
    Q_INVOKABLE void updateGrid();
    Q_INVOKABLE void moveArea(double deltaX, double deltaY);
    Q_INVOKABLE void generateMission();

    // Geodesic calculation methods
    Q_INVOKABLE double geodesicDistance(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2);
    Q_INVOKABLE QGeoCoordinate coordinateAtDistance(const QGeoCoordinate& referenceCoord, double distance, double bearing);
    Q_INVOKABLE double calculateBearing(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2);

signals:
    void areaCenterChanged();
    void areaWidthChanged();
    void areaHeightChanged();
    void lineSpacingChanged();
    void pointsPerLineChanged();
    void waypointsChanged();
    void gridLinesChanged();
    void areaCornersChanged();
    void statusChanged();
    void statusColorChanged();
    void busyChanged();
    void areaUpdated();
    void gridUpdated();
    void missionGenerated();

private:
    void setStatus(const QString& status, const QColor& color);
    void setBusy(bool busy);
    QGeoCoordinate interpolateCoordinate(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2, double progress);

    // Properties
    QGeoCoordinate _areaCenter;
    double _areaWidth;
    double _areaHeight;
    double _lineSpacing;
    int _pointsPerLine;
    QVariantList _waypoints;
    QVariantList _gridLines;
    QVariantList _areaCorners;
    QString _status;
    QColor _statusColor;
    bool _busy;

    // Constants
    static constexpr double EARTH_RADIUS = 6371000.0; // meters
    static constexpr double PI = 3.14159265358979323846;
}; 