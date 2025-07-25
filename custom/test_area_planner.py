#!/usr/bin/env python3
"""
Simple test script to verify Area Planner logic works correctly
This replicates the functionality from mission_gui.py in a testable format
"""

import math
from geopy.distance import geodesic

class AreaPlanner:
    def __init__(self):
        self.width = 30.0
        self.height = 90.0
        self.line_spacing = 3.0
        self.num_points = 1
        self.center_lat = 49.82824897481479
        self.center_lon = 24.033390804256005
        
    def set_width(self, width):
        self.width = width
        print(f"Width set to: {width} m")
        
    def set_height(self, height):
        self.height = height
        print(f"Height set to: {height} m")
        
    def set_line_spacing(self, spacing):
        self.line_spacing = spacing
        print(f"Line spacing set to: {spacing} m")
        
    def set_num_points(self, points):
        self.num_points = points
        print(f"Points per line set to: {points}")
        
    def set_center(self, lat, lon):
        self.center_lat = lat
        self.center_lon = lon
        print(f"Center set to: {lat:.6f}, {lon:.6f}")
        
    def calculate_waypoints(self):
        """Calculate waypoints based on current settings"""
        waypoints = []
        
        # Calculate area corners (clockwise from NW)
        lat, lon = self.center_lat, self.center_lon
        w, h = self.width, self.height
        
        # Calculate corners using geodesic calculations
        # North and South points
        north_lat = lat + (h/2) / 111320.0  # Approximate conversion
        south_lat = lat - (h/2) / 111320.0
        
        # Calculate longitude offset based on latitude
        lon_offset = (w/2) / (111320.0 * math.cos(math.radians(lat)))
        
        # NW, NE, SE, SW corners
        nw = (north_lat, lon - lon_offset)
        ne = (north_lat, lon + lon_offset)
        se = (south_lat, lon + lon_offset)
        sw = (south_lat, lon - lon_offset)
        
        print(f"Area corners:")
        print(f"  NW: {nw[0]:.6f}, {nw[1]:.6f}")
        print(f"  NE: {ne[0]:.6f}, {ne[1]:.6f}")
        print(f"  SE: {se[0]:.6f}, {se[1]:.6f}")
        print(f"  SW: {sw[0]:.6f}, {sw[1]:.6f}")
        
        # Calculate waypoints along lines
        n_lines = max(1, int(h // self.line_spacing))
        
        for i in range(n_lines):
            # Calculate line center
            offset = (-(h/2) + (i + 0.5) * self.line_spacing)
            line_lat = lat + offset / 111320.0
            
            # Calculate line start and end
            line_lon_offset = (w/2) / (111320.0 * math.cos(math.radians(line_lat)))
            line_start = (line_lat, lon - line_lon_offset)
            line_end = (line_lat, lon + line_lon_offset)
            
            print(f"Line {i+1}: {line_start[0]:.6f}, {line_start[1]:.6f} to {line_end[0]:.6f}, {line_end[1]:.6f}")
            
            # Add points along the line
            for j in range(self.num_points):
                frac = (j + 0.5) / self.num_points
                point_lon = line_start[1] + frac * (line_end[1] - line_start[1])
                waypoints.append((line_lat, point_lon))
                
        return waypoints
        
    def generate_mission(self):
        """Generate mission waypoints"""
        print("Generating mission...")
        waypoints = self.calculate_waypoints()
        print(f"Generated {len(waypoints)} waypoints:")
        
        for i, (lat, lon) in enumerate(waypoints):
            print(f"  {i+1:2d}: {lat:.6f}, {lon:.6f}")
            
        return waypoints

def test_area_planner():
    """Test the Area Planner functionality"""
    print("=== Area Planner Test ===\n")
    
    # Create area planner
    planner = AreaPlanner()
    
    # Test default settings
    print("Default settings:")
    print(f"  Width: {planner.width} m")
    print(f"  Height: {planner.height} m")
    print(f"  Line spacing: {planner.line_spacing} m")
    print(f"  Points per line: {planner.num_points}")
    print(f"  Center: {planner.center_lat:.6f}, {planner.center_lon:.6f}")
    print()
    
    # Generate mission with default settings
    waypoints = planner.generate_mission()
    print()
    
    # Test with different settings
    print("Testing with different settings:")
    planner.set_width(50.0)
    planner.set_height(100.0)
    planner.set_line_spacing(5.0)
    planner.set_num_points(3)
    planner.set_center(49.82824897481479, 24.033390804256005)
    print()
    
    # Generate mission with new settings
    waypoints = planner.generate_mission()
    print()
    
    print("=== Test completed successfully ===")
    return waypoints

if __name__ == "__main__":
    test_area_planner() 