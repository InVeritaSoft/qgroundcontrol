#!/usr/bin/env python3
"""
ArduPilot SITL Demo Manager
This script provides easy management of the SITL demo environment.
"""

import os
import sys
import subprocess
import time
import json
import argparse
from pathlib import Path

class SITLDemoManager:
    def __init__(self, demo_dir):
        self.demo_dir = Path(demo_dir)
        self.docker_compose_file = self.demo_dir / "docker-compose.yml"
        self.config_dir = self.demo_dir / "configs"
        self.logs_dir = self.demo_dir / "logs"
        
    def check_prerequisites(self):
        """Check if Docker and Docker Compose are available."""
        try:
            subprocess.run(["docker", "--version"], check=True, capture_output=True)
            subprocess.run(["docker-compose", "--version"], check=True, capture_output=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("❌ Docker or Docker Compose not found. Please install them first.")
            return False
    
    def start_services(self, vehicle_type="all"):
        """Start SITL services."""
        if not self.check_prerequisites():
            return False
            
        print(f"🚀 Starting SITL demo environment...")
        
        # Create necessary directories
        self.logs_dir.mkdir(exist_ok=True)
        
        # Start services based on vehicle type
        if vehicle_type == "all":
            cmd = ["docker-compose", "-f", str(self.docker_compose_file), "up", "-d"]
        else:
            cmd = ["docker-compose", "-f", str(self.docker_compose_file), "up", "-d", f"sitl-{vehicle_type}"]
        
        try:
            result = subprocess.run(cmd, cwd=self.demo_dir, check=True, capture_output=True, text=True)
            print("✅ SITL services started successfully!")
            self.show_status()
            return True
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to start services: {e}")
            print(f"Error output: {e.stderr}")
            return False
    
    def stop_services(self):
        """Stop all SITL services."""
        print("🛑 Stopping SITL demo environment...")
        
        try:
            cmd = ["docker-compose", "-f", str(self.docker_compose_file), "down"]
            subprocess.run(cmd, cwd=self.demo_dir, check=True, capture_output=True, text=True)
            print("✅ SITL services stopped successfully!")
            return True
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to stop services: {e}")
            return False
    
    def restart_services(self, vehicle_type="all"):
        """Restart SITL services."""
        print("🔄 Restarting SITL demo environment...")
        self.stop_services()
        time.sleep(2)
        return self.start_services(vehicle_type)
    
    def show_status(self):
        """Show status of running services."""
        print("\n📊 SITL Demo Status:")
        print("=" * 50)
        
        try:
            cmd = ["docker-compose", "-f", str(self.docker_compose_file), "ps"]
            result = subprocess.run(cmd, cwd=self.demo_dir, check=True, capture_output=True, text=True)
            print(result.stdout)
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to get status: {e}")
    
    def show_logs(self, service="sitl-copter", lines=50):
        """Show logs for a specific service."""
        print(f"📋 Showing logs for {service} (last {lines} lines):")
        print("=" * 50)
        
        try:
            cmd = ["docker-compose", "-f", str(self.docker_compose_file), "logs", "--tail", str(lines), service]
            result = subprocess.run(cmd, cwd=self.demo_dir, check=True, capture_output=True, text=True)
            print(result.stdout)
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to get logs: {e}")
    
    def check_connection(self):
        """Check if QGroundControl can connect to SITL."""
        print("🔍 Checking SITL connection...")
        
        try:
            import socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2)
            
            # Test UDP port 14550
            result = sock.connect_ex(('127.0.0.1', 14550))
            if result == 0:
                print("✅ SITL is listening on UDP port 14550")
            else:
                print("❌ SITL is not listening on UDP port 14550")
            
            sock.close()
        except Exception as e:
            print(f"❌ Connection check failed: {e}")
    
    def create_demo_mission(self, mission_type="simple"):
        """Create a demo mission file."""
        missions_dir = self.demo_dir / "missions"
        missions_dir.mkdir(exist_ok=True)
        
        if mission_type == "simple":
            mission_data = {
                "mission": {
                    "plannedHomePosition": [47.397742, 8.545594, 488],
                    "items": [
                        {
                            "type": "SimpleItem",
                            "command": 16,  # NAV_WAYPOINT
                            "params": [0, 0, 0, 0, 47.397842, 8.545694, 50]
                        },
                        {
                            "type": "SimpleItem",
                            "command": 16,  # NAV_WAYPOINT
                            "params": [0, 0, 0, 0, 47.397642, 8.545494, 50]
                        },
                        {
                            "type": "SimpleItem",
                            "command": 20,  # NAV_RETURN_TO_LAUNCH
                            "params": [0, 0, 0, 0, 0, 0, 0]
                        }
                    ]
                }
            }
        
        mission_file = missions_dir / f"{mission_type}_mission.json"
        with open(mission_file, 'w') as f:
            json.dump(mission_data, f, indent=2)
        
        print(f"✅ Created demo mission: {mission_file}")
        return mission_file
    
    def setup_qgc_connection(self):
        """Provide instructions for QGroundControl connection."""
        print("\n🔗 QGroundControl Connection Instructions:")
        print("=" * 50)
        print("1. Open QGroundControl")
        print("2. Go to Settings → Comm Links")
        print("3. Add a new UDP connection:")
        print("   - Name: SITL Demo")
        print("   - Host: 127.0.0.1")
        print("   - Port: 14550")
        print("   - AutoConnect: Checked")
        print("4. Click 'Add' and then 'Connect'")
        print("\nAlternative ports for different vehicles:")
        print("- Copter: 127.0.0.1:14550")
        print("- Plane: 127.0.0.1:14552")
        print("- Rover: 127.0.0.1:14554")

def main():
    parser = argparse.ArgumentParser(description="ArduPilot SITL Demo Manager")
    parser.add_argument("action", choices=["start", "stop", "restart", "status", "logs", "check", "mission", "help"],
                       help="Action to perform")
    parser.add_argument("--vehicle", choices=["copter", "plane", "rover", "all"], default="all",
                       help="Vehicle type to start (default: all)")
    parser.add_argument("--service", default="sitl-copter",
                       help="Service name for logs command")
    parser.add_argument("--lines", type=int, default=50,
                       help="Number of log lines to show")
    parser.add_argument("--demo-dir", default=".",
                       help="Demo directory path")
    
    args = parser.parse_args()
    
    manager = SITLDemoManager(args.demo_dir)
    
    if args.action == "start":
        manager.start_services(args.vehicle)
    elif args.action == "stop":
        manager.stop_services()
    elif args.action == "restart":
        manager.restart_services(args.vehicle)
    elif args.action == "status":
        manager.show_status()
    elif args.action == "logs":
        manager.show_logs(args.service, args.lines)
    elif args.action == "check":
        manager.check_connection()
    elif args.action == "mission":
        manager.create_demo_mission()
    elif args.action == "help":
        manager.setup_qgc_connection()

if __name__ == "__main__":
    main() 