# ArduPilot SITL Demo with AreaPlanner Integration

A comprehensive demonstration environment for ArduPilot Software In The Loop (SITL) simulation with QGroundControl AreaPlanner integration.

## 🚀 Quick Start

### 1. Start All Services
```bash
cd sitl-demo
# Docker Compose v2
docker compose up -d
# or legacy
docker-compose up -d
```

### 2. Connect QGroundControl
- Open QGroundControl
- Connect to SITL: **UDP 127.0.0.1:14550**

### 3. Access Web Interfaces
- **SITL Manager**: http://localhost:8082
- **AreaPlanner Integration**: http://localhost:8084
- **QGC Bridge**: http://localhost:8085
- **Complete Demo**: http://localhost:8086
- **MAVProxy**: http://localhost:8080
- **Performance Monitor**: http://localhost:8083

## 🎯 AreaPlanner Integration

### Using AreaPlanner with SITL

1. **Start SITL Environment:**
   ```bash
   cd sitl-demo
   docker compose up -d  # or: docker-compose up -d
   ```

2. **Open QGroundControl and Connect:**
   - Open QGroundControl
   - Connect to SITL using UDP: `127.0.0.1:14550`

3. **Use AreaPlanner:**
   - Go to **Plan View**
   - Click on **Area Plan** tab
   - Configure area parameters:
     - Area Width: 100m
     - Area Height: 100m
     - Line Spacing: 10m
     - Mission Altitude: 30m
   - Position area on map
   - Click **Generate Mission**

4. **Upload and Execute:**
   - Review waypoints on map
   - Click **Upload to Vehicle**
   - Switch to **Fly view**
   - Click **Start Mission**

5. **Monitor Execution:**
   - Watch vehicle follow waypoints
   - Monitor progress in Fly view
   - Vehicle will return to launch when complete

## 🎬 Demo Scenarios

### 1. Multi-Drone AUTO Demo (up to 3 drones)
```bash
# Start three simulated copters on different UDP ports and SYSIDs
docker compose up -d sitl-copter sitl-copter-2 sitl-copter-3  # or legacy docker-compose

# QGroundControl: add UDP links (or rely on AutoConnect)
# - Drone 1: 127.0.0.1:14550 (SYSID 1)
# - Drone 2: 127.0.0.1:14552 (SYSID 2)
# - Drone 3: 127.0.0.1:14554 (SYSID 3)

# All 3 start in AUTO mode and follow simple routes
```

### 2. Basic SITL Demo
```bash
# Start basic SITL services
docker compose up sitl-copter -d  # or: docker-compose up sitl-copter -d

# Run basic automation
python scripts/demo_automation.py --demo basic
```

### 3. Multi-Vehicle Demo
```bash
# Start all vehicle types
docker compose up sitl-copter sitl-plane sitl-rover -d  # or: docker-compose up ...

# Run multi-vehicle demo
python scripts/demo_automation.py --demo multi
```

### 4. Mission Planning Demo
```bash
# Start with mission planning
docker-compose up sitl-copter mavproxy -d

# Run mission planning demo
python scripts/demo_automation.py --demo mission
```

### 5. Advanced Features Demo
```bash
# Start all services
docker compose up -d  # or: docker-compose up -d

# Run advanced demo
python scripts/demo_automation.py --demo advanced
```

### 6. Full Demo
```bash
# Start complete environment
docker compose up -d  # or: docker-compose up -d

# Run complete demo
python scripts/demo_automation.py --demo full
```

### 7. AreaPlanner Integration Demo
```bash
# Run AreaPlanner workflow demo
docker compose up area-planner-integration -d  # or: docker-compose up area-planner-integration -d

# Interactive AreaPlanner demo
docker compose up qgc-area-planner-bridge -d  # or: docker-compose up qgc-area-planner-bridge -d

# Complete AreaPlanner demo
docker compose up complete-area-planner-demo -d  # or: docker-compose up complete-area-planner-demo -d
```

## 🛠️ Management Commands

### Service Management
```bash
# Start all services
docker compose up -d  # or: docker-compose up -d

# Start specific service
docker compose up sitl-copter -d  # or: docker-compose up sitl-copter -d

# Stop all services
docker compose down  # or: docker-compose down

# Restart services
docker compose restart  # or: docker-compose restart

# View logs
docker compose logs -f sitl-copter  # or: docker-compose logs -f sitl-copter
```

### Quick Scripts
```bash
# Start demo environment
python scripts/start_demo.py start

# Stop demo environment
python scripts/start_demo.py stop

# Check status
python scripts/start_demo.py status

# View logs
python scripts/start_demo.py logs

# Check connection
python scripts/start_demo.py check

# Show QGroundControl connection instructions
python scripts/start_demo.py help

# Show AreaPlanner integration guide
python scripts/qgc_area_planner_bridge.py --demo guide
```

## 🔧 Configuration

### Vehicle Parameters
- **Copter**: `configs/copter_demo.parm`
- **Plane**: `configs/plane_demo.parm`
- **Rover**: `configs/rover_demo.parm`

