# ArduPilot SITL (Software In The Loop) Demo Guide

This guide provides comprehensive instructions for setting up and using ArduPilot SITL for QGroundControl demonstrations and development testing.

## 📁 Project Structure

```
sitl-demo/
├── README.md                 # This file
├── docker-compose.yml        # Docker Compose configuration
├── configs/                  # SITL configuration files
│   ├── copter_demo.parm     # Copter parameters
│   ├── plane_demo.parm      # Plane parameters
│   └── rover_demo.parm      # Rover parameters
├── scripts/                  # Management scripts
│   ├── start_demo.py        # Main demo manager
│   ├── demo_automation.py   # Automated demo sequences
│   ├── requirements.txt     # Python dependencies
│   ├── quick_start.bat      # Windows quick start
│   └── quick_start.sh       # Linux/macOS quick start
├── manager/                  # Web-based management interface
│   ├── app.py              # Flask web application
│   └── templates/          # HTML templates
├── logs/                    # Log files (created automatically)
├── missions/               # Mission files (created automatically)
└── data/                   # Data files (created automatically)
```

## 🚀 Quick Start

### Prerequisites

- **Docker and Docker Compose** installed
- **Python 3.7+** (for scripts and automation)
- **QGroundControl** installed

### Option 1: Using Quick Start Scripts

**Windows:**
```bash
cd sitl-demo
scripts\quick_start.bat
```

**Linux/macOS:**
```bash
cd sitl-demo
chmod +x scripts/quick_start.sh
./scripts/quick_start.sh
```

### Option 2: Using Python Scripts

```bash
cd sitl-demo

# Install dependencies
pip install -r scripts/requirements.txt

# Start all services
python scripts/start_demo.py start

# Check status
python scripts/start_demo.py status

# Run automated demo
python scripts/demo_automation.py --demo full
```

### Option 3: Using Docker Compose Directly

```bash
cd sitl-demo

# Start all services
docker-compose up -d

# Start specific vehicle
docker-compose up -d sitl-copter

# View logs
docker-compose logs -f sitl-copter

# Stop all services
docker-compose down
```

## 🌐 Web Management Interface

The SITL demo includes a web-based management interface:

1. **Start the web manager:**
   ```bash
   cd sitl-demo
   python manager/app.py
   ```

2. **Access the interface:**
   - Main Dashboard: http://localhost:8082
   - Service Logs: http://localhost:8082/logs
   - Configuration: http://localhost:8082/config

3. **Features:**
   - Start/stop/restart services
   - View real-time logs
   - Monitor service status
   - View configuration files

## 🔗 Connecting QGroundControl

1. **Open QGroundControl**
2. **Go to Settings → Comm Links**
3. **Add a new UDP connection:**
   - **Name**: SITL Demo
   - **Host**: `127.0.0.1`
   - **Port**: `14550` (copter), `14552` (plane), `14554` (rover)
   - **AutoConnect**: Checked
4. **Click 'Add' and then 'Connect'**

## 🎬 Demo Scenarios

### 1. Basic Flight Demo
```bash
python scripts/demo_automation.py --demo basic
```

### 2. Mission Planning Demo
```bash
python scripts/demo_automation.py --demo mission
```

### 3. Advanced Features Demo
```bash
python scripts/demo_automation.py --demo advanced
```

### 4. Full Demo Sequence
```bash
python scripts/demo_automation.py --demo full
```

## 🛠️ Management Commands

### Service Management
```bash
# Start all services
python scripts/start_demo.py start

# Start specific vehicle
python scripts/start_demo.py start --vehicle copter

# Stop all services
python scripts/start_demo.py stop

# Restart services
python scripts/start_demo.py restart

# Show status
python scripts/start_demo.py status
```

### Logging and Monitoring
```bash
# View logs
python scripts/start_demo.py logs --service sitl-copter --lines 100

# Check connection
python scripts/start_demo.py check

# Create demo mission
python scripts/start_demo.py mission
```

