#!/usr/bin/env python3
"""
SITL Manager Web Application
A web interface for managing ArduPilot SITL instances.
"""

from flask import Flask, render_template, jsonify, request, redirect, url_for
import subprocess
import json
import os
import time
from pathlib import Path

app = Flask(__name__)

class SITLManager:
    def __init__(self, demo_dir="."):
        self.demo_dir = Path(demo_dir)
        self.docker_compose_file = self.demo_dir / "docker-compose.yml"
        self.compose_cmd = self._detect_compose()

    def _detect_compose(self):
        try:
            result = subprocess.run(["docker", "compose", "version"], capture_output=True, text=True)
            if result.returncode == 0:
                return ["docker", "compose"]
        except Exception:
            pass
        return ["docker-compose"]
        
    def get_services_status(self):
        """Get status of all SITL services."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "ps", "--format", "json"]
            result = subprocess.run(cmd, cwd=self.demo_dir, capture_output=True, text=True)
            
            services = []
            for line in result.stdout.strip().split('\n'):
                if line:
                    service = json.loads(line)
                    services.append({
                        'name': service.get('Name', ''),
                        'status': service.get('State', ''),
                        'ports': service.get('Ports', ''),
                        'image': service.get('Image', '')
                    })
            
            return services
        except Exception as e:
            return []
    
    def start_service(self, service_name):
        """Start a specific service."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "up", "-d", service_name]
            subprocess.run(cmd, cwd=self.demo_dir, check=True)
            return True
        except subprocess.CalledProcessError:
            return False
    
    def stop_service(self, service_name):
        """Stop a specific service."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "stop", service_name]
            subprocess.run(cmd, cwd=self.demo_dir, check=True)
            return True
        except subprocess.CalledProcessError:
            return False
    
    def restart_service(self, service_name):
        """Restart a specific service."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "restart", service_name]
            subprocess.run(cmd, cwd=self.demo_dir, check=True)
            return True
        except subprocess.CalledProcessError:
            return False
    
    def get_service_logs(self, service_name, lines=50):
        """Get logs for a specific service."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "logs", "--tail", str(lines), service_name]
            result = subprocess.run(cmd, cwd=self.demo_dir, capture_output=True, text=True)
            return result.stdout
        except subprocess.CalledProcessError:
            return "Failed to get logs"
    
    def start_all_services(self):
        """Start all SITL services."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "up", "-d"]
            subprocess.run(cmd, cwd=self.demo_dir, check=True)
            return True
        except subprocess.CalledProcessError:
            return False
    
    def stop_all_services(self):
        """Stop all SITL services."""
        try:
            cmd = self.compose_cmd + ["-f", str(self.docker_compose_file), "down"]
            subprocess.run(cmd, cwd=self.demo_dir, check=True)
            return True
        except subprocess.CalledProcessError:
            return False

# Initialize SITL manager
sitl_manager = SITLManager()

@app.route('/')
def index():
    """Main dashboard page."""
    services = sitl_manager.get_services_status()
    return render_template('index.html', services=services)

@app.route('/api/services')
def api_services():
    """API endpoint to get services status."""
    services = sitl_manager.get_services_status()
    return jsonify(services)

@app.route('/api/service/<service_name>/start', methods=['POST'])
def api_start_service(service_name):
    """API endpoint to start a service."""
    success = sitl_manager.start_service(service_name)
    return jsonify({'success': success})

@app.route('/api/service/<service_name>/stop', methods=['POST'])
def api_stop_service(service_name):
    """API endpoint to stop a service."""
    success = sitl_manager.stop_service(service_name)
    return jsonify({'success': success})

@app.route('/api/service/<service_name>/restart', methods=['POST'])
def api_restart_service(service_name):
    """API endpoint to restart a service."""
    success = sitl_manager.restart_service(service_name)
    return jsonify({'success': success})

@app.route('/api/service/<service_name>/logs')
def api_service_logs(service_name):
    """API endpoint to get service logs."""
    lines = request.args.get('lines', 50, type=int)
    logs = sitl_manager.get_service_logs(service_name, lines)
    return jsonify({'logs': logs})

@app.route('/api/services/start-all', methods=['POST'])
def api_start_all_services():
    """API endpoint to start all services."""
    success = sitl_manager.start_all_services()
    return jsonify({'success': success})

@app.route('/api/services/stop-all', methods=['POST'])
def api_stop_all_services():
    """API endpoint to stop all services."""
    success = sitl_manager.stop_all_services()
    return jsonify({'success': success})

@app.route('/service/<service_name>')
def service_detail(service_name):
    """Service detail page."""
    services = sitl_manager.get_services_status()
    service = next((s for s in services if s['name'] == service_name), None)
    
    if not service:
        return redirect(url_for('index'))
    
    logs = sitl_manager.get_service_logs(service_name, 100)
    return render_template('service_detail.html', service=service, logs=logs)

@app.route('/logs')
def logs_view():
    """Logs view page."""
    services = sitl_manager.get_services_status()
    return render_template('logs.html', services=services)

@app.route('/config')
def config_view():
    """Configuration view page."""
    config_dir = Path("configs")
    config_files = []
    
    if config_dir.exists():
        for file in config_dir.glob("*.parm"):
            config_files.append({
                'name': file.name,
                'size': file.stat().st_size,
                'modified': time.ctime(file.stat().st_mtime)
            })
    
    return render_template('config.html', config_files=config_files)

@app.route('/config/<filename>')
def view_config_file(filename):
    """View a specific configuration file."""
    config_file = Path("configs") / filename
    
    if not config_file.exists():
        return "File not found", 404
    
    with open(config_file, 'r') as f:
        content = f.read()
    
    return render_template('config_file.html', filename=filename, content=content)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True) 