### Environment Variables
```bash
# MAVLink Configuration
MAVLINK_HOST=sitl-copter
MAVLINK_PORT=14550
VEHICLE_TYPE=copter

# Logging
LOG_LEVEL=INFO
DATA_DIR=/app/data

# Multi-drone UDP ports mapping used here
# SYSID 1 -> 14550, SYSID 2 -> 14552, SYSID 3 -> 14554
```

## 📁 Directory Structure
```
sitl-demo/
├── docker-compose.yml          # Docker services configuration
├── configs/                    # Vehicle parameter files
│   ├── copter_demo.parm
│   ├── plane_demo.parm
│   └── rover_demo.parm
├── scripts/                    # Python automation scripts
│   ├── start_demo.py
│   ├── demo_automation.py
│   ├── area_planner_integration.py
│   ├── qgc_area_planner_bridge.py
│   ├── demo_area_planner_complete.py
│   ├── quick_start.bat
│   └── quick_start.sh
├── manager/                    # Web management interface
│   ├── app.py
│   └── templates/
├── missions/                   # Generated mission files
├── logs/                       # SITL and application logs
└── data/                       # Data logging and analysis
```

## 🌐 Available Services

### Core Services
- **sitl-copter**: ArduPilot Copter SITL (UDP:14550)
- **sitl-plane**: ArduPilot Plane SITL (UDP:14552)
- **sitl-rover**: ArduPilot Rover SITL (UDP:14554)
- **mavproxy**: MAVProxy for advanced control (Web:8080)

### Management Services
- **sitl-manager**: Web interface for SITL management (Web:8082)
- **data-logger**: Telemetry and mission data logging
- **performance-monitor**: System resource monitoring (Web:8083)

### AreaPlanner Integration Services
- **area-planner-integration**: AreaPlanner workflow demo (Web:8084)
- **qgc-area-planner-bridge**: QGC mission bridge (Web:8085)
- **complete-area-planner-demo**: Complete demo orchestration (Web:8086)

## 🔍 Troubleshooting

### Common Issues
1. **Port conflicts**: Ensure ports 14550-14555, 8080-8086 are available
2. **Docker not running**: Start Docker Desktop/daemon
3. **Permission issues**: Run with appropriate Docker permissions
4. **Network issues**: Check Docker network configuration
5. **Windows/macOS routing to host**: We use `host.docker.internal` for UDP back to host. On Linux, set `SITL_TX_HOST` to your host/bridge IP (e.g., `172.17.0.1`) and restart services.

### Debug Commands
```bash
# Check service status
docker compose ps  # or: docker-compose ps

# View service logs
docker compose logs -f [service-name]  # or: docker-compose logs -f [service-name]

# Check network connectivity
docker compose exec sitl-copter ping sitl-plane  # or: docker-compose exec sitl-copter ping sitl-plane

# Test MAVLink connection
python scripts/start_demo.py check

# Verify UDP sockets are bound
netstat -anu | grep 1455
```

## 📊 Monitoring

### Web Interfaces
- **SITL Manager**: Service status and control
- **MAVProxy**: Real-time telemetry and control
- **Performance Monitor**: System resource usage
- **AreaPlanner Integration**: Mission generation and execution

### Log Files
- **SITL Logs**: `logs/sitl-*.log`
- **Application Logs**: `logs/app-*.log`
- **Mission Files**: `missions/*.json`, `missions/*.plan`

## 🚀 Advanced Usage

### Custom Missions
```bash
# Create custom mission
python scripts/area_planner_integration.py --demo interactive

# Upload QGC mission
python scripts/qgc_area_planner_bridge.py --demo workflow
```

### Multi-Vehicle Operations
```bash
# Start multiple vehicles
docker compose up sitl-copter sitl-plane -d  # or: docker-compose up ...

# Coordinate missions
python scripts/demo_automation.py --demo multi
```

### Performance Testing
```bash
# Monitor system resources
docker compose up performance-monitor -d  # or: docker-compose up performance-monitor -d

# Analyze mission data
python scripts/demo_automation.py --demo performance
```

## 📚 Documentation

### AreaPlanner Integration
- **Mission Generation**: Grid-based waypoint generation
- **QGC Compatibility**: Parse and upload QGC mission files
- **Real-time Monitoring**: Mission progress tracking
- **Automated Workflows**: Complete demo orchestration

### SITL Configuration
- **Vehicle Types**: Copter, Plane, Rover support
- **Parameter Files**: Optimized for demonstration
- **Network Configuration**: MAVLink UDP/TCP support
- **Logging**: Comprehensive telemetry logging

### Web Management
- **Service Control**: Start/stop/restart services
- **Log Viewing**: Real-time log monitoring
- **Configuration**: Parameter file management
- **Status Monitoring**: Service health checks

## 🤝 Contributing

### Development Setup
```bash
# Install dependencies
pip install -r scripts/requirements.txt

# Run tests
python -m pytest scripts/tests/

# Format code
black scripts/
```

### Adding New Features
1. Create new script in `scripts/`
2. Add service to `docker-compose.yml`
3. Update documentation
4. Test with different vehicle types

## 📄 License

This project is licensed under the same terms as ArduPilot and QGroundControl.

---

**Happy Flying! 🚁✈️🚗** 