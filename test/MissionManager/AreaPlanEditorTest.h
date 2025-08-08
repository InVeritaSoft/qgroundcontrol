/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MissionControllerManagerTest.h"

class AreaPlanEditor;
class PlanMasterController;
class MissionController;

class AreaPlanEditorTest : public MissionControllerManagerTest {
    Q_OBJECT

public:
    AreaPlanEditorTest() = default;

private slots:
    void _basicProperties();
    void _generateWaypointsAndAddToMission();
    void _multiDroneDefaultsAndSetters();
    void _perDronePreviewCounts();
    void _balancedPartition();
    void _boundsAndRotation();
    void _rotationHandling();
};


