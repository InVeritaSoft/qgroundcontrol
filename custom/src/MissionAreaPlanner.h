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
#include <QGeoRectangle>
#include <QVariantList>

class MissionAreaPlanner : public QObject
{
    Q_OBJECT

public:
    explicit MissionAreaPlanner(QObject *parent = nullptr);

    // Area properties
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(double width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(double height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(double lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int pointsPerLine READ pointsPerLine WRITE setPointsPerLine NOTIFY pointsPerLineChanged)
    Q_PROPERTY(QGeoRectangle bounds READ bounds NOTIFY boundsChanged)
    Q_PROPERTY(QVariantList gridPoints READ gridPoints NOTIFY gridPointsChanged)
    Q_PROPERTY(QVariantList gridLines READ gridLines NOTIFY gridLinesChanged)

    // Getters
    QGeoCoordinate center() const { return _center; }
    double width() const { return _width; }
    double height() const { return _height; }
    double lineSpacing() const { return _lineSpacing; }
    int pointsPerLine() const { return _pointsPerLine; }
    QGeoRectangle bounds() const { return _bounds; }
    QVariantList gridPoints() const { return _gridPoints; }
    QVariantList gridLines() const { return _gridLines; }

    // Setters
    Q_INVOKABLE void setCenter(const QGeoCoordinate &center);
    Q_INVOKABLE void setWidth(double width);
    Q_INVOKABLE void setHeight(double height);
    Q_INVOKABLE void setLineSpacing(double spacing);
    Q_INVOKABLE void setPointsPerLine(int points);

    // Utility methods
    Q_INVOKABLE void updateGrid();
    Q_INVOKABLE void setAreaFromCenter(const QGeoCoordinate &center, double width, double height);
    Q_INVOKABLE QVariantList generateMissionWaypoints() const;

signals:
    void centerChanged();
    void widthChanged();
    void heightChanged();
    void lineSpacingChanged();
    void pointsPerLineChanged();
    void boundsChanged();
    void gridPointsChanged();
    void gridLinesChanged();

private:
    void calculateBounds();
    void generateGrid();
    QGeoCoordinate offsetCoordinate(const QGeoCoordinate &base, double latOffset, double lonOffset) const;

    QGeoCoordinate _center;
    double _width = 100.0;  // meters
    double _height = 100.0; // meters
    double _lineSpacing = 10.0; // meters
    int _pointsPerLine = 10;
    
    QGeoRectangle _bounds;
    QVariantList _gridPoints;
    QVariantList _gridLines;
}; 