import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton, QLabel, QMessageBox
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtCore import QUrl
import os
import logging
from geopy.distance import geodesic
from PyQt5.QtWidgets import QDoubleSpinBox, QHBoxLayout
from PyQt5.QtWebChannel import QWebChannel
from PyQt5.QtCore import pyqtSlot, QObject

# Optional: Use dronekit for MAVLink connection
try:
    from dronekit import connect, VehicleMode
    DRONEKIT_AVAILABLE = True
except ImportError:
    DRONEKIT_AVAILABLE = False

CENTER_LAT = 49.82824897481479
CENTER_LON = 24.033390804256005
CONNECTION_STRING = "tcp:127.0.0.1:5763"

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s: %(message)s')

class MissionGUI(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Mission Planner UI')
        self.vehicle = None
        self.home_location = [CENTER_LAT, CENTER_LON]  # Home/precise location
        self.square_center = [CENTER_LAT, CENTER_LON]
        self.square_width = 30.0
        self.square_height = 90.0
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        self.status_label = QLabel('Status: Disconnected')
        layout.addWidget(self.status_label)

        # Square controls
        square_layout = QHBoxLayout()
        square_layout.addWidget(QLabel('Width (m):'))
        self.width_spin = QDoubleSpinBox()
        self.width_spin.setRange(1, 100)
        self.width_spin.setValue(30.0)
        self.width_spin.setSingleStep(0.1)
        self.width_spin.valueChanged.connect(self.update_square)
        square_layout.addWidget(self.width_spin)
        square_layout.addWidget(QLabel('Height (m):'))
        self.height_spin = QDoubleSpinBox()
        self.height_spin.setRange(1, 100)
        self.height_spin.setValue(90.0)
        self.height_spin.setSingleStep(0.1)
        self.height_spin.valueChanged.connect(self.update_square)
        square_layout.addWidget(self.height_spin)
        # Add controls for line spacing, number of points, and point spacing
        self.line_spacing_label = QLabel('Line Spacing (m):')
        self.line_spacing_spin = QDoubleSpinBox()
        self.line_spacing_spin.setRange(0.1, 10)
        self.line_spacing_spin.setValue(3.0)
        self.line_spacing_spin.setSingleStep(0.1)
        self.line_spacing_spin.valueChanged.connect(self.update_square)
        square_layout.addWidget(self.line_spacing_label)
        square_layout.addWidget(self.line_spacing_spin)
        self.num_points_label = QLabel('Number of Points:')
        from PyQt5.QtWidgets import QSpinBox
        self.num_points_spin = QSpinBox()
        self.num_points_spin.setRange(1, 50)
        self.num_points_spin.setValue(1)
        self.num_points_spin.setSingleStep(1)
        self.num_points_spin.valueChanged.connect(self.update_square)
        square_layout.addWidget(self.num_points_label)
        square_layout.addWidget(self.num_points_spin)
        layout.addLayout(square_layout)

        move_layout = QHBoxLayout()
        self.move_n_btn = QPushButton('↑')
        self.move_n_btn.clicked.connect(lambda: self.move_square('N'))
        move_layout.addWidget(self.move_n_btn)
        self.move_s_btn = QPushButton('↓')
        self.move_s_btn.clicked.connect(lambda: self.move_square('S'))
        move_layout.addWidget(self.move_s_btn)
        self.move_e_btn = QPushButton('→')
        self.move_e_btn.clicked.connect(lambda: self.move_square('E'))
        move_layout.addWidget(self.move_e_btn)
        self.move_w_btn = QPushButton('←')
        self.move_w_btn.clicked.connect(lambda: self.move_square('W'))
        move_layout.addWidget(self.move_w_btn)
        layout.addLayout(move_layout)

        self.connect_btn = QPushButton('Connect Drone')
        self.connect_btn.clicked.connect(self.connect_drone)
        layout.addWidget(self.connect_btn)

        self.takeoff_btn = QPushButton('Takeoff')
        self.takeoff_btn.clicked.connect(self.takeoff)
        self.takeoff_btn.setEnabled(False)
        layout.addWidget(self.takeoff_btn)

        self.mission_btn = QPushButton('Start Mission')
        self.mission_btn.clicked.connect(self.start_mission)
        self.mission_btn.setEnabled(False)
        layout.addWidget(self.mission_btn)

        self.write_btn = QPushButton('Write Mission Files')
        self.write_btn.clicked.connect(self.write_files)
        layout.addWidget(self.write_btn)

        # Map
        self.map_view = QWebEngineView()
        self.generate_map()
        layout.addWidget(self.map_view)

        self.location_btn = QPushButton('Get Precise Location')
        self.location_btn.clicked.connect(self.get_precise_location)
        layout.addWidget(self.location_btn)

        self.setLayout(layout)

    def update_square(self):
        logging.info(f'Width changed to {self.width_spin.value()}, Height changed to {self.height_spin.value()}')
        self.square_width = self.width_spin.value()
        self.square_height = self.height_spin.value()
        self.line_spacing = self.line_spacing_spin.value()
        self.num_points = self.num_points_spin.value()
        self.generate_map()

    def move_square(self, direction):
        logging.info(f'Move square: {direction}')
        step = 0.5  # meters per click
        lat, lon = self.square_center
        if direction == 'N':
            lat, lon = geodesic(meters=step).destination((lat, lon), 0).latitude, geodesic(meters=step).destination((lat, lon), 0).longitude
        elif direction == 'S':
            lat, lon = geodesic(meters=step).destination((lat, lon), 180).latitude, geodesic(meters=step).destination((lat, lon), 180).longitude
        elif direction == 'E':
            lat, lon = geodesic(meters=step).destination((lat, lon), 90).latitude, geodesic(meters=step).destination((lat, lon), 90).longitude
        elif direction == 'W':
            lat, lon = geodesic(meters=step).destination((lat, lon), 270).latitude, geodesic(meters=step).destination((lat, lon), 270).longitude
        logging.info(f'New square center: ({lat}, {lon})')
        self.square_center = [lat, lon]
        self.generate_map()

    def generate_map(self):
        lat, lon = self.square_center
        w, h = self.square_width, self.square_height
        home_lat, home_lon = self.home_location
        line_spacing = getattr(self, 'line_spacing', 1.0)
        num_points = getattr(self, 'num_points', 3)
        logging.info(f'Generating map: square_center=({lat}, {lon}), width={w}, height={h}, home=({home_lat}, {home_lon}), line_spacing={line_spacing}, num_points={num_points}')
        # Calculate true rectangle corners (clockwise from NW)
        north = geodesic(meters=h/2).destination((lat, lon), 0)
        south = geodesic(meters=h/2).destination((lat, lon), 180)
        nw = geodesic(meters=w/2).destination((north.latitude, north.longitude), 270)
        ne = geodesic(meters=w/2).destination((north.latitude, north.longitude), 90)
        se = geodesic(meters=w/2).destination((south.latitude, south.longitude), 90)
        sw = geodesic(meters=w/2).destination((south.latitude, south.longitude), 270)
        corners = [
            [nw.latitude, nw.longitude],
            [ne.latitude, ne.longitude],
            [se.latitude, se.longitude],
            [sw.latitude, sw.longitude],
        ]
        logging.info(f'Square corners: {corners}')
        # Calculate horizontal lines and points
        lines_js = ""
        points_js = ""
        n_lines = max(1, int(h // line_spacing))
        for i in range(n_lines):
            offset = (-(h/2) + (i + 0.5) * line_spacing)
            # Interpolate between north and south for line
            line_center = geodesic(meters=offset).destination((lat, lon), 180)
            start = geodesic(meters=w/2).destination((line_center.latitude, line_center.longitude), 270)
            end = geodesic(meters=w/2).destination((line_center.latitude, line_center.longitude), 90)
            lines_js += f"L.polyline([[{start.latitude},{start.longitude}],[{end.latitude},{end.longitude}]],{{color:'blue',weight:2}}).addTo(map);\n"
            # Points along the line, centered
            line_len = w
            for j in range(num_points):
                frac_p = (j + 0.5) / num_points
                pt_offset = (frac_p - 0.5) * line_len
                pt = geodesic(meters=pt_offset).destination((line_center.latitude, line_center.longitude), 90)
                points_js += f"L.circleMarker([{pt.latitude},{pt.longitude}],{{radius:5,color:'green',fill:true,fillColor:'green'}}).addTo(map);\n"
        # Add JS bridge for geolocation, no HTML button
        html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset='utf-8'>
            <title>Mission Map</title>
            <style>html, body, #map {{ height: 100%; margin: 0; }}</style>
            <link rel='stylesheet' href='https://unpkg.com/leaflet/dist/leaflet.css' />
            <script src='https://unpkg.com/leaflet/dist/leaflet.js'></script>
            <script src='qrc:///qtwebchannel/qwebchannel.js'></script>
        </head>
        <body>
            <div id='map' style='width: 100%; height: 100vh;'></div>
            <script>
                var map = L.map('map', {{ zoomControl: true, attributionControl: false }}).setView([{lat}, {lon}], 20);
                L.tileLayer('https://{{s}}.tile.openstreetmap.org/{{z}}/{{x}}/{{y}}.png', {{ maxZoom: 22 }}).addTo(map);
                var homeMarker = L.marker([{home_lat}, {home_lon}], {{icon: L.icon({{iconUrl: 'https://maps.gstatic.com/mapfiles/ms2/micons/blue-dot.png', iconSize: [32,32], iconAnchor: [16,32]}})}}).addTo(map).bindPopup('Home');
                var centerMarker = L.marker([{lat}, {lon}], {{draggable: true, icon: L.icon({{iconUrl: 'https://maps.gstatic.com/mapfiles/ms2/micons/red-dot.png', iconSize: [32,32], iconAnchor: [16,32]}})}}).addTo(map).bindPopup('Drag to move area');
                var square = null;
                var overlays = [];
                function drawSquareAndOverlays(centerLat, centerLon) {{
                    if (square) map.removeLayer(square);
                    overlays.forEach(function(o) {{ map.removeLayer(o); }});
                    overlays = [];
                    var w = {w}, h = {h};
                    var line_spacing = {line_spacing};
                    var num_points = {num_points};
                    var n_lines = Math.max(1, Math.floor(h / line_spacing));
                    // Calculate corners
                    var north = window.geodesicOffset(centerLat, centerLon, h/2, 0);
                    var south = window.geodesicOffset(centerLat, centerLon, h/2, 180);
                    var nw = window.geodesicOffset(north[0], north[1], w/2, 270);
                    var ne = window.geodesicOffset(north[0], north[1], w/2, 90);
                    var se = window.geodesicOffset(south[0], south[1], w/2, 90);
                    var sw = window.geodesicOffset(south[0], south[1], w/2, 270);
                    var corners = [nw, ne, se, sw];
                    square = L.polygon(corners, {{color: 'red', interactive: false}}).addTo(map);
                    for (var i = 0; i < n_lines; i++) {{
                        var offset = (-(h/2) + (i + 0.5) * line_spacing);
                        var line_center = window.geodesicOffset(centerLat, centerLon, offset, 180);
                        var start = window.geodesicOffset(line_center[0], line_center[1], w/2, 270);
                        var end = window.geodesicOffset(line_center[0], line_center[1], w/2, 90);
                        var line = L.polyline([start, end], {{color:'blue',weight:2, interactive: false}}).addTo(map);
                        overlays.push(line);
                        for (var j = 0; j < num_points; j++) {{
                            var frac_p = (j + 0.5) / num_points;
                            var pt_offset = (frac_p - 0.5) * w;
                            var pt = window.geodesicOffset(line_center[0], line_center[1], pt_offset, 90);
                            var marker = L.circleMarker(pt,{{radius:5,color:'green',fill:true,fillColor:'green',interactive: false}}).addTo(map);
                            overlays.push(marker);
                        }}
                    }}
                }}
                // Geodesic offset helper using Vincenty formula (approx)
                window.geodesicOffset = function(lat, lon, meters, bearing) {{
                    var R = 6378137.0;
                    var d = meters;
                    var brad = bearing * Math.PI / 180.0;
                    var lat1 = lat * Math.PI / 180.0;
                    var lon1 = lon * Math.PI / 180.0;
                    var lat2 = Math.asin(Math.sin(lat1) * Math.cos(d / R) + Math.cos(lat1) * Math.sin(d / R) * Math.cos(brad));
                    var lon2 = lon1 + Math.atan2(Math.sin(brad) * Math.sin(d / R) * Math.cos(lat1), Math.cos(d / R) - Math.sin(lat1) * Math.sin(lat2));
                    return [lat2 * 180.0 / Math.PI, lon2 * 180.0 / Math.PI];
                }};
                // Ensure overlays are drawn after map is ready
                map.whenReady(function() {{
                    drawSquareAndOverlays({lat}, {lon});
                }});
                centerMarker.on('drag', function(e) {{
                    var pos = e.target.getLatLng();
                    drawSquareAndOverlays(pos.lat, pos.lng);
                }});
                centerMarker.on('dragend', function(e) {{
                    var pos = e.target.getLatLng();
                    if (window.pyObj) window.pyObj.setAreaCenter(pos.lat, pos.lng);
                }});
                new QWebChannel(qt.webChannelTransport, function(channel) {{
                    window.pyObj = channel.objects.pyObj;
                }});
                window.getLocation = function() {{
                    if (navigator.geolocation) {{
                        navigator.geolocation.getCurrentPosition(function(pos) {{
                            var lat = pos.coords.latitude;
                            var lon = pos.coords.longitude;
                            homeMarker.setLatLng([lat, lon]);
                            map.setView([lat, lon], 20);
                            if (window.pyObj) window.pyObj.setLocation(lat, lon);
                        }}, function(err) {{
                            if (window.pyObj) window.pyObj.setLocationError(err.message);
                        }});
                    }} else {{
                        if (window.pyObj) window.pyObj.setLocationError('Geolocation not supported.');
                    }}
                }};
            </script>
        </body>
        </html>
        """
        map_path = os.path.abspath('mission_map.html')
        with open(map_path, 'w', encoding='utf-8') as f:
            f.write(html)
        self.map_view.setUrl(QUrl.fromLocalFile(map_path))
        logging.info('Map HTML updated and loaded in QWebEngineView')

    def connect_drone(self):
        logging.info('Connect Drone button clicked')
        if not DRONEKIT_AVAILABLE:
            QMessageBox.warning(self, 'Error', 'dronekit not installed!')
            logging.warning('dronekit not installed!')
            return
        try:
            self.vehicle = connect(CONNECTION_STRING, wait_ready=True)
            self.status_label.setText('Status: Connected')
            self.takeoff_btn.setEnabled(True)
            self.mission_btn.setEnabled(True)
            logging.info(f'Drone connected successfully to {CONNECTION_STRING}')
        except Exception as e:
            logging.error(f'Connection Failed: {e}')
            QMessageBox.critical(self, 'Connection Failed', str(e))

    def takeoff(self):
        logging.info('Takeoff button clicked')
        if not self.vehicle:
            logging.warning('No vehicle connected')
            return
        try:
            self.vehicle.mode = VehicleMode('GUIDED')
            self.vehicle.armed = True
            while not self.vehicle.armed:
                pass
            self.vehicle.simple_takeoff(2.0)  # 2m altitude
            self.status_label.setText('Status: Takeoff initiated')
            logging.info('Takeoff initiated')
        except Exception as e:
            logging.error(f'Takeoff Failed: {e}')
            QMessageBox.critical(self, 'Takeoff Failed', str(e))

    def start_mission(self):
        logging.info('Start Mission button clicked')
        if not self.vehicle:
            logging.warning('No vehicle connected')
            return
        try:
            # Load mission from file (mission_square.waypoints)
            # For demo, just print message
            self.status_label.setText('Status: Mission started (demo)')
            logging.info('Mission started (demo)')
            # Real implementation: upload waypoints and set AUTO mode
        except Exception as e:
            logging.error(f'Mission Failed: {e}')
            QMessageBox.critical(self, 'Mission Failed', str(e))

    def compute_area_and_waypoints(self):
        lat, lon = self.square_center
        w, h = self.square_width, self.square_height
        line_spacing = getattr(self, 'line_spacing', 1.0)
        num_points = getattr(self, 'num_points', 3)
        # Area corners (clockwise from NW)
        north = geodesic(meters=h/2).destination((lat, lon), 0)
        south = geodesic(meters=h/2).destination((lat, lon), 180)
        nw = geodesic(meters=w/2).destination((north.latitude, north.longitude), 270)
        ne = geodesic(meters=w/2).destination((north.latitude, north.longitude), 90)
        se = geodesic(meters=w/2).destination((south.latitude, south.longitude), 90)
        sw = geodesic(meters=w/2).destination((south.latitude, south.longitude), 270)
        corners = [
            [nw.latitude, nw.longitude],
            [ne.latitude, ne.longitude],
            [se.latitude, se.longitude],
            [sw.latitude, sw.longitude],
        ]
        # Waypoints (points on lines)
        waypoints = []
        n_lines = max(1, int(h // line_spacing))
        for i in range(n_lines):
            offset = (-(h/2) + (i + 0.5) * line_spacing)
            line_center = geodesic(meters=offset).destination((lat, lon), 180)
            line_len = w
            for j in range(num_points):
                frac_p = (j + 0.5) / num_points
                pt_offset = (frac_p - 0.5) * line_len
                pt = geodesic(meters=pt_offset).destination((line_center.latitude, line_center.longitude), 90)
                waypoints.append([pt.latitude, pt.longitude])
        return corners, waypoints

    def write_files(self):
        logging.info('Write Mission Files button clicked')
        # Compute area and waypoints from current GUI state
        corners, waypoints = self.compute_area_and_waypoints()
        # Ensure polygon is closed (first == last)
        polygon = corners + [corners[0]]
        from main import ALTITUDE_M
        # Use precise location (home_location) for takeoff/RTL
        takeoff_lat, takeoff_lon = self.home_location
        # Write the new mission file with per-point logic
        self.write_per_point_mission('mission_square.waypoints', waypoints, ALTITUDE_M, takeoff_lat, takeoff_lon)
        # Write takeoff and land files as before
        from main import write_takeoff, write_land
        write_takeoff('takeoff.waypoints', ALTITUDE_M)  # FIX: only pass filename and altitude
        write_land('land.waypoints')
        # Optionally, write the area polygon to a file
        with open('mission_area_polygon.txt', 'w') as f:
            for lat, lon in polygon:
                f.write(f"{lat},{lon}\n")
        QMessageBox.information(self, 'Files Written', 'Mission files have been written.')
        logging.info('Mission files written')

    def write_per_point_mission(self, filename, waypoints, altitude, home_lat, home_lon):
        """
        For each point: takeoff at home, fly to point, hover 30s, return to home, repeat for all points.
        Mission Planner/QGC .waypoints format.
        """
        with open(filename, 'w') as f:
            f.write('QGC WPL 110\n')
            seq = 0
            for idx, (pt_lat, pt_lon) in enumerate(waypoints):
                # Home (dummy, required)
                f.write(f'{seq}\t1\t0\t16\t0\t0\t0\t0\t{home_lat:.7f}\t{home_lon:.7f}\t0\t1\n')
                seq += 1
                # Takeoff
                f.write(f'{seq}\t0\t3\t22\t0\t0\t0\t0\t{home_lat:.7f}\t{home_lon:.7f}\t{altitude:.2f}\t1\n')
                seq += 1
                # Fly to point (hover 1m above carriage)
                hover_alt = altitude + 1.0
                f.write(f'{seq}\t0\t3\t16\t0\t0\t0\t0\t{pt_lat:.7f}\t{pt_lon:.7f}\t{hover_alt:.2f}\t1\n')
                seq += 1
                # Loiter for 30s (MAV_CMD_NAV_LOITER_TIME = 19)
                f.write(f'{seq}\t0\t3\t19\t30\t0\t0\t0\t{pt_lat:.7f}\t{pt_lon:.7f}\t{hover_alt:.2f}\t1\n')
                seq += 1
                # Return to launch (RTL, MAV_CMD_NAV_RETURN_TO_LAUNCH = 20)
                f.write(f'{seq}\t0\t3\t20\t0\t0\t0\t0\t0\t0\t0\t1\n')
                seq += 1
            # Optionally, land at home after last point
            f.write(f'{seq}\t0\t3\t21\t0\t0\t0\t0\t{home_lat:.7f}\t{home_lon:.7f}\t0\t1\n')

    def get_precise_location(self):
        lat, lon = self.square_center
        logging.info(f'Get Precise Location button clicked. Center screen at: ({lat}, {lon})')
        self.map_view.page().runJavaScript('getLocation();')

    def setLocation(self, lat, lon):
        logging.info(f'Location set to: {lat}, {lon}')
        global CENTER_LAT, CENTER_LON
        CENTER_LAT, CENTER_LON = lat, lon
        self.home_location = [lat, lon]
        self.status_label.setText(f'Precise Location: {lat:.6f}, {lon:.6f}')
        self.generate_map()

    def setLocationError(self, msg):
        logging.error(f'Location Error: {msg}')
        QMessageBox.warning(self, 'Location Error', msg)

    def setAreaCenter(self, lat, lon):
        logging.info(f'Area center set by drag: {lat}, {lon}')
        self.square_center = [lat, lon]
        # Only update overlays and square, do not regenerate the whole map (preserve zoom)
        js = f'drawSquareAndOverlays({lat}, {lon});'
        self.map_view.page().runJavaScript(js)

    def keyPressEvent(self, event):
        if event.key() == 81:  # Qt.Key_Q
            logging.info('Q pressed, quitting app')
            self.close()

# Add JS <-> Python bridge
class JSBridge(QObject):
    def __init__(self, gui):
        super().__init__()
        self.gui = gui
    @pyqtSlot(float, float)
    def setLocation(self, lat, lon):
        self.gui.setLocation(lat, lon)
    @pyqtSlot(str)
    def setLocationError(self, msg):
        self.gui.setLocationError(msg)
    @pyqtSlot(float, float)
    def setAreaCenter(self, lat, lon):
        self.gui.setAreaCenter(lat, lon)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    gui = MissionGUI()
    # Setup JS bridge
    channel = QWebChannel()
    bridge = JSBridge(gui)
    channel.registerObject('pyObj', bridge)
    gui.map_view.page().setWebChannel(channel)
    gui.show()
    sys.exit(app.exec_())