### Help and Instructions
```bash
# Show QGroundControl connection instructions
python scripts/start_demo.py help
```

## 🔧 Configuration

### Vehicle Parameters

The demo includes pre-configured parameter files for different vehicle types:

- **`configs/copter_demo.parm`** - Multi-rotor vehicle configuration
- **`configs/plane_demo.parm`** - Fixed-wing aircraft configuration  
- **`configs/rover_demo.parm`** - Ground vehicle configuration

### Key Parameters

```bash
# GPS Simulation
SIM_GPS_ENABLE 1
SIM_GPS_LAT 47.397742
SIM_GPS_LON 8.545594
SIM_GPS_ALT 488

# Vehicle Configuration
FRAME_CLASS 1  # Quadcopter
FRAME_TYPE 0   # Plus frame

# Flight Modes
FLTMODE1 3     # Manual
FLTMODE2 4     # Stabilize
FLTMODE3 6     # Loiter

# Safety Features
FENCE_ENABLE 1
FENCE_ALT_MAX 100
FENCE_RADIUS 100
```

## 🐛 Troubleshooting

### Common Issues

#### 1. Connection Problems
```bash
# Check if SITL is running
python scripts/start_demo.py status

# Check connection
python scripts/start_demo.py check

# Restart services
python scripts/start_demo.py restart
```

#### 2. No GPS Signal
- Verify `SIM_GPS_ENABLE 1` in parameter files
- Check GPS position parameters
- Restart SITL services

#### 3. Vehicle Not Responding
- Check flight mode settings
- Verify RC input simulation
- Check parameter configuration

#### 4. Performance Issues
- Reduce simulation speed: `SIM_SPEEDUP 0.5`
- Disable unnecessary features
- Monitor system resources

### Debug Commands

```bash
# View service logs
docker-compose logs sitl-copter

# Check container status
docker-compose ps

# Monitor system resources
docker stats
```

## 📊 Available Services

| Service | Port | Description |
|---------|------|-------------|
| SITL Copter | 14550 | Multi-rotor simulation |
| SITL Plane | 14552 | Fixed-wing simulation |
| SITL Rover | 14554 | Ground vehicle simulation |
| MAVProxy | 8080 | MAVLink proxy and web interface |
| Mission Planner | 8081 | Mission planning interface |
| SITL Manager | 8082 | Web-based management interface |
| Performance Monitor | 8083 | System resource monitoring |

## 🔄 Advanced Usage

### Custom Vehicle Models

Create custom parameter files:

```bash
# Create custom configuration
cat > configs/custom_vehicle.parm << EOF
FRAME_CLASS 1
FRAME_TYPE 0
MOT_PWM_TYPE 1
MOT_PWM_RATE 490
SIM_GPS_ENABLE 1
SIM_GPS_LAT 47.397742
SIM_GPS_LON 8.545594
EOF

# Start with custom parameters
docker-compose up -d sitl-copter
```

### Multi-Vehicle Operations

```bash
# Start multiple vehicles
docker-compose up -d sitl-copter sitl-plane

# Connect to different ports in QGroundControl
# Copter: 127.0.0.1:14550
# Plane: 127.0.0.1:14552
```

### Integration with CI/CD

```yaml
# .github/workflows/sitl-test.yml
name: SITL Tests
on: [push, pull_request]

jobs:
  sitl-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Start SITL
        run: |
          cd sitl-demo
          docker-compose up -d sitl-copter
          sleep 30
      - name: Run Tests
        run: |
          python scripts/demo_automation.py --demo basic
      - name: Cleanup
        run: docker-compose down
```

## 📚 Additional Resources

- [ArduPilot Documentation](https://ardupilot.org/)
- [QGroundControl Documentation](https://docs.qgroundcontrol.com/)
- [MAVProxy Documentation](https://ardupilot.org/mavproxy/)
- [Docker Compose Documentation](https://docs.docker.com/compose/)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the same terms as QGroundControl.

---

**Happy Flying! 🚁✈️🚗** 