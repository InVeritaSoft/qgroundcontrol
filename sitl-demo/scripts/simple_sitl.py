#!/usr/bin/env python3
"""
Simple SITL (Software In The Loop) simulation using pymavlink
This script provides a basic MAVLink simulation for testing QGroundControl.

Improvements:
- Use pymavlink's connection API to correctly encode and send MAVLink messages
- Simplify networking by using udpout to QGC (127.0.0.1:<port>)
- Clean shutdown and basic mode/arming state handling
"""

import sys
import time
import argparse
import math
from typing import Optional, List, Dict
from pymavlink import mavutil

DEFAULT_LAT = 47.397742
DEFAULT_LON = 8.545594
DEFAULT_ALT = 488.0

class SimpleSITL:
    def __init__(self, vehicle_type: str = 'copter', port: int = 14550, rate_hz: float = 1.0,
                 sysid: int = 1, mission_file: Optional[str] = None, auto_start: bool = False,
                 start_lat: Optional[float] = None, start_lon: Optional[float] = None,
                 start_alt: Optional[float] = None):
        self.vehicle_type: str = vehicle_type
        self.port: int = port
        self.rate_hz: float = max(rate_hz, 0.5)
        self.running: bool = False

        # Vehicle state
        self.lat: float = start_lat if start_lat is not None else DEFAULT_LAT
        self.lon: float = start_lon if start_lon is not None else DEFAULT_LON
        self.alt: float = start_alt if start_alt is not None else DEFAULT_ALT
        self.heading: float = 0.0
        self.speed: float = 10.0  # km/h cruise for AUTO demo
        self.armed: bool = auto_start
        self.mode: str = 'AUTO' if auto_start else 'STABILIZE'

        # Time since boot (monotonic) for MAVLink fields requiring milliseconds since boot
        self._boot_time_monotonic: float = time.monotonic()

        # Minimal parameter store to satisfy QGC queries
        # Using common ArduPilot-style failsafe params as placeholders
        self.parameters: Dict[str, float] = {
            'FS_OPTIONS': 0.0,
            'FS_GCS_TIMEOUT': 5.0,
            'FS_GCS_ENABLE': 0.0,
        }

        # Home position (lat, lon in deg, alt in meters AMSL)
        self.home_lat: float = self.lat
        self.home_lon: float = self.lon
        self.home_alt: float = self.alt

        # MAVLink connections
        # - tx: send to QGC on host
        # - rx: receive commands from QGC (binds on container/all interfaces)
        self.mav: Optional[mavutil.mavfile] = None
        self.rx: Optional[mavutil.mavfile] = None
        self.sysid: int = max(1, min(sysid, 255))

        # Mission state
        self.mission: List[Dict[str, float]] = []
        self.current_wp_index: int = 0
        if mission_file:
            self._load_mission(mission_file)

        print(f"🚁 Simple SITL {vehicle_type.upper()} started on port {port}")
        print(f"📍 Location: {self.lat}, {self.lon}, {self.alt}m")
        print(f"🔗 Connect QGroundControl to UDP: 127.0.0.1:{port}")
    
    def send_heartbeat(self) -> None:
        """Send MAVLink heartbeat message using connection API."""
        vehicle_type = (
            mavutil.mavlink.MAV_TYPE_QUADROTOR if self.vehicle_type == 'copter'
            else mavutil.mavlink.MAV_TYPE_FIXED_WING if self.vehicle_type == 'plane'
            else mavutil.mavlink.MAV_TYPE_GROUND_ROVER
        )

        base_mode = 0
        if self.armed:
            base_mode |= mavutil.mavlink.MAV_MODE_FLAG_SAFETY_ARMED

        self.mav.mav.heartbeat_send(
            vehicle_type,
            mavutil.mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA,
            base_mode,
            0,  # custom_mode (firmware-specific)
            mavutil.mavlink.MAV_STATE_ACTIVE,
        )
    
    def send_gps_raw_int(self) -> None:
        """Send GPS position."""
        self.mav.mav.gps_raw_int_send(
            int(time.time() * 1_000_000),
            3,  # 3D fix
            int(self.lat * 1e7),
            int(self.lon * 1e7),
            int(self.alt * 1000),
            100,  # eph (cm)
            100,  # epv (cm)
            int(self.speed * 100),
            int(self.heading * 100),
            10,
        )
    
    def send_attitude(self) -> None:
        """Send attitude information."""
        # MAVLink expects time since boot in milliseconds (uint32)
        time_boot_ms = int((time.monotonic() - self._boot_time_monotonic) * 1000)
        self.mav.mav.attitude_send(
            time_boot_ms,
            0.0,
            0.0,
            math.radians(self.heading),
            0.0,
            0.0,
            0.0,
        )
    
    def send_vfr_hud(self) -> None:
        """Send VFR HUD data."""
        self.mav.mav.vfr_hud_send(
            self.speed,
            self.speed,
            int(self.heading),
            0,
            self.alt,
            0.0,
        )
    
    def send_sys_status(self) -> None:
        """Send system status."""
        self.mav.mav.sys_status_send(
            0,  # onboard_control_sensors_present (bitmask)
            0,  # onboard_control_sensors_enabled (bitmask)
            0,  # onboard_control_sensors_health  (bitmask)
            200,      # load (0-1000 -> 0-100%)
            12000,    # voltage_battery (mV)
            0,        # current_battery (10 * mA)
            100,      # battery_remaining (%) or -1 unknown
            0,        # drop_rate_comm (1/100 %)
            0,        # errors_comm
            0, 0, 0, 0  # errors_count1..4
        )
    
    def ensure_connection(self) -> None:
        if self.mav is None:
            # Send to host.docker.internal if available, else 172.17.0.1 as fallback
            # Allow override via environment SITL_TX_HOST
            import os
            tx_host = os.getenv('SITL_TX_HOST', 'host.docker.internal')
            self.mav = mavutil.mavlink_connection(
                f'udpout:{tx_host}:{self.port}', autoreconnect=True,
                source_system=self.sysid, source_component=1
            )
        if self.rx is None:
            # Listen for commands from QGC
            self.rx = mavutil.mavlink_connection(
                f'udpin:0.0.0.0:{self.port}', autoreconnect=True,
                source_system=self.sysid, source_component=1
            )
            self.rx.wait_heartbeat(timeout=2)

    def _send_param_value(self, name: str, index: int, count: int) -> None:
        try:
            value = float(self.parameters.get(name, 0.0))
            # param_id must be <=16 chars
            param_id = name[:16].encode('ascii')
            self.mav.mav.param_value_send(
                param_id,
                value,
                mavutil.mavlink.MAV_PARAM_TYPE_INT32,
                count,
                index,
            )
        except Exception:
            pass
    
    def update_position(self) -> None:
        """Update vehicle position based on current state."""
        if self.armed and self.mode in ['AUTO', 'GUIDED']:
            speed_ms = self.speed / 3.6
            dt = (1.0 / max(self.rate_hz, 0.5))

            if self.mode == 'AUTO' and self.mission:
                # Move toward current waypoint
                wp = self.mission[self.current_wp_index]
                target_lat = wp['lat']
                target_lon = wp['lon']
                target_alt = wp.get('alt', self.alt)

                # Compute bearing and distance (flat-earth approx)
                dlat = (target_lat - self.lat) * 111320.0
                dlon = (target_lon - self.lon) * 111320.0 * math.cos(math.radians(self.lat))
                distance = math.hypot(dlat, dlon)

                if distance < 2.0:  # within 2 meters -> advance waypoint
                    if self.current_wp_index < len(self.mission) - 1:
                        self.current_wp_index += 1
                    # Update heading for next leg
                    return

                bearing_rad = math.atan2(dlon, dlat)
                self.heading = math.degrees(bearing_rad) % 360.0

                step = speed_ms * dt
                if step > distance:
                    step = distance

                lat_change = (step * math.cos(bearing_rad)) / 111320.0
                lon_change = (step * math.sin(bearing_rad)) / (111320.0 * math.cos(math.radians(self.lat)))
                self.lat += lat_change
                self.lon += lon_change
                # Simple altitude convergence
                self.alt += max(-1.0, min(1.0, target_alt - self.alt))
            else:
                # GUIDED: straight line by heading
                distance = speed_ms * dt
                lat_change = distance * math.cos(math.radians(self.heading)) / 111320.0
                lon_change = distance * math.sin(math.radians(self.heading)) / (111320.0 * math.cos(math.radians(self.lat)))
                self.lat += lat_change
                self.lon += lon_change

    def _load_mission(self, mission_file: str) -> None:
        try:
            import json
            with open(mission_file, 'r') as f:
                data = json.load(f)
            items = data.get('mission', {}).get('items', [])
            waypoints: List[Dict[str, float]] = []
            for item in items:
                if int(item.get('command', 0)) in (16, 22, 20):
                    params = item.get('params', [])
                    if len(params) >= 7:
                        waypoints.append({'lat': float(params[4]), 'lon': float(params[5]), 'alt': float(params[6])})
            if waypoints:
                self.mission = waypoints
                self.current_wp_index = 0
                # Set start to home if available
                home = data.get('mission', {}).get('plannedHomePosition', None)
                if home and len(home) >= 3:
                    self.lat, self.lon, self.alt = float(home[0]), float(home[1]), float(home[2])
        except Exception as e:
            print(f"⚠️  Failed to load mission file {mission_file}: {e}")
    
    def run(self) -> None:
        """Main simulation loop."""
        self.ensure_connection()
        self.running = True

        print("🚀 Starting SITL simulation...")

        period = 1.0 / self.rate_hz
        try:
            while self.running:
                # poll inbound messages (non-blocking)
                try:
                    msg = self.rx.recv_match(blocking=False)
                    if msg:
                        mtype = msg.get_type()
                        if mtype == 'COMMAND_LONG' and getattr(msg, 'command', None) == mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM:
                            self.armed = (int(getattr(msg, 'param1', 0)) == 1)
                        elif mtype == 'COMMAND_LONG' and getattr(msg, 'command', None) == mavutil.mavlink.MAV_CMD_DO_SET_HOME:
                            # QGC expects a COMMAND_ACK in response. Update stored home if provided.
                            use_current = int(getattr(msg, 'param1', 0)) == 1
                            if use_current:
                                self.home_lat, self.home_lon, self.home_alt = self.lat, self.lon, self.alt
                            else:
                                # params 5/6/7 are lat/lon/alt
                                lat = float(getattr(msg, 'param5', self.home_lat) or self.home_lat)
                                lon = float(getattr(msg, 'param6', self.home_lon) or self.home_lon)
                                alt = float(getattr(msg, 'param7', self.home_alt) or self.home_alt)
                                self.home_lat, self.home_lon, self.home_alt = lat, lon, alt

                            # Send ACK (accepted)
                            try:
                                self.mav.mav.command_ack_send(
                                    mavutil.mavlink.MAV_CMD_DO_SET_HOME,
                                    mavutil.mavlink.MAV_RESULT_ACCEPTED,
                                    0, 0,
                                    getattr(msg, 'target_system', 0) or 0,
                                    getattr(msg, 'target_component', 0) or 0,
                                )
                                # Also emit HOME_POSITION to reflect the change
                                self.mav.mav.home_position_send(
                                    int(self.home_lat * 1e7),
                                    int(self.home_lon * 1e7),
                                    int(self.home_alt * 1000),
                                    0, 0, 0,  # local NED not modeled
                                    (1.0, 0.0, 0.0, 0.0),  # quaternion w,x,y,z
                                    0.0, 0.0, 0.0,
                                )
                            except Exception:
                                pass
                        elif mtype == 'COMMAND_INT' and getattr(msg, 'command', None) == mavutil.mavlink.MAV_CMD_DO_SET_HOME:
                            # COMMAND_INT variant: param1=use_current, x=lat*1e7, y=lon*1e7, z=alt
                            use_current = int(getattr(msg, 'param1', 0)) == 1
                            if use_current:
                                self.home_lat, self.home_lon, self.home_alt = self.lat, self.lon, self.alt
                            else:
                                lat = getattr(msg, 'x', int(self.home_lat * 1e7))
                                lon = getattr(msg, 'y', int(self.home_lon * 1e7))
                                alt = float(getattr(msg, 'z', self.home_alt))
                                self.home_lat = float(lat) / 1e7
                                self.home_lon = float(lon) / 1e7
                                self.home_alt = float(alt)

                            try:
                                self.mav.mav.command_ack_send(
                                    mavutil.mavlink.MAV_CMD_DO_SET_HOME,
                                    mavutil.mavlink.MAV_RESULT_ACCEPTED,
                                    0, 0,
                                    getattr(msg, 'target_system', 0) or 0,
                                    getattr(msg, 'target_component', 0) or 0,
                                )
                                self.mav.mav.home_position_send(
                                    int(self.home_lat * 1e7),
                                    int(self.home_lon * 1e7),
                                    int(self.home_alt * 1000),
                                    0, 0, 0,
                                    (1.0, 0.0, 0.0, 0.0),
                                    0.0, 0.0, 0.0,
                                )
                            except Exception:
                                pass
                        elif mtype == 'SET_MODE':
                            # best-effort mode decode
                            self.mode = str(getattr(msg, 'custom_mode', self.mode))
                        elif mtype == 'PARAM_REQUEST_LIST':
                            names = list(self.parameters.keys())
                            total = len(names)
                            for idx, pname in enumerate(names):
                                self._send_param_value(pname, idx, total)
                        elif mtype == 'PARAM_REQUEST_READ':
                            # QGC may request by name or index
                            requested_name = getattr(msg, 'param_id', b'')
                            if isinstance(requested_name, bytes):
                                requested_name = requested_name.decode('ascii', errors='ignore').strip('\x00')
                            if requested_name and requested_name in self.parameters:
                                names = list(self.parameters.keys())
                                self._send_param_value(requested_name, names.index(requested_name), len(names))
                            else:
                                # fallback by index
                                idx = int(getattr(msg, 'param_index', -1))
                                names = list(self.parameters.keys())
                                if 0 <= idx < len(names):
                                    self._send_param_value(names[idx], idx, len(names))
                        elif mtype == 'PARAM_SET':
                            pname = getattr(msg, 'param_id', b'')
                            if isinstance(pname, bytes):
                                pname = pname.decode('ascii', errors='ignore').strip('\x00')
                            pval = float(getattr(msg, 'param_value', 0.0))
                            if pname:
                                self.parameters[pname] = pval
                                names = list(self.parameters.keys())
                                self._send_param_value(pname, names.index(pname), len(names))
                except Exception:
                    pass
                self.send_heartbeat()
                self.send_gps_raw_int()
                self.send_attitude()
                self.send_vfr_hud()
                self.send_sys_status()

                self.update_position()
                time.sleep(period)
        except KeyboardInterrupt:
            pass
        finally:
            self.stop()
    
    def stop(self) -> None:
        """Stop the simulation."""
        self.running = False
        try:
            if self.mav is not None:
                self.mav.close()
            if self.rx is not None:
                self.rx.close()
        finally:
            print("🛑 SITL simulation stopped")

