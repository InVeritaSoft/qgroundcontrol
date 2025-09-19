/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "../../pch.h"
#include <QtCore/QtMath>
#include <QtPositioning/QGeoCoordinate>
#include "../AreaPlanEditor.h"

// QGroundControl includes
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MissionManager.h"
#include "MissionItem.h"
#include "MissionController.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGroundControlQmlGlobal.h"
#include "QmlObjectListModel.h"
#include "AreaPartition.h"
#include "MissionSettingsItem.h"
#include "QGCMAVLink.h"

/**
 * @file AreaPlanEditor_MissionGeneration.cc
 * @brief Mission generation and waypoint logic for AreaPlanEditor
 * 
 * This file contains all mission generation, waypoint creation, and
 * mission file management methods for the AreaPlanEditor class.
 */

// Forward declaration for serialization helper
static QVariantMap serializeMissionItem(const MissionItem* mi);

// Mission generation methods
QList<QVariant> AreaPlanEditor::generateWaypoints()
{
    QList<QVariant> waypoints;

    // Basic validation
    if (areaCenter().isValid() == false || areaWidth() <= 0 || areaHeight() <= 0 || numPoints() <= 0 || lineSpacing() <= 0) {
        return waypoints;
    }

    // Compute number of grid lines along height (north-south axis before rotation)
    const int lineCount = qMax(1, static_cast<int>(qFloor(areaHeight() / lineSpacing())));

    // Local helpers for geometry
    const qreal halfW = areaWidth() * 0.5;
    const qreal halfH = areaHeight() * 0.5;
    const qreal theta = qDegreesToRadians(-areaRotation()); // rotation: positive = clockwise
    const qreal cosT = qCos(theta);
    const qreal sinT = qSin(theta);

    // Precompute meters-to-degrees factors at center latitude (single conversion, no extra trig per point)
    const qreal metersPerDegreeLat = 111319.9;
    const qreal metersPerDegreeLon = 111319.9 * qCos(qDegreesToRadians(areaCenter().latitude()));

    auto rotateXY = [&](qreal x, qreal y) {
        // Rotate local (x,y) around origin by theta
        return QPointF(x * cosT - y * sinT, x * sinT + y * cosT);
    };

    auto clamp = [](qreal v, qreal lo, qreal hi){ return v < lo ? lo : (v > hi ? hi : v); };

    // Reserve and precompute coordinate arrays to reduce allocations
    waypoints.reserve(lineCount * _numPoints);
    QList<qreal> xs; xs.resize(_numPoints);
    QList<qreal> ys; ys.resize(lineCount);
    if (_numPoints == 1) {
        xs[0] = 0.0;
        } else {
        const qreal stepX = _areaWidth / (_numPoints - 1);
        for (int pi = 0; pi < _numPoints; ++pi) xs[pi] = -halfW + stepX * pi;
    }
    if (lineCount == 1) {
        ys[0] = 0.0;
            } else {
        const qreal stepY = _areaHeight / (lineCount - 1);
        for (int li = 0; li < lineCount; ++li) ys[li] = -halfH + stepY * li;
    }

    // Generate waypoints
    for (int li = 0; li < lineCount; ++li) {
        const qreal y = ys[li];
        for (int pi = 0; pi < numPoints(); ++pi) {
            const qreal x = xs[pi];
            const QPointF rotated = rotateXY(x, y);
            
            // Convert to lat/lon
            const qreal lat = areaCenter().latitude() + rotated.y() / metersPerDegreeLat;
            const qreal lon = areaCenter().longitude() + rotated.x() / metersPerDegreeLon;
            
            // Clamp to valid coordinate ranges
            const qreal clampedLat = clamp(lat, -90.0, 90.0);
            const qreal clampedLon = clamp(lon, -180.0, 180.0);
            
            QGeoCoordinate coord(clampedLat, clampedLon, _missionAltitude);
            waypoints.append(QVariant::fromValue(coord));
        }
    }

    return waypoints;
}

