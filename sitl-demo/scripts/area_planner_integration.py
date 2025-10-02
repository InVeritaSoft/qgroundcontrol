#!/usr/bin/env python3
"""
AreaPlanner Integration with ArduPilot SITL
Demonstrates how to generate and execute AreaPlanner-like missions on SITL
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

class AreaPlannerSITLIntegration:
    def __init__(self, host: str = None, port: int = None):
        """Initialize AreaPlanner SITL integration."""
        # Use environment variables for Docker compatibility
        self.host = host or os.getenv('MAVLINK_HOST', '127.0.0.1')
        self.port = port or int(os.getenv('MAVLINK_PORT', 14550))
        self.connection_string = f"udp:{self.host}:{self.port}"
        
        self.mavlink_connection = None
        self.vehicle_type = os.getenv('VEHICLE_TYPE', 'copter')
        self.log_level = os.getenv('LOG_LEVEL', 'INFO')
        
        print(f"🚁 AreaPlanner SITL Integration")
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

    def generate_area_mission(self, area_config: Dict[str, Any]) -> Dict[str, Any]:
        """Generate a mission from AreaPlanner configuration."""
        print("📋 Generating area mission from configuration...")

        # Extract parameters from area config
        center_lat = area_config.get('center_lat', 47.397742)
        center_lon = area_config.get('center_lon', 8.545594)
        area_width = area_config.get('area_width', 100)  # meters
        area_height = area_config.get('area_height', 100)  # meters
        line_spacing = area_config.get('line_spacing', 10)  # meters
        altitude = area_config.get('altitude', 50)  # meters
        rotation = area_config.get('rotation', 0)  # degrees

        # Calculate grid waypoints
        waypoints = self._calculate_grid_waypoints(
            center_lat, center_lon, area_width, area_height,
            line_spacing, altitude, rotation
        )

        # Create mission structure
        mission_data = {
            "mission": {
                "plannedHomePosition": [center_lat, center_lon, altitude],
                "items": []
            }
        }

        # Add takeoff command
        mission_data["mission"]["items"].append({
            "type": "SimpleItem",
            "command": 22,  # MAV_CMD_NAV_TAKEOFF
            "params": [0, 0, 0, 0, center_lat, center_lon, altitude]
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

        return mission_data

    def _calculate_grid_waypoints(self, center_lat: float, center_lon: float,
                                  width: float, height: float, spacing: float,
                                  altitude: float, rotation: float) -> List[Dict[str, float]]:
        """Calculate grid waypoints for area coverage."""
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

    def save_mission_file(self, mission_data: Dict[str, Any], filename: str) -> Path:
        """Save mission data to file."""
        missions_dir = Path("/app/missions") if os.path.exists("/app/missions") else Path("missions")
        missions_dir.mkdir(exist_ok=True)
        
        mission_file = missions_dir / filename
        with open(mission_file, 'w') as f:
            json.dump(mission_data, f, indent=2)
        
        print(f"💾 Mission saved to: {mission_file}")
        return mission_file

    def upload_mission(self, mission_data: Dict[str, Any]) -> bool:
        """Upload mission to SITL."""
        try:
            print("📤 Uploading mission to SITL...")
            
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
            
            print(f"✅ Mission uploaded successfully! ({len(mission_items)} items)")
            return True
        except Exception as e:
            print(f"❌ Failed to upload mission: {e}")
            return False

    def start_mission(self) -> bool:
        """Start mission execution."""
        try:
            print("🚀 Starting mission...")
            self.mavlink_connection.mav.command_long_send(
                self.mavlink_connection.target_system,
                self.mavlink_connection.target_component,
                mavutil.mavlink.MAV_CMD_MISSION_START,
                0, 0, 0, 0, 0, 0, 0
            )
            print("✅ Mission started!")
            return True
        except Exception as e:
            print(f"❌ Failed to start mission: {e}")
            return False

    def monitor_mission_progress(self, timeout: int = 300) -> bool:
        """Monitor mission execution progress."""
        print("📊 Monitoring mission progress...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            try:
                # Check mission status
                mission_current = self.mavlink_connection.recv_match(type='MISSION_CURRENT', blocking=False)
                if mission_current:
                    print(f"📍 Current waypoint: {mission_current.seq}")
                
                # Check vehicle state
                heartbeat = self.mavlink_connection.recv_match(type='HEARTBEAT', blocking=False)
                if heartbeat:
                    print(f"🛩️  Vehicle mode: {heartbeat.custom_mode}")
                
                time.sleep(5)
                
            except Exception as e:
                print(f"⚠️  Monitoring error: {e}")
                time.sleep(1)
        
        print("⏰ Mission monitoring timeout")
        return False

    def demo_area_planner_workflow(self):
        """Demonstrate AreaPlanner workflow."""
        print("\n🎬 AreaPlanner Workflow Demo")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        if not self.wait_for_gps():
            return False
        
        # Generate area mission
        area_config = {
            'center_lat': 47.397742,
            'center_lon': 8.545594,
            'area_width': 100,
            'area_height': 100,
            'line_spacing': 10,
            'altitude': 50,
            'rotation': 0
        }
        
        mission_data = self.generate_area_mission(area_config)
        
        # Save mission
        self.save_mission_file(mission_data, "area_planner_demo.json")
        
        # Upload and execute
        if self.upload_mission(mission_data):
            self.arm_disarm(True)
            time.sleep(2)
            self.start_mission()
            self.monitor_mission_progress()
        
        print("✅ AreaPlanner workflow demo completed!")
        return True

    def demo_interactive_planning(self):
        """Interactive AreaPlanner demo."""
        print("\n🎯 Interactive AreaPlanner Demo")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        # Interactive area configuration
        print("Configure your area:")
        center_lat = float(input("Center Latitude (default 47.397742): ") or 47.397742)
        center_lon = float(input("Center Longitude (default 8.545594): ") or 8.545594)
        area_width = float(input("Area Width (meters, default 100): ") or 100)
        area_height = float(input("Area Height (meters, default 100): ") or 100)
        line_spacing = float(input("Line Spacing (meters, default 10): ") or 10)
        altitude = float(input("Mission Altitude (meters, default 50): ") or 50)
        rotation = float(input("Rotation (degrees, default 0): ") or 0)
        
        area_config = {
            'center_lat': center_lat,
            'center_lon': center_lon,
            'area_width': area_width,
            'area_height': area_height,
            'line_spacing': line_spacing,
            'altitude': altitude,
            'rotation': rotation
        }
        
        mission_data = self.generate_area_mission(area_config)
        self.save_mission_file(mission_data, "interactive_area_mission.json")
        
        if self.upload_mission(mission_data):
            self.arm_disarm(True)
            time.sleep(2)
            self.start_mission()
            self.monitor_mission_progress()
        
        print("✅ Interactive AreaPlanner demo completed!")
        return True

    def create_sample_missions(self):
        """Create sample missions for testing."""
        print("\n📝 Creating sample missions...")
        
        missions = [
            {
                'name': 'small_area.json',
                'config': {
                    'center_lat': 47.397742,
                    'center_lon': 8.545594,
                    'area_width': 50,
                    'area_height': 50,
                    'line_spacing': 5,
                    'altitude': 30,
                    'rotation': 0
                }
            },
            {
                'name': 'large_area.json',
                'config': {
                    'center_lat': 47.397742,
                    'center_lon': 8.545594,
                    'area_width': 200,
                    'area_height': 200,
                    'line_spacing': 20,
                    'altitude': 80,
                    'rotation': 45
                }
            },
            {
                'name': 'rotated_area.json',
                'config': {
                    'center_lat': 47.397742,
                    'center_lon': 8.545594,
                    'area_width': 150,
                    'area_height': 100,
                    'line_spacing': 15,
                    'altitude': 60,
                    'rotation': 30
                }
            }
        ]
        
        for mission in missions:
            mission_data = self.generate_area_mission(mission['config'])
            self.save_mission_file(mission_data, mission['name'])
        
        print("✅ Sample missions created!")

    def show_qgc_integration_instructions(self):
        """Show QGroundControl integration instructions."""
        print("\n📋 QGroundControl AreaPlanner Integration Guide")
        print("=" * 60)
        print("""
1. Start SITL Environment:
   docker-compose up -d

2. Open QGroundControl and Connect:
   - Open QGroundControl
   - Connect to SITL using UDP: 127.0.0.1:14550

3. Use AreaPlanner:
   - Go to Plan View
   - Click on Area Plan tab
   - Configure area parameters:
     * Area Width: 100m
     * Area Height: 100m
     * Line Spacing: 10m
     * Mission Altitude: 30m
   - Position area on map
   - Click Generate Mission

4. Upload and Execute:
   - Review waypoints on map
   - Click Upload to Vehicle
   - Switch to Fly view
   - Click Start Mission

5. Monitor Execution:
   - Watch vehicle follow waypoints
   - Monitor progress in Fly view
   - Vehicle will return to launch when complete
        """)

def main():
    parser = argparse.ArgumentParser(description="AreaPlanner SITL Integration")
    parser.add_argument("--host", default=None, help="MAVLink host")
    parser.add_argument("--port", type=int, default=None, help="MAVLink port")
    parser.add_argument("--demo", choices=["workflow", "interactive"], help="Run demo")
    parser.add_argument("--create-samples", action="store_true", help="Create sample missions")
    parser.add_argument("--show-guide", action="store_true", help="Show QGC integration guide")
    
    args = parser.parse_args()
    
    integration = AreaPlannerSITLIntegration(args.host, args.port)
    
    try:
        if args.demo == "workflow":
            integration.demo_area_planner_workflow()
        elif args.demo == "interactive":
            integration.demo_interactive_planning()
        elif args.create_samples:
            integration.create_sample_missions()
        elif args.show_guide:
            integration.show_qgc_integration_instructions()
        else:
            integration.show_qgc_integration_instructions()
    finally:
        integration.disconnect()

if __name__ == "__main__":
    main() 