# GStreamer Setup Script for QGroundControl
# This script helps set up the environment for building QGroundControl with GStreamer support

Write-Host "Setting up GStreamer environment for QGroundControl..." -ForegroundColor Green

# Check if GStreamer is installed
$gstreamerPaths = @(
    "C:\gstreamer\1.0\msvc_x86_64",
    "C:\Program Files\gstreamer\1.0\msvc_x86_64"
)

$gstreamerFound = $false
$gstreamerPath = ""

foreach ($path in $gstreamerPaths) {
    if (Test-Path $path) {
        Write-Host "Found GStreamer at: $path" -ForegroundColor Green
        $gstreamerFound = $true
        $gstreamerPath = $path
        break
    }
}

if (-not $gstreamerFound) {
    Write-Host "GStreamer not found in standard locations!" -ForegroundColor Red
    Write-Host "Please install GStreamer from: https://gstreamer.freedesktop.org/download/" -ForegroundColor Yellow
    Write-Host "Make sure to install the MSVC x86_64 version." -ForegroundColor Yellow
    exit 1
}

# Set environment variables
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = $gstreamerPath
Write-Host "Set GSTREAMER_1_0_ROOT_MSVC_X86_64 to: $gstreamerPath" -ForegroundColor Green

# Verify required directories exist
$requiredDirs = @("lib", "include", "bin")
foreach ($dir in $requiredDirs) {
    $fullPath = Join-Path $gstreamerPath $dir
    if (Test-Path $fullPath) {
        Write-Host "✓ Found $dir directory" -ForegroundColor Green
    } else {
        Write-Host "✗ Missing $dir directory at: $fullPath" -ForegroundColor Red
        exit 1
    }
}

# Check for gstreamer-1.0 plugin directory
$pluginPath = Join-Path $gstreamerPath "lib\gstreamer-1.0"
if (Test-Path $pluginPath) {
    Write-Host "✓ Found gstreamer-1.0 plugin directory" -ForegroundColor Green
} else {
    Write-Host "✗ Missing gstreamer-1.0 plugin directory at: $pluginPath" -ForegroundColor Red
    exit 1
}

# Check for pkg-config
$pkgConfigPath = Join-Path $gstreamerPath "bin\pkg-config.exe"
if (Test-Path $pkgConfigPath) {
    Write-Host "✓ Found pkg-config.exe" -ForegroundColor Green
} else {
    Write-Host "✗ Missing pkg-config.exe at: $pkgConfigPath" -ForegroundColor Red
    exit 1
}

Write-Host "`nGStreamer environment setup complete!" -ForegroundColor Green
Write-Host "You can now run CMake configuration." -ForegroundColor Yellow
Write-Host "Example: cmake .. -G 'Visual Studio 17 2022' -A x64" -ForegroundColor Cyan 