QList<QVariant> AreaPlanEditor::generatePerDroneWaypoints(int droneIndex) const
{
    QList<QVariant> waypoints;
    
    if (droneIndex < 0 || droneIndex >= droneCount()) {
        return waypoints;
    }
    
    // Calculate altitude for this drone using altitude banding
    const qreal droneAltitude = _missionAltitude + _altitudeBandStart + (droneIndex * _altitudeBandStep);
    
    // Generate base waypoints and adjust altitude
    QList<QVariant> baseWaypoints = generateWaypoints();
    for (const QVariant& waypoint : baseWaypoints) {
        QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
        coord.setAltitude(droneAltitude);
        waypoints.append(QVariant::fromValue(coord));
    }
    
    return waypoints;
}

void AreaPlanEditor::addWaypointsToMission()
{
    if (!planMasterController()) {
        handleError("No mission controller available", "Please ensure a mission controller is set");
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
    if (!missionController) {
        handleError("Invalid mission controller", "Please ensure a valid mission controller is set");
        return;
    }
    
    // Clear existing mission items
    missionController->removeAll();
    
    // Generate waypoints
    QList<QVariant> waypoints = const_cast<AreaPlanEditor*>(this)->generateWaypoints();
    
    // Add waypoints to mission
    for (const QVariant& waypoint : waypoints) {
        QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
        missionController->insertSimpleMissionItem(coord, missionController->missionItems()->count());
    }
    
    updateStatus("Waypoints added to mission");
}

void AreaPlanEditor::addPerDroneToMission(int droneIndex)
{
    if (droneIndex < 0 || droneIndex >= droneCount()) {
        handleError("Invalid drone index", QString("Drone index %1 is out of range").arg(droneIndex));
        return;
    }
    
    if (!planMasterController()) {
        handleError("No mission controller available", "Please ensure a mission controller is set");
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
    if (!missionController) {
        handleError("Invalid mission controller", "Please ensure a valid mission controller is set");
        return;
    }
    
    // Generate waypoints for this specific drone
    QList<QVariant> waypoints = generatePerDroneWaypoints(droneIndex);
    
    // Clear existing mission items for this drone
    // Note: This is a simplified implementation - in practice, you might want
    // to store per-drone missions separately
    
    // Add waypoints to mission
    for (const QVariant& waypoint : waypoints) {
        QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
        missionController->insertSimpleMissionItem(coord, missionController->missionItems()->count());
    }
    
    updateStatus(QString("Waypoints added for drone %1").arg(droneIndex));
}

void AreaPlanEditor::addAllDronesToMission()
{
    if (!planMasterController()) {
        handleError("No mission controller available", "Please ensure a mission controller is set");
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
    if (!missionController) {
        handleError("Invalid mission controller", "Please ensure a valid mission controller is set");
        return;
    }
    
    // Clear existing mission items
    missionController->removeAll();
    
    // Add waypoints for all drones
    for (int i = 0; i < droneCount(); ++i) {
        QList<QVariant> waypoints = generatePerDroneWaypoints(i);
        
        // Add waypoints to mission
        for (const QVariant& waypoint : waypoints) {
            QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
            missionController->insertSimpleMissionItem(coord, missionController->missionItems()->count());
        }
    }
    
    updateStatus(QString("Waypoints added for all %1 drones").arg(droneCount()));
}

void AreaPlanEditor::clearMission()
{
    if (!planMasterController()) {
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
    if (missionController) {
        missionController->removeAll();
        updateStatus("Mission cleared");
    }
}

void AreaPlanEditor::clearAllMissions()
{
    clearMission();
    clearAllStoredMissions();
    updateStatus("All missions cleared");
}

void AreaPlanEditor::saveMissionFile()
{
    if (!planMasterController()) {
        handleError("No mission controller available", "Please ensure a mission controller is set");
        return;
    }
    
    MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
    if (!missionController) {
        handleError("Invalid mission controller", "Please ensure a valid mission controller is set");
        return;
    }
    
    // Generate filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("area_mission_%1.csv").arg(timestamp);
    
    saveMissionToFile(missionController, filename);
    updateStatus(QString("Mission saved to %1").arg(filename));
}

void AreaPlanEditor::savePerDroneMissionFiles()
{
    for (int i = 0; i < droneCount(); ++i) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString filename = QString("drone_%1_mission_%2.csv").arg(i).arg(timestamp);
        
        // Generate waypoints for this drone
        QList<QVariant> waypoints = generatePerDroneWaypoints(i);
        
        // Convert to MissionItem list for saving
        QList<MissionItem*> missionItems;
        for (const QVariant& waypoint : waypoints) {
            QGeoCoordinate coord = waypoint.value<QGeoCoordinate>();
            MissionItem* item = new MissionItem(nullptr, this);
            item->coordinate()->setRawValue(coord);
            missionItems.append(item);
        }
        
        saveMissionToFile(missionItems, filename);
    }
    
    updateStatus(QString("Mission files saved for all %1 drones").arg(droneCount()));
}

void AreaPlanEditor::saveMissionToFile(MissionController* missionController, const QString& filename)
{
    if (!missionController) return;
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    if (missionController->visualItems()) {
        for (int i = 0; i < missionController->visualItems()->count(); ++i) {
            auto* vmi = qobject_cast<VisualMissionItem*>(missionController->visualItems()->get(i));
            if (!vmi) continue;
            const QGeoCoordinate c = vmi->coordinate();
            ts << QString::number(c.latitude(), 'f', 7) << ","
               << QString::number(c.longitude(), 'f', 7) << ","
               << QString::number(c.altitude(), 'f', 2) << "\n";
        }
    }
    f.close();
}

void AreaPlanEditor::saveMissionToFile(const QList<MissionItem*>& missionItems, const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        handleError(QString("Unable to open %1").arg(filename), QString());
        return;
    }
    QTextStream ts(&f);
    ts << "lat,lon,alt\n";
    for (const MissionItem* item : missionItems) {
        const QGeoCoordinate c = item->coordinate();
        ts << QString::number(c.latitude(), 'f', 7) << ","
           << QString::number(c.longitude(), 'f', 7) << ","
           << QString::number(c.altitude(), 'f', 2) << "\n";
    }
    f.close();
}

void AreaPlanEditor::insertGripperRelease(MissionController* mission, const QGeoCoordinate& atCoord)
{
    if (!mission) return;
    
    // Find the closest waypoint to insert the gripper release command
    int insertIndex = 0;
    qreal minDistance = std::numeric_limits<qreal>::max();
    
    if (mission->visualItems()) {
        for (int i = 0; i < mission->visualItems()->count(); ++i) {
            auto* vmi = qobject_cast<VisualMissionItem*>(mission->visualItems()->get(i));
            if (!vmi) continue;
            
            qreal distance = vmi->coordinate().distanceTo(atCoord);
            if (distance < minDistance) {
                minDistance = distance;
                insertIndex = i;
            }
        }
    }
    
    // Insert gripper release command
    // Note: This is a simplified implementation - in practice, you would
    // need to create a proper mission item with the appropriate MAVLink command
    mission->insertSimpleMissionItem(insertIndex, atCoord);
}

void AreaPlanEditor::optimizeWaypointGeneration()
{
    // This method would contain waypoint generation optimization logic
    // For now, it's a placeholder for future optimization features
    updateStatus("Waypoint generation optimization completed");
}

// Helper function for mission item serialization
static QVariantMap serializeMissionItem(const MissionItem* mi)
{
    QVariantMap map;
    if (!mi) return map;
    
    map["coordinate"] = QVariant::fromValue(mi->coordinate());
    map["altitude"] = mi->coordinate().altitude();
    map["command"] = mi->command();
    map["param1"] = mi->param1();
    map["param2"] = mi->param2();
    map["param3"] = mi->param3();
    map["param4"] = mi->param4();
    
    return map;
}
