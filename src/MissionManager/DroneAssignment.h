/****************************************************************************
 *
 * (c) 2009-2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <vector>

namespace AreaPlan {

struct DroneAssignment {
    int         droneIndex {0};
    double      altitudeOffsetM {0.0};
    double      timeOffsetS {0.0};
    std::vector<int> lineIndices; // indices of lines/segments assigned to this drone
};

} // namespace AreaPlan


