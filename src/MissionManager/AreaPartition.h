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
#include <cmath>

namespace AreaPlan {

// Minimal geometry to keep header portable
struct Point { double x {0.0}; double y {0.0}; };
struct Line  { Point a; Point b; };

// Split rectangle centered at (cx,cy) of size (width x height) into N stripes
inline std::vector<Line> splitIntoStripes(double cx, double cy,
                                          double width, double height,
                                          int stripeCount,
                                          bool alongShortAxis,
                                          double rotationDeg)
{
    std::vector<Line> stripes;
    if (stripeCount <= 0 || width <= 0.0 || height <= 0.0) return stripes;

    const double hw = width  * 0.5;
    const double hh = height * 0.5;

    const bool splitAlongX = alongShortAxis ? (width <= height) : (width > height);

    const double radians = rotationDeg * 3.14159265358979323846 / 180.0;
    const double cosA = std::cos(radians);
    const double sinA = std::sin(radians);

    auto rotate = [&](const Point& p) -> Point {
        const double dx = p.x - cx;
        const double dy = p.y - cy;
        const double rx =  dx * cosA - dy * sinA;
        const double ry =  dx * sinA + dy * cosA;
        return Point{cx + rx, cy + ry};
    };

    if (splitAlongX) {
        const double step = (2.0 * hw) / stripeCount;
        for (int i = 0; i < stripeCount; ++i) {
            const double x = -hw + (i + 0.5) * step;
            Point p1{cx + x, cy - hh};
            Point p2{cx + x, cy + hh};
            stripes.push_back(Line{rotate(p1), rotate(p2)});
        }
    } else {
        const double step = (2.0 * hh) / stripeCount;
        for (int i = 0; i < stripeCount; ++i) {
            const double y = -hh + (i + 0.5) * step;
            Point p1{cx - hw, cy + y};
            Point p2{cx + hw, cy + y};
            stripes.push_back(Line{rotate(p1), rotate(p2)});
        }
    }

    return stripes;
}

// Round-robin assignment of indices [0..lineCount)
// Distributes indices [0..lineCount) in round-robin order across drones.
// Properties:
// - If lineCount % droneCount = r, then the first r drones receive ⌈lineCount/droneCount⌉
//   indices and the others receive ⌊lineCount/droneCount⌋. The max difference between
//   any two drone counts is at most 1.
inline std::vector<std::vector<int>> assignStripesRoundRobin(int droneCount, int lineCount)
{
    std::vector<std::vector<int>> assignment;
    if (droneCount <= 0 || lineCount <= 0) return assignment;
    assignment.assign(static_cast<size_t>(droneCount), {});
    for (int i = 0; i < lineCount; ++i) {
        assignment[static_cast<size_t>(i % droneCount)].push_back(i);
    }
    return assignment;
}

} // namespace AreaPlan