def main() -> None:
    parser = argparse.ArgumentParser(description='Simple SITL Simulation')
    parser.add_argument('--vehicle', choices=['copter', 'plane', 'rover'],
                        default='copter', help='Vehicle type to simulate')
    parser.add_argument('--port', type=int, default=14550,
                        help='UDP port for QGC to connect to')
    parser.add_argument('--rate', type=float, default=1.0,
                        help='Message rate in Hz (default 1.0)')
    parser.add_argument('--sysid', type=int, default=1, help='MAVLink system id (1-255)')
    parser.add_argument('--mission-file', type=str, help='Path to mission JSON file')
    parser.add_argument('--auto-start', action='store_true', help='Start armed in AUTO mode')
    parser.add_argument('--start-lat', type=float, help='Override start latitude')
    parser.add_argument('--start-lon', type=float, help='Override start longitude')
    parser.add_argument('--start-alt', type=float, help='Override start altitude (m)')

    args = parser.parse_args()

    sitl = SimpleSITL(
        vehicle_type=args.vehicle,
        port=args.port,
        rate_hz=args.rate,
        sysid=args.sysid,
        mission_file=args.mission_file,
        auto_start=args.auto_start,
        start_lat=args.start_lat,
        start_lon=args.start_lon,
        start_alt=args.start_alt,
    )
    sitl.run()

if __name__ == '__main__':
    main() 