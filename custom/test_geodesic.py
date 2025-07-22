#!/usr/bin/env python3
"""
Test script to validate geodesic calculations in MissionAreaPlanner C++ implementation
against the original Python/geopy implementation from mission_gui.py
"""

import sys
import math
from geopy.distance import geodesic

# Constants from MissionAreaPlanner.cc
EARTH_RADIUS = 6371000.0  # meters
PI = 3.14159265358979323846


def haversine_distance(lat1, lon1, lat2, lon2):
    """Haversine formula implementation matching C++ code"""
    lat1_rad = lat1 * PI / 180.0
    lon1_rad = lon1 * PI / 180.0
    lat2_rad = lat2 * PI / 180.0
    lon2_rad = lon2 * PI / 180.0

    dLat = lat2_rad - lat1_rad
    dLon = lon2_rad - lon1_rad

    a = math.sin(dLat / 2) * math.sin(dLat / 2) + math.cos(lat1_rad) * math.cos(
        lat2_rad
    ) * math.sin(dLon / 2) * math.sin(dLon / 2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    return EARTH_RADIUS * c


def coordinate_at_distance(lat, lon, distance, bearing):
    """Coordinate at distance implementation matching C++ code"""
    lat1_rad = lat * PI / 180.0
    lon1_rad = lon * PI / 180.0
    brng_rad = bearing * PI / 180.0

    angular_distance = distance / EARTH_RADIUS

    lat2_rad = math.asin(
        math.sin(lat1_rad) * math.cos(angular_distance)
        + math.cos(lat1_rad) * math.sin(angular_distance) * math.cos(brng_rad)
    )

    lon2_rad = lon1_rad + math.atan2(
        math.sin(brng_rad) * math.sin(angular_distance) * math.cos(lat1_rad),
        math.cos(angular_distance) - math.sin(lat1_rad) * math.sin(lat2_rad),
    )

    return (lat2_rad * 180.0 / PI, lon2_rad * 180.0 / PI)


def calculate_bearing(lat1, lon1, lat2, lon2):
    """Bearing calculation implementation matching C++ code"""
    lat1_rad = lat1 * PI / 180.0
    lon1_rad = lon1 * PI / 180.0
    lat2_rad = lat2 * PI / 180.0
    lon2_rad = lon2 * PI / 180.0

    dLon = lon2_rad - lon1_rad

    y = math.sin(dLon) * math.cos(lat2_rad)
    x = math.cos(lat1_rad) * math.sin(lat2_rad) - math.sin(lat1_rad) * math.cos(
        lat2_rad
    ) * math.cos(dLon)

    bearing = math.atan2(y, x) * 180.0 / PI
    return (bearing + 360) % 360


def test_distance_calculations():
    """Test distance calculations between implementations"""
    print("Testing distance calculations...")

    # Test coordinates (Lviv, Ukraine area)
    lat1, lon1 = 49.82824897481479, 24.033390804256005
    lat2, lon2 = 49.829, 24.034

    # Haversine implementation
    haversine_dist = haversine_distance(lat1, lon1, lat2, lon2)

    # Geopy implementation
    geopy_dist = geodesic((lat1, lon1), (lat2, lon2)).meters

    print(f"Coordinates: ({lat1}, {lon1}) to ({lat2}, {lon2})")
    print(f"Haversine distance: {haversine_dist:.2f} meters")
    print(f"Geopy distance: {geopy_dist:.2f} meters")
    print(f"Difference: {abs(haversine_dist - geopy_dist):.2f} meters")
    print(
        f"Percentage difference: {abs(haversine_dist - geopy_dist) / geopy_dist * 100:.4f}%"
    )
    print()


def test_coordinate_at_distance():
    """Test coordinate at distance calculations"""
    print("Testing coordinate at distance calculations...")

    # Test center point
    center_lat, center_lon = 49.82824897481479, 24.033390804256005

    # Test distances and bearings
    test_cases = [
        (100, 0),  # 100m North
        (100, 90),  # 100m East
        (100, 180),  # 100m South
        (100, 270),  # 100m West
        (50, 45),  # 50m Northeast
    ]

    for distance, bearing in test_cases:
        # Haversine implementation
        new_lat, new_lon = coordinate_at_distance(
            center_lat, center_lon, distance, bearing
        )

        # Geopy implementation
        geopy_point = geodesic(meters=distance).destination(
            (center_lat, center_lon), bearing
        )

        print(f"Distance: {distance}m, Bearing: {bearing}°")
        print(f"Haversine result: ({new_lat:.8f}, {new_lon:.8f})")
        print(
            f"Geopy result: ({geopy_point.latitude:.8f}, {geopy_point.longitude:.8f})"
        )

        # Calculate distance back to verify
        haversine_back = haversine_distance(center_lat, center_lon, new_lat, new_lon)
        geopy_back = geodesic(
            (center_lat, center_lon), (geopy_point.latitude, geopy_point.longitude)
        ).meters

        print(f"Distance back (Haversine): {haversine_back:.2f}m")
        print(f"Distance back (Geopy): {geopy_back:.2f}m")
        print()


def test_area_corners():
    """Test area corner calculations"""
    print("Testing area corner calculations...")

    # Test parameters from mission_gui.py
    center_lat, center_lon = 49.82824897481479, 24.033390804256005
    width, height = 30.0, 90.0  # meters

    print(f"Area center: ({center_lat}, {center_lon})")
    print(f"Area dimensions: {width}m x {height}m")
    print()

    # Calculate corners using Haversine implementation
    half_width = width / 2.0
    half_height = height / 2.0

    # North and South points
    north_lat, north_lon = coordinate_at_distance(
        center_lat, center_lon, half_height, 0
    )
    south_lat, south_lon = coordinate_at_distance(
        center_lat, center_lon, half_height, 180
    )

    # Corner points
    nw_lat, nw_lon = coordinate_at_distance(north_lat, north_lon, half_width, 270)
    ne_lat, ne_lon = coordinate_at_distance(north_lat, north_lon, half_width, 90)
    se_lat, se_lon = coordinate_at_distance(south_lat, south_lon, half_width, 90)
    sw_lat, sw_lon = coordinate_at_distance(south_lat, south_lon, half_width, 270)

    corners = [
        ("NW", nw_lat, nw_lon),
        ("NE", ne_lat, ne_lon),
        ("SE", se_lat, se_lon),
        ("SW", sw_lat, sw_lon),
    ]

    print("Area corners (Haversine implementation):")
    for name, lat, lon in corners:
        print(f"  {name}: ({lat:.8f}, {lon:.8f})")
    print()

    # Compare with geopy implementation
    print("Area corners (Geopy implementation):")
    north = geodesic(meters=half_height).destination((center_lat, center_lon), 0)
    south = geodesic(meters=half_height).destination((center_lat, center_lon), 180)

    nw_geopy = geodesic(meters=half_width).destination(
        (north.latitude, north.longitude), 270
    )
    ne_geopy = geodesic(meters=half_width).destination(
        (north.latitude, north.longitude), 90
    )
    se_geopy = geodesic(meters=half_width).destination(
        (south.latitude, south.longitude), 90
    )
    sw_geopy = geodesic(meters=half_width).destination(
        (south.latitude, south.longitude), 270
    )

    geopy_corners = [
        ("NW", nw_geopy.latitude, nw_geopy.longitude),
        ("NE", ne_geopy.latitude, ne_geopy.longitude),
        ("SE", se_geopy.latitude, se_geopy.longitude),
        ("SW", sw_geopy.latitude, sw_geopy.longitude),
    ]

    for name, lat, lon in geopy_corners:
        print(f"  {name}: ({lat:.8f}, {lon:.8f})")
    print()


def test_grid_generation():
    """Test grid generation logic"""
    print("Testing grid generation logic...")

    # Test parameters
    center_lat, center_lon = 49.82824897481479, 24.033390804256005
    width, height = 30.0, 90.0
    line_spacing = 3.0
    num_points = 3

    print(f"Grid parameters:")
    print(f"  Center: ({center_lat}, {center_lon})")
    print(f"  Dimensions: {width}m x {height}m")
    print(f"  Line spacing: {line_spacing}m")
    print(f"  Points per line: {num_points}")
    print()

    # Calculate area corners first
    half_width = width / 2.0
    half_height = height / 2.0

    north_lat, north_lon = coordinate_at_distance(
        center_lat, center_lon, half_height, 0
    )
    south_lat, south_lon = coordinate_at_distance(
        center_lat, center_lon, half_height, 180
    )

    nw_lat, nw_lon = coordinate_at_distance(north_lat, north_lon, half_width, 270)
    ne_lat, ne_lon = coordinate_at_distance(north_lat, north_lon, half_width, 90)
    se_lat, se_lon = coordinate_at_distance(south_lat, south_lon, half_width, 90)
    sw_lat, sw_lon = coordinate_at_distance(south_lat, south_lon, half_width, 270)

    # Generate grid lines
    n_lines = max(1, int(height // line_spacing))
    waypoints = []

    print(f"Generating {n_lines} grid lines...")

    for i in range(n_lines):
        offset = -(height / 2) + (i + 0.5) * line_spacing
        line_center_lat, line_center_lon = coordinate_at_distance(
            center_lat, center_lon, abs(offset), 180 if offset < 0 else 0
        )

        print(f"  Line {i+1}: center at ({line_center_lat:.8f}, {line_center_lon:.8f})")

        for j in range(num_points):
            frac_p = (j + 0.5) / num_points
            pt_offset = (frac_p - 0.5) * width
            pt_lat, pt_lon = coordinate_at_distance(
                line_center_lat,
                line_center_lon,
                abs(pt_offset),
                90 if pt_offset > 0 else 270,
            )
            waypoints.append((pt_lat, pt_lon))
            print(f"    Point {j+1}: ({pt_lat:.8f}, {pt_lon:.8f})")

    print(f"\nTotal waypoints generated: {len(waypoints)}")
    print()


def main():
    """Run all tests"""
    print("Mission Area Planner - Geodesic Calculation Tests")
    print("=" * 50)
    print()

    test_distance_calculations()
    test_coordinate_at_distance()
    test_area_corners()
    test_grid_generation()

    print("All tests completed!")


if __name__ == "__main__":
    main()
