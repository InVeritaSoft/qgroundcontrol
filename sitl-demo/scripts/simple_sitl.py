#!/usr/bin/env python3
"""
Simple SITL (Software In The Loop) simulation using pymavlink
This script provides a basic MAVLink simulation for testing QGroundControl
"""

import sys
import time
import socket
import argparse
import threading
from pymavlink import mavutil
from pymavlink import mavwp
import math

# Set MAVLink version globally
try:
    mavutil.mavlink.MAVLINK_VERSION = 2
except:
    pass

class SimpleSITL:
    def __init__(self, vehicle_type='copter', port=14550):
        self.vehicle_type = vehicle_type
        self.port = port
        self.running = False
        
        # Vehicle state
        self.lat = 47.397742
        self.lon = 8.545594
        self.alt = 488.0
        self.heading = 0.0
        self.speed = 0.0
        self.armed = False
        self.mode = 'STABILIZED'
        
        # MAVLink system info
        self.system_id = 1
        self.component_id = 1
        
        # Create UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('0.0.0.0', port))
        self.sock.settimeout(1.0)
        
        print(f"🚁 Simple SITL {vehicle_type.upper()} started on port {port}")
        print(f"📍 Location: {self.lat}, {self.lon}, {self.alt}m")
        print(f"🔗 Connect QGroundControl to UDP: 127.0.0.1:{port}")
    
    def send_heartbeat(self):
        """Send MAVLink heartbeat message"""
        msg = mavutil.mavlink.MAVLink_heartbeat_message(
            type=mavutil.mavlink.MAV_TYPE_QUADROTOR if self.vehicle_type == 'copter' else 
                 mavutil.mavlink.MAV_TYPE_FIXED_WING if self.vehicle_type == 'plane' else
                 mavutil.mavlink.MAV_TYPE_GROUND_ROVER,
            autopilot=mavutil.mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA,
            base_mode=0,
            custom_mode=0,
            system_status=mavutil.mavlink.MAV_STATE_ACTIVE
        )
        self.send_message(msg)
    
    def send_gps_raw_int(self):
        """Send GPS position"""
        msg = mavutil.mavlink.MAVLink_gps_raw_int_message(
            time_usec=int(time.time() * 1000000),
            fix_type=3,  # 3D fix
            lat=int(self.lat * 1e7),
            lon=int(self.lon * 1e7),
            alt=int(self.alt * 1000),
            eph=100,
            epv=100,
            vel=int(self.speed * 100),
            cog=int(self.heading * 100),
            satellites_visible=10
        )
        self.send_message(msg)
    
    def send_attitude(self):
        """Send attitude information"""
        msg = mavutil.mavlink.MAVLink_attitude_message(
            time_boot_ms=int(time.time() * 1000),
            roll=0.0,
            pitch=0.0,
            yaw=math.radians(self.heading),
            rollspeed=0.0,
            pitchspeed=0.0,
            yawspeed=0.0
        )
        self.send_message(msg)
    
    def send_vfr_hud(self):
        """Send VFR HUD data"""
        msg = mavutil.mavlink.MAVLink_vfr_hud_message(
            airspeed=self.speed,
            groundspeed=self.speed,
            heading=int(self.heading),
            throttle=0,
            alt=self.alt,
            climb=0.0
        )
        self.send_message(msg)
    
    def send_sys_status(self):
        """Send system status"""
        msg = mavutil.mavlink.MAVLink_sys_status_message(
            voltage_battery=12000,  # 12V in millivolts
            current_battery=0,
            battery_remaining=100,
            drop_rate_comm=0,
            errors_comm=0,
            errors_count1=0,
            errors_count2=0,
            errors_count3=0,
            errors_count4=0
        )
        self.send_message(msg)
    
    def send_message(self, msg):
        """Send MAVLink message"""
        try:
            msg.pack(mavutil.mavlink.MAVLink('', 255, 1))
            data = msg.get_msgbuf()
            self.sock.sendto(data, ('127.0.0.1', self.port))
        except Exception as e:
            print(f"Error sending message: {e}")
    
    def handle_mavlink_message(self, data):
        """Handle incoming MAVLink messages"""
        try:
            msg = mavutil.mavlink.MAVLink('', 255, 1)
            msg.decode(data)
            
            if msg.get_type() == 'COMMAND_LONG':
                if msg.command == mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM:
                    self.armed = msg.param1 > 0
                    print(f"🔄 Vehicle {'ARMED' if self.armed else 'DISARMED'}")
                    
            elif msg.get_type() == 'SET_MODE':
                self.mode = msg.custom_mode
                print(f"🔄 Mode changed to: {self.mode}")
                
        except Exception as e:
            pass  # Ignore parsing errors
    
    def update_position(self):
        """Update vehicle position based on current state"""
        if self.armed and self.mode in ['AUTO', 'GUIDED']:
            # Simulate movement
            speed_ms = self.speed / 3.6  # Convert km/h to m/s
            distance = speed_ms * 0.1  # Update every 100ms
            
            # Update position based on heading
            lat_change = distance * math.cos(math.radians(self.heading)) / 111320.0
            lon_change = distance * math.sin(math.radians(self.heading)) / (111320.0 * math.cos(math.radians(self.lat)))
            
            self.lat += lat_change
            self.lon += lon_change
    
    def run(self):
        """Main simulation loop"""
        self.running = True
        last_heartbeat = 0
        last_position_update = 0
        
        print("🚀 Starting SITL simulation...")
        print("📡 Listening for MAVLink messages...")
        
        while self.running:
            current_time = time.time()
            
            # Send periodic messages
            if current_time - last_heartbeat > 1.0:
                self.send_heartbeat()
                self.send_gps_raw_int()
                self.send_attitude()
                self.send_vfr_hud()
                self.send_sys_status()
                last_heartbeat = current_time
            
            # Update position
            if current_time - last_position_update > 0.1:
                self.update_position()
                last_position_update = current_time
            
            # Handle incoming messages
            try:
                data, addr = self.sock.recvfrom(1024)
                self.handle_mavlink_message(data)
            except socket.timeout:
                pass
            except Exception as e:
                print(f"Error receiving message: {e}")
    
    def stop(self):
        """Stop the simulation"""
        self.running = False
        self.sock.close()
        print("🛑 SITL simulation stopped")

def main():
    parser = argparse.ArgumentParser(description='Simple SITL Simulation')
    parser.add_argument('--vehicle', choices=['copter', 'plane', 'rover'], 
                       default='copter', help='Vehicle type to simulate')
    parser.add_argument('--port', type=int, default=14550, 
                       help='UDP port to listen on')
    
    args = parser.parse_args()
    
    try:
        sitl = SimpleSITL(args.vehicle, args.port)
        sitl.run()
    except KeyboardInterrupt:
        print("\n🛑 Interrupted by user")
        sitl.stop()
    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main() 