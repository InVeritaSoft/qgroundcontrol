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
#include <QList>
#include <QVariantList>

class MissionAreaPlanner : public QObject
{
    Q_OBJECT

public:
    explicit MissionAreaPlanner(QObject* parent = nullptr);

    Q_PROPERTY(double width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(double height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(double lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(int numPoints READ numPoints WRITE setNumPoints NOTIFY numPointsChanged)
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

    double width() const { return _width; }
    double height() const { return _height; }
    double lineSpacing() const { return _lineSpacing; }
    int numPoints() const { return _numPoints; }
    QGeoCoordinate center() const { return _center; }
    bool busy() const { return _busy; }
    QString status() const { return _status; }

    void setWidth(double width);
    void setHeight(double height);
    void setLineSpacing(double spacing);
    void setNumPoints(int points);
    void setCenter(const QGeoCoordinate& center);

    Q_INVOKABLE void generateMission();
    Q_INVOKABLE void clearMission();

signals:
    void widthChanged();
    void heightChanged();
    void lineSpacingChanged();
    void numPointsChanged();
    void centerChanged();
    void busyChanged();
    void statusChanged();

private:
    void updateStatus(const QString& status);
    QList<QGeoCoordinate> calculateWaypoints() const;

    double _width = 30.0;
    double _height = 90.0;
    double _lineSpacing = 3.0;
    int _numPoints = 1;
    QGeoCoordinate _center = QGeoCoordinate(49.82824897481479, 24.033390804256005);
    bool _busy = false;
    QString _status = "Ready";
}; 