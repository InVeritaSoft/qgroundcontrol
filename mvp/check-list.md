# Pre-Flight Checklist

## 🔌 Power System
- [ ] Battery fully charged
- [ ] Battery voltage & capacity verified (Mission Planner > Initial Setup > Battery Monitor)
- [ ] Battery connectors securely attached (e.g., XT60/XT90)

## 📡 Radio & GPS Signals *(Not valid for current test)*
- [ ] RC transmitter link stable (RSSI > 90%)
- [ ] Telemetry radio link established (Ground ↔ Air)
- [ ] GPS fix acquired (3D Fix or better)
- [ ] HDOP < 1.5, Satellites > 10
- [ ] GPS/Compass alignment verified
- [ ] No GPS glitches or jumps
- [ ] All failsafes configured (RC Loss, GCS Loss, Battery FS)

## 🧭 Sensor Quality & Calibration
- [ ] Accelerometers calibrated (flat surface)
- [ ] Compass calibrated (external preferred)
- [ ] Compass interference tested (arm & observe drift)
- [ ] EKF status OK (no warnings or high variances)

## 🖥️ Flight Controller & Software
- [ ] ArduPilot firmware is up to date
- [ ] Correct frame type selected (Quad)
- [ ] PID tuning roughly tested
- [ ] Flight modes configured (Stabilize, Loiter, RTL, etc.)
- [ ] Failsafe behavior set and verified

## ⚙️ Motors & ESCs
- [ ] All motors spin in the correct direction
- [ ] Props installed correctly (CW/CCW)
- [ ] Motor & ESC temperature normal after spin
- [ ] No unusual vibration or sound
- [ ] ESCs calibrated

## ⚡ Electrical Integrity
- [ ] No short circuits or overheating spots
- [ ] BEC voltage output verified
- [ ] Battery voltage/current logs OK in hover test

## 📊 Telemetry & Logs
- [ ] SD card inserted and logging enabled
- [ ] Log types configured (IMU, GPS, BAT, RC, etc.)
- [ ] GCS/OSD shows live: Voltage, Current, GPS, RSSI

## 🛠️ Mechanical Structure
- [ ] Frame bolts tightened
- [ ] Landing gear secured
- [ ] Props balanced
- [ ] Vibration dampening in place
- [ ] Center of Gravity (C.G.) balanced

## 📱 Ground Station / Companion Computer
- [ ] GCS connected
- [ ] Companion computer boots and connects
- [ ] Camera/Streaming working (if applicable)
- [ ] MAVLink routing verified

## 🧪 Optional Pre-Flight Tests
- [ ] Bench hover test (~15s) for logs
- [ ] ESC output PWM check
- [ ] Arm/disarm behavior test
- [ ] Vibration (VIBE.X/Y/Z) within safe limits (<30)
- [ ] Logs: CTUN, BAT, GPS all clear
