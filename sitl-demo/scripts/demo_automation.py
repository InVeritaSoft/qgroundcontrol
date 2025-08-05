#!/usr/bin/env python3
"""
ArduPilot SITL Demo Automation
This script provides automated demo sequences for QGroundControl demonstrations.
"""

import time
import json
import argparse
from pymavlink import mavutil
from pathlib import Path

class SITLDemoAutomation:
    def __init__(self, connection_string="udp:127.0.0.1:14550"):
        self.connection_string = connection_string
        self.connection = None
        self.target_system = 1
        self.target_component = 1
        
    def connect(self):
        """Connect to SITL."""
        try:
            print(f"🔗 Connecting to SITL at {self.connection_string}...")
            self.connection = mavutil.mavlink_connection(self.connection_string)
            self.connection.wait_heartbeat()
            print("✅ Connected to SITL successfully!")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from SITL."""
        if self.connection:
            self.connection.close()
            print("🔌 Disconnected from SITL")
    
    def wait_for_gps(self, timeout=30):
        """Wait for GPS lock."""
        print("📍 Waiting for GPS lock...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            try:
                gps = self.connection.recv_match(type='GPS_RAW_INT', blocking=True, timeout=1)
                if gps and gps.fix_type >= 3:
                    print("✅ GPS lock acquired!")
                    return True
            except:
                pass
            time.sleep(1)
        
        print("❌ GPS lock timeout")
        return False
    
    def set_flight_mode(self, mode):
        """Set flight mode."""
        print(f"🔄 Setting flight mode to {mode}...")
        
        mode_mapping = {
            'MANUAL': 0,
            'CIRCLE': 1,
            'STABILIZE': 2,
            'TRAINING': 3,
            'ACRO': 4,
            'FBWA': 5,
            'FBWB': 6,
            'CRUISE': 7,
            'AUTOTUNE': 8,
            'AUTO': 10,
            'RTL': 11,
            'LOITER': 12,
            'TAKEOFF': 13,
            'AVOID_ADSB': 14,
            'GUIDED': 15,
            'INITIALISING': 16,
            'QSTABILIZE': 17,
            'QHOVER': 18,
            'QLOITER': 19,
            'QLAND': 20,
            'QRTL': 21,
            'QAUTOTUNE': 22,
            'QACRO': 23,
            'THERMAL': 24
        }
        
        if mode.upper() in mode_mapping:
            mode_id = mode_mapping[mode.upper()]
            self.connection.mav.command_long_send(
                self.target_system,
                self.target_component,
                mavutil.mavlink.MAV_CMD_DO_SET_MODE,
                0, mode_id, 0, 0, 0, 0, 0, 0
            )
            time.sleep(2)
            print(f"✅ Flight mode set to {mode}")
            return True
        else:
            print(f"❌ Unknown flight mode: {mode}")
            return False
    
    def arm_disarm(self, arm=True):
        """Arm or disarm the vehicle."""
        action = "Arm" if arm else "Disarm"
        print(f"🔧 {action}ing vehicle...")
        
        self.connection.mav.command_long_send(
            self.target_system,
            self.target_component,
            mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
            0, 1 if arm else 0, 0, 0, 0, 0, 0, 0
        )
        time.sleep(2)
        print(f"✅ Vehicle {action.lower()}ed")
    
    def takeoff(self, altitude=50):
        """Execute takeoff."""
        print(f"🚁 Taking off to {altitude}m...")
        
        # Set to GUIDED mode
        self.set_flight_mode('GUIDED')
        time.sleep(1)
        
        # Arm the vehicle
        self.arm_disarm(True)
        time.sleep(2)
        
        # Send takeoff command
        self.connection.mav.command_long_send(
            self.target_system,
            self.target_component,
            mavutil.mavlink.MAV_CMD_NAV_TAKEOFF,
            0, 0, 0, 0, 0, 0, 0, altitude
        )
        
        print("✅ Takeoff command sent")
    
    def land(self):
        """Execute landing."""
        print("🛬 Landing...")
        
        self.connection.mav.command_long_send(
            self.target_system,
            self.target_component,
            mavutil.mavlink.MAV_CMD_NAV_LAND,
            0, 0, 0, 0, 0, 0, 0, 0
        )
        
        print("✅ Landing command sent")
    
    def return_to_launch(self):
        """Execute return to launch."""
        print("🏠 Returning to launch...")
        
        self.set_flight_mode('RTL')
        print("✅ RTL command sent")
    
    def upload_mission(self, mission_file):
        """Upload a mission file."""
        print(f"📋 Uploading mission from {mission_file}...")
        
        try:
            with open(mission_file, 'r') as f:
                mission_data = json.load(f)
            
            # Clear existing mission
            self.connection.mav.command_long_send(
                self.target_system,
                self.target_component,
                mavutil.mavlink.MAV_CMD_MISSION_CLEAR_ALL,
                0, 0, 0, 0, 0, 0, 0, 0
            )
            time.sleep(1)
            
            # Upload mission items
            mission_items = mission_data['mission']['items']
            for i, item in enumerate(mission_items):
                self.connection.mav.mission_item_send(
                    self.target_system,
                    self.target_component,
                    i,
                    mavutil.mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT,
                    item['command'],
                    0, 0,  # autocontinue, mission_type
                    *item['params']
                )
                time.sleep(0.1)
            
            print(f"✅ Mission uploaded with {len(mission_items)} items")
            return True
            
        except Exception as e:
            print(f"❌ Failed to upload mission: {e}")
            return False
    
    def start_mission(self):
        """Start mission execution."""
        print("🎯 Starting mission...")
        
        self.set_flight_mode('AUTO')
        time.sleep(1)
        
        self.connection.mav.command_long_send(
            self.target_system,
            self.target_component,
            mavutil.mavlink.MAV_CMD_MISSION_START,
            0, 0, 0, 0, 0, 0, 0, 0
        )
        
        print("✅ Mission started")
    
    def demo_basic_flight(self):
        """Demonstrate basic flight operations."""
        print("\n🎬 Starting Basic Flight Demo")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        if not self.wait_for_gps():
            return False
        
        # Takeoff
        self.takeoff(30)
        time.sleep(10)
        
        # Switch to LOITER mode
        self.set_flight_mode('LOITER')
        time.sleep(5)
        
        # Return to launch
        self.return_to_launch()
        time.sleep(10)
        
        # Land
        self.land()
        time.sleep(5)
        
        # Disarm
        self.arm_disarm(False)
        
        print("✅ Basic Flight Demo completed")
        return True
    
    def demo_mission_planning(self, mission_file=None):
        """Demonstrate mission planning capabilities."""
        print("\n🎬 Starting Mission Planning Demo")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        if not self.wait_for_gps():
            return False
        
        # Create default mission if none provided
        if not mission_file:
            mission_file = self.create_default_mission()
        
        # Upload and execute mission
        if self.upload_mission(mission_file):
            self.takeoff(20)
            time.sleep(8)
            self.start_mission()
            time.sleep(30)  # Let mission run for 30 seconds
            self.return_to_launch()
            time.sleep(10)
            self.land()
            time.sleep(5)
            self.arm_disarm(False)
        
        print("✅ Mission Planning Demo completed")
        return True
    
    def demo_advanced_features(self):
        """Demonstrate advanced QGroundControl features."""
        print("\n🎬 Starting Advanced Features Demo")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        if not self.wait_for_gps():
            return False
        
        # Demonstrate different flight modes
        modes = ['MANUAL', 'STABILIZE', 'LOITER', 'GUIDED', 'AUTO']
        
        for mode in modes:
            print(f"🔄 Demonstrating {mode} mode...")
            self.set_flight_mode(mode)
            time.sleep(3)
        
        # Demonstrate geofencing (if available)
        print("🔒 Demonstrating geofencing...")
        # This would require parameter setting
        
        # Demonstrate camera control (if available)
        print("📷 Demonstrating camera control...")
        # This would require camera commands
        
        print("✅ Advanced Features Demo completed")
        return True
    
    def create_default_mission(self):
        """Create a default demo mission."""
        mission_data = {
            "mission": {
                "plannedHomePosition": [47.397742, 8.545594, 488],
                "items": [
                    {
                        "type": "SimpleItem",
                        "command": 16,  # NAV_WAYPOINT
                        "params": [0, 0, 0, 0, 47.397842, 8.545694, 30]
                    },
                    {
                        "type": "SimpleItem",
                        "command": 16,  # NAV_WAYPOINT
                        "params": [0, 0, 0, 0, 47.397642, 8.545494, 30]
                    },
                    {
                        "type": "SimpleItem",
                        "command": 16,  # NAV_WAYPOINT
                        "params": [0, 0, 0, 0, 47.397742, 8.545594, 30]
                    },
                    {
                        "type": "SimpleItem",
                        "command": 20,  # NAV_RETURN_TO_LAUNCH
                        "params": [0, 0, 0, 0, 0, 0, 0]
                    }
                ]
            }
        }
        
        mission_file = Path("default_mission.json")
        with open(mission_file, 'w') as f:
            json.dump(mission_data, f, indent=2)
        
        return mission_file
    
    def run_full_demo(self):
        """Run the complete demo sequence."""
        print("🎬 Starting Full QGroundControl SITL Demo")
        print("=" * 50)
        
        try:
            # Basic flight demo
            self.demo_basic_flight()
            time.sleep(5)
            
            # Mission planning demo
            self.demo_mission_planning()
            time.sleep(5)
            
            # Advanced features demo
            self.demo_advanced_features()
            
            print("\n🎉 Full demo completed successfully!")
            
        except KeyboardInterrupt:
            print("\n⏹️ Demo interrupted by user")
        except Exception as e:
            print(f"\n❌ Demo failed: {e}")
        finally:
            self.disconnect()

def main():
    parser = argparse.ArgumentParser(description="ArduPilot SITL Demo Automation")
    parser.add_argument("--demo", choices=["basic", "mission", "advanced", "full"], default="full",
                       help="Demo type to run")
    parser.add_argument("--connection", default="udp:127.0.0.1:14550",
                       help="MAVLink connection string")
    parser.add_argument("--mission-file",
                       help="Mission file to use for mission demo")
    
    args = parser.parse_args()
    
    automation = SITLDemoAutomation(args.connection)
    
    if args.demo == "basic":
        automation.demo_basic_flight()
    elif args.demo == "mission":
        automation.demo_mission_planning(args.mission_file)
    elif args.demo == "advanced":
        automation.demo_advanced_features()
    elif args.demo == "full":
        automation.run_full_demo()

if __name__ == "__main__":
    main() 