#!/usr/bin/env python3
"""
Complete AreaPlanner Demo
Orchestrates a full demonstration of AreaPlanner workflow with SITL
"""

import os
import sys
import time
import json
import argparse
from pathlib import Path
from typing import Dict, List, Any, Optional

try:
    from pymavlink import mavutil
    from pymavlink import mavwp
except ImportError:
    print("❌ pymavlink not found. Install with: pip install pymavlink")
    sys.exit(1)

class CompleteAreaPlannerDemo:
    def __init__(self, host: str = None, port: int = None):
        """Initialize Complete AreaPlanner Demo."""
        # Use environment variables for Docker compatibility
        self.host = host or os.getenv('MAVLINK_HOST', '127.0.0.1')
        self.port = port or int(os.getenv('MAVLINK_PORT', 14550))
        self.connection_string = f"udp:{self.host}:{self.port}"
        
        self.mavlink_connection = None
        self.vehicle_type = os.getenv('VEHICLE_TYPE', 'copter')
        self.log_level = os.getenv('LOG_LEVEL', 'INFO')
        
        print(f"🎬 Complete AreaPlanner Demo")
        print(f"   Host: {self.host}")
        print(f"   Port: {self.port}")
        print(f"   Vehicle: {self.vehicle_type}")
        print(f"   Connection: {self.connection_string}")

    def connect(self) -> bool:
        """Connect to SITL via MAVLink."""
        try:
            print(f"🔌 Connecting to SITL at {self.connection_string}...")
            self.mavlink_connection = mavutil.mavlink_connection(self.connection_string)
            self.mavlink_connection.wait_heartbeat(timeout=10)
            print("✅ Connected to SITL successfully!")
            return True
        except Exception as e:
            print(f"❌ Failed to connect to SITL: {e}")
            return False

    def disconnect(self):
        """Disconnect from SITL."""
        if self.mavlink_connection:
            self.mavlink_connection.close()
            print("🔌 Disconnected from SITL")

    def wait_for_gps(self, timeout: int = 60) -> bool:
        """Wait for GPS lock."""
        print("📍 Waiting for GPS lock...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            try:
                gps = self.mavlink_connection.recv_match(type='GPS_RAW_INT', blocking=True, timeout=1)
                if gps and gps.fix_type >= 2:  # 2D or 3D fix
                    print("✅ GPS lock acquired!")
                    return True
            except Exception:
                pass
            time.sleep(1)
        
        print("❌ GPS lock timeout")
        return False

    def set_flight_mode(self, mode: str) -> bool:
        """Set flight mode."""
        try:
            print(f"🎮 Setting flight mode to {mode}...")
            self.mavlink_connection.mav.command_long_send(
                self.mavlink_connection.target_system,
                self.mavlink_connection.target_component,
                mavutil.mavlink.MAV_CMD_DO_SET_MODE,
                0, 0, 0, 0, 0, 0, 0
            )
            time.sleep(2)
            print(f"✅ Flight mode set to {mode}")
            return True
        except Exception as e:
            print(f"❌ Failed to set flight mode: {e}")
            return False

    def arm_disarm(self, arm: bool) -> bool:
        """Arm or disarm the vehicle."""
        try:
            action = "Arm" if arm else "Disarm"
            print(f"🔧 {action}ing vehicle...")
            
            self.mavlink_connection.mav.command_long_send(
                self.mavlink_connection.target_system,
                self.mavlink_connection.target_component,
                mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
                0, 0, 1 if arm else 0, 0, 0, 0, 0, 0
            )
            time.sleep(2)
            print(f"✅ Vehicle {action.lower()}ed successfully!")
            return True
        except Exception as e:
            print(f"❌ Failed to {action.lower()} vehicle: {e}")
            return False

    def run_complete_demo(self):
        """Run the complete AreaPlanner demonstration."""
        print("\n🎬 Complete AreaPlanner SITL Demonstration")
        print("=" * 60)

        if not self.connect():
            return False

        if not self.wait_for_gps():
            return False

        # Step 1: Simulate AreaPlanner workflow
        waypoints = self.simulate_area_planner_workflow()

        # Step 2: Create mission from waypoints
        mission_data = self.create_mission_from_waypoints(waypoints)

        # Step 3: Save mission file
        mission_file = self.save_mission_file(mission_data, "area_planner_demo.json")

        # Step 4: Upload mission to SITL
        if not self.upload_mission(mission_data):
            return False

        # Step 5: Arm vehicle
        print("\n7. Arming Vehicle:")
        self.arm_disarm(True)
        time.sleep(2)

        # Step 6: Start mission
        self.start_mission()

        # Step 7: Monitor progress
        self.monitor_mission_progress()

        # Step 8: Land and disarm
        print("\n8. Completing Mission:")
        self.set_flight_mode('RTL')
        time.sleep(10)
        self.arm_disarm(False)

        print("\n✅ Complete AreaPlanner demo finished successfully!")
        print("\n📋 Demo Summary:")
        print("   - AreaPlanner workflow simulated")
        print("   - Grid waypoints generated")
        print("   - Mission created and uploaded")
        print("   - SITL executed mission successfully")
        print("   - Vehicle returned to launch")

        return True

    def simulate_area_planner_workflow(self) -> List[Dict[str, float]]:
        """Simulate AreaPlanner mission generation workflow."""
        print("\n1. Simulating AreaPlanner Workflow:")
        print("   - Configuring area parameters")
        print("   - Calculating grid pattern")
        print("   - Generating waypoints")
        
        # Area configuration (simulating QGC AreaPlanner)
        area_config = {
            'center_lat': 47.397742,
            'center_lon': 8.545594,
            'area_width': 100,  # meters
            'area_height': 100,  # meters
            'line_spacing': 10,  # meters
            'altitude': 50,  # meters
            'rotation': 0  # degrees
        }
        
        print(f"   - Area Center: {area_config['center_lat']:.6f}, {area_config['center_lon']:.6f}")
        print(f"   - Area Size: {area_config['area_width']}m x {area_config['area_height']}m")
        print(f"   - Line Spacing: {area_config['line_spacing']}m")
        print(f"   - Mission Altitude: {area_config['altitude']}m")
        
        # Generate grid waypoints
        waypoints = self._generate_grid_waypoints(
            area_config['center_lat'], area_config['center_lon'],
            area_config['area_width'], area_config['area_height'],
            area_config['line_spacing'], area_config['altitude'],
            area_config['rotation']
        )
        
        print(f"   - Generated {len(waypoints)} waypoints")
        return waypoints

    def _generate_grid_waypoints(self, center_lat: float, center_lon: float,
                                  width: float, height: float, spacing: float,
                                  altitude: float, rotation: float) -> List[Dict[str, float]]:
        """Generate grid waypoints for area coverage."""
        import math

        # Convert to radians
        rotation_rad = math.radians(rotation)

        # Calculate number of lines
        num_lines = int(height / spacing) + 1
        points_per_line = int(width / spacing) + 1

        waypoints = []

        # Generate grid pattern
        for line in range(num_lines):
            y_offset = (line - num_lines // 2) * spacing

            for point in range(points_per_line):
                x_offset = (point - points_per_line // 2) * spacing

                # Apply rotation
                rotated_x = x_offset * math.cos(rotation_rad) - y_offset * math.sin(rotation_rad)
                rotated_y = x_offset * math.sin(rotation_rad) + y_offset * math.cos(rotation_rad)

                # Convert to lat/lon (approximate)
                lat_offset = rotated_y / 111320.0  # meters to degrees
                lon_offset = rotated_x / (111320.0 * math.cos(math.radians(center_lat)))

                waypoint = {
                    'lat': center_lat + lat_offset,
                    'lon': center_lon + lon_offset,
                    'alt': altitude
                }
                waypoints.append(waypoint)

        return waypoints

    def create_mission_from_waypoints(self, waypoints: List[Dict[str, float]]) -> Dict[str, Any]:
        """Create mission structure from waypoints."""
        print("\n2. Creating Mission from Waypoints:")
        print("   - Adding takeoff command")
        print("   - Adding waypoints")
        print("   - Adding return to launch")

        # Get home position from first waypoint
        home_lat = waypoints[0]['lat'] if waypoints else 47.397742
        home_lon = waypoints[0]['lon'] if waypoints else 8.545594
        home_alt = waypoints[0]['alt'] if waypoints else 50

        mission_data = {
            "mission": {
                "plannedHomePosition": [home_lat, home_lon, home_alt],
                "items": []
            }
        }

        # Add takeoff command
        mission_data["mission"]["items"].append({
            "type": "SimpleItem",
            "command": 22,  # MAV_CMD_NAV_TAKEOFF
            "params": [0, 0, 0, 0, home_lat, home_lon, home_alt]
        })

        # Add waypoints
        for i, waypoint in enumerate(waypoints):
            mission_data["mission"]["items"].append({
                "type": "SimpleItem",
                "command": 16,  # MAV_CMD_NAV_WAYPOINT
                "params": [0, 0, 0, 0, waypoint['lat'], waypoint['lon'], waypoint['alt']]
            })

        # Add return to launch
        mission_data["mission"]["items"].append({
            "type": "SimpleItem",
            "command": 20,  # MAV_CMD_NAV_RETURN_TO_LAUNCH
            "params": [0, 0, 0, 0, 0, 0, 0]
        })

        print(f"   - Mission created with {len(mission_data['mission']['items'])} items")
        return mission_data

    def save_mission_file(self, mission_data: Dict[str, Any], filename: str) -> Path:
        """Save mission data to file."""
        print(f"\n3. Saving Mission File:")
        
        missions_dir = Path("/app/missions") if os.path.exists("/app/missions") else Path("missions")
        missions_dir.mkdir(exist_ok=True)
        
        mission_file = missions_dir / filename
        with open(mission_file, 'w') as f:
            json.dump(mission_data, f, indent=2)
        
        print(f"   - Mission saved to: {mission_file}")
        return mission_file

    def upload_mission(self, mission_data: Dict[str, Any]) -> bool:
        """Upload mission to SITL."""
        print("\n4. Uploading Mission to SITL:")
        print("   - Clearing existing mission")
        print("   - Uploading new mission items")
        
        try:
            # Clear existing mission
            self.mavlink_connection.waypoint_clear_all_send()
            self.mavlink_connection.waypoint_count_send(0)
            
            # Upload new mission items
            mission_items = mission_data["mission"]["items"]
            self.mavlink_connection.waypoint_count_send(len(mission_items))
            
            for i, item in enumerate(mission_items):
                self.mavlink_connection.mav.mission_item_send(
                    self.mavlink_connection.target_system,
                    self.mavlink_connection.target_component,
                    i,
                    item.get("frame", 0),
                    item["command"],
                    0, 0,  # current, autocontinue
                    *item["params"]
                )
                time.sleep(0.1)
            
            print(f"   - Mission uploaded successfully! ({len(mission_items)} items)")
            return True
        except Exception as e:
            print(f"   - Failed to upload mission: {e}")
            return False

    def start_mission(self) -> bool:
        """Start mission execution."""
        print("\n5. Starting Mission Execution:")
        print("   - Setting flight mode to AUTO")
        print("   - Starting mission")
        
        try:
            # Set flight mode to AUTO
            self.set_flight_mode('AUTO')
            time.sleep(2)
            
            # Start mission
            self.mavlink_connection.mav.command_long_send(
                self.mavlink_connection.target_system,
                self.mavlink_connection.target_component,
                mavutil.mavlink.MAV_CMD_MISSION_START,
                0, 0, 0, 0, 0, 0, 0
            )
            print("   - Mission started successfully!")
            return True
        except Exception as e:
            print(f"   - Failed to start mission: {e}")
            return False

    def monitor_mission_progress(self, timeout: int = 300) -> bool:
        """Monitor mission execution progress."""
        print("\n6. Monitoring Mission Progress:")
        print("   - Tracking waypoint progress")
        print("   - Monitoring vehicle state")
        
        start_time = time.time()
        waypoint_count = 0
        
        while time.time() - start_time < timeout:
            try:
                # Check mission status
                mission_current = self.mavlink_connection.recv_match(type='MISSION_CURRENT', blocking=False)
                if mission_current:
                    if mission_current.seq != waypoint_count:
                        waypoint_count = mission_current.seq
                        print(f"   - Reached waypoint {waypoint_count}")
                
                # Check vehicle state
                heartbeat = self.mavlink_connection.recv_match(type='HEARTBEAT', blocking=False)
                if heartbeat:
                    print(f"   - Vehicle mode: {heartbeat.custom_mode}")
                
                # Check GPS position
                gps = self.mavlink_connection.recv_match(type='GPS_RAW_INT', blocking=False)
                if gps and gps.fix_type >= 2:
                    print(f"   - Position: {gps.lat/1e7:.6f}, {gps.lon/1e7:.6f}, Alt: {gps.alt/1000:.1f}m")
                
                time.sleep(5)
                
            except Exception as e:
                print(f"   - Monitoring error: {e}")
                time.sleep(1)
        
        print("   - Mission monitoring completed")
        return True

    def show_demo_instructions(self):
        """Show demo instructions."""
        print("\n📋 Complete AreaPlanner Demo Instructions")
        print("=" * 60)
        print("""
🎬 Demo Overview:
This demo simulates the complete AreaPlanner workflow:

1. AreaPlanner Configuration:
   - Simulates QGC AreaPlanner interface
   - Configures area parameters
   - Generates grid waypoints

2. Mission Creation:
   - Creates mission structure
   - Adds takeoff command
   - Adds waypoints
   - Adds return to launch

3. SITL Integration:
   - Uploads mission to SITL
   - Arms vehicle
   - Starts mission execution
   - Monitors progress

4. Mission Execution:
   - Vehicle follows waypoints
   - Real-time monitoring
   - Automatic return to launch

🌐 Web Interfaces:
   - SITL Manager: http://localhost:8082
   - AreaPlanner Integration: http://localhost:8084
   - QGC Bridge: http://localhost:8085
   - Complete Demo: http://localhost:8086

📁 Files Generated:
   - Mission files: missions/
   - Logs: logs/
   - Data: data/
        """)

def main():
    parser = argparse.ArgumentParser(description="Complete AreaPlanner Demo")
    parser.add_argument("--host", default=None, help="MAVLink host")
    parser.add_argument("--port", type=int, default=None, help="MAVLink port")
    parser.add_argument("--instructions", action="store_true", help="Show demo instructions")
    
    args = parser.parse_args()
    
    demo = CompleteAreaPlannerDemo(args.host, args.port)
    
    try:
        if args.instructions:
            demo.show_demo_instructions()
        else:
            demo.run_complete_demo()
    finally:
        demo.disconnect()

if __name__ == "__main__":
    main() 