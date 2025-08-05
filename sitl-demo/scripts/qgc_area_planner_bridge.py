#!/usr/bin/env python3
"""
QGroundControl AreaPlanner Bridge
Bridges QGroundControl's AreaPlanner output with ArduPilot SITL
"""

import os
import sys
import time
import json
import argparse
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Any, Optional

try:
    from pymavlink import mavutil
    from pymavlink import mavwp
except ImportError:
    print("❌ pymavlink not found. Install with: pip install pymavlink")
    sys.exit(1)

class QGCAreaPlannerBridge:
    def __init__(self, host: str = None, port: int = None):
        """Initialize QGC AreaPlanner Bridge."""
        # Use environment variables for Docker compatibility
        self.host = host or os.getenv('MAVLINK_HOST', '127.0.0.1')
        self.port = port or int(os.getenv('MAVLINK_PORT', 14550))
        self.connection_string = f"udp:{self.host}:{self.port}"
        
        self.mavlink_connection = None
        self.vehicle_type = os.getenv('VEHICLE_TYPE', 'copter')
        self.log_level = os.getenv('LOG_LEVEL', 'INFO')
        
        print(f"🌉 QGC AreaPlanner Bridge")
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

    def parse_qgc_mission_file(self, mission_file: Path) -> Dict[str, Any]:
        """Parse QGroundControl mission file (.plan format)."""
        print(f"📋 Parsing QGC mission file: {mission_file}")

        try:
            tree = ET.parse(mission_file)
            root = tree.getroot()

            mission_data = {
                "mission": {
                    "plannedHomePosition": [0, 0, 0],
                    "items": []
                }
            }

            # Parse home position
            home_position = root.find('.//homePosition')
            if home_position is not None:
                lat = float(home_position.get('lat', 0))
                lon = float(home_position.get('lon', 0))
                alt = float(home_position.get('alt', 0))
                mission_data["mission"]["plannedHomePosition"] = [lat, lon, alt]

            # Parse mission items
            mission_items = root.findall('.//missionItem')
            for item in mission_items:
                command = int(item.get('command', 0))
                frame = int(item.get('frame', 0))
                autoContinue = item.get('autoContinue', '1') == '1'

                # Parse parameters
                params = []
                for i in range(7):
                    param = float(item.get(f'param{i}', 0))
                    params.append(param)

                mission_item = {
                    "type": "SimpleItem",
                    "command": command,
                    "params": params
                }
                mission_data["mission"]["items"].append(mission_item)

            print(f"✅ Parsed {len(mission_data['mission']['items'])} mission items")
            return mission_data

        except Exception as e:
            print(f"❌ Failed to parse mission file: {e}")
            return None

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

    def demo_qgc_area_planner_workflow(self):
        """Demonstrate QGC AreaPlanner workflow."""
        print("\n🎬 QGC AreaPlanner Workflow Demo")
        print("=" * 50)
        
        if not self.connect():
            return False
        
        if not self.wait_for_gps():
            return False
        
        # Look for QGC mission files
        missions_dir = Path("/app/missions") if os.path.exists("/app/missions") else Path("missions")
        qgc_mission_files = list(missions_dir.glob("*.plan"))
        
        if not qgc_mission_files:
            print("📝 No QGC mission files found. Creating sample...")
            self.create_sample_qgc_mission()
            qgc_mission_files = list(missions_dir.glob("*.plan"))
        
        # Use the first available mission file
        mission_file = qgc_mission_files[0]
        print(f"📋 Using mission file: {mission_file}")
        
        # Parse and upload mission
        mission_data = self.parse_qgc_mission_file(mission_file)
        if mission_data and self.upload_mission(mission_data):
            self.arm_disarm(True)
            time.sleep(2)
            self.start_mission()
            self.monitor_mission_progress()
        
        print("✅ QGC AreaPlanner workflow demo completed!")
        return True

    def create_sample_qgc_mission(self):
        """Create a sample QGC mission file."""
        print("📝 Creating sample QGC mission file...")
        
        missions_dir = Path("/app/missions") if os.path.exists("/app/missions") else Path("missions")
        missions_dir.mkdir(exist_ok=True)
        
        sample_mission = f"""<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
    <Document>
        <name>Sample AreaPlanner Mission</name>
        <description>Generated by QGC AreaPlanner Bridge</description>
        <homePosition lat="47.397742" lon="8.545594" alt="488"/>
        <missionItem command="22" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.397742" param6="8.545594" param7="50"/>
        <missionItem command="16" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.398242" param6="8.545594" param7="50"/>
        <missionItem command="16" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.398242" param6="8.546094" param7="50"/>
        <missionItem command="16" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.397742" param6="8.546094" param7="50"/>
        <missionItem command="16" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.397242" param6="8.546094" param7="50"/>
        <missionItem command="16" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="47.397242" param6="8.545594" param7="50"/>
        <missionItem command="20" frame="0" autoContinue="1" param1="0" param2="0" param3="0" param4="0" param5="0" param6="0" param7="0"/>
    </Document>
</kml>"""
        
        mission_file = missions_dir / "sample_area_mission.plan"
        with open(mission_file, 'w') as f:
            f.write(sample_mission)
        
        print(f"✅ Sample QGC mission created: {mission_file}")

    def show_qgc_integration_guide(self):
        """Show comprehensive QGC integration guide."""
        print("\n📋 QGroundControl AreaPlanner Integration Guide")
        print("=" * 70)
        print("""
🎯 Complete Workflow:

1. Start SITL Environment:
   docker-compose up -d

2. Open QGroundControl:
   - Launch QGroundControl
   - Connect to SITL: UDP 127.0.0.1:14550

3. Create Area Plan:
   - Go to Plan View
   - Click Area Plan tab
   - Configure parameters:
     * Area Width: 100m
     * Area Height: 100m
     * Line Spacing: 10m
     * Mission Altitude: 50m
   - Position area on map
   - Click Generate Mission

4. Export Mission:
   - Review waypoints on map
   - File → Save Mission As...
   - Save as .plan file in missions/ directory

5. Upload to SITL:
   - Use this bridge script to parse and upload
   - Monitor execution in QGC Fly view

6. Monitor Execution:
   - Watch vehicle follow waypoints
   - Check progress in QGC
   - Vehicle returns to launch when complete

📁 File Locations:
   - QGC Mission Files: missions/*.plan
   - SITL Logs: logs/
   - Bridge Scripts: scripts/

🌐 Web Interfaces:
   - SITL Manager: http://localhost:8082
   - AreaPlanner Bridge: http://localhost:8085
   - Complete Demo: http://localhost:8086
        """)

def main():
    parser = argparse.ArgumentParser(description="QGC AreaPlanner Bridge")
    parser.add_argument("--host", default=None, help="MAVLink host")
    parser.add_argument("--port", type=int, default=None, help="MAVLink port")
    parser.add_argument("--demo", choices=["workflow", "guide"], help="Run demo")
    parser.add_argument("--create-sample", action="store_true", help="Create sample QGC mission")
    parser.add_argument("--show-guide", action="store_true", help="Show integration guide")
    
    args = parser.parse_args()
    
    bridge = QGCAreaPlannerBridge(args.host, args.port)
    
    try:
        if args.demo == "workflow":
            bridge.demo_qgc_area_planner_workflow()
        elif args.create_sample:
            bridge.create_sample_qgc_mission()
        elif args.show_guide or args.demo == "guide":
            bridge.show_qgc_integration_guide()
        else:
            bridge.show_qgc_integration_guide()
    finally:
        bridge.disconnect()

if __name__ == "__main__":
    main() 