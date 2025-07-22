#!/usr/bin/env pwsh

# Simple deployment package creation script
param(
    [string]$BuildType = "Release",
    [string]$Platform = "Windows",
    [string]$OutputDir = "./deploy"
)

Write-Host "=== Creating Custom QGroundControl Deployment Package ===" -ForegroundColor Green
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
Write-Host "Output Directory: $OutputDir" -ForegroundColor Yellow
Write-Host ""

# Create deployment directory
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    Write-Host "Created deployment directory: $OutputDir" -ForegroundColor Green
}

# Create subdirectories
$SubDirs = @("custom", "docs", "scripts", "build")
foreach ($Dir in $SubDirs) {
    $Path = Join-Path $OutputDir $Dir
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        Write-Host "Created directory: $Path" -ForegroundColor Green
    }
}

# Copy custom build files
Write-Host "Copying custom build files..." -ForegroundColor Cyan

# Copy source files
if (Test-Path "src") {
    Copy-Item -Path "src" -Destination "$OutputDir/custom/" -Recurse -Force
    Write-Host "✓ C++ source files copied" -ForegroundColor Green
}

if (Test-Path "res") {
    Copy-Item -Path "res" -Destination "$OutputDir/custom/" -Recurse -Force
    Write-Host "✓ QML resources copied" -ForegroundColor Green
}

if (Test-Path "cmake") {
    Copy-Item -Path "cmake" -Destination "$OutputDir/custom/" -Recurse -Force
    Write-Host "✓ CMake configuration copied" -ForegroundColor Green
}

# Copy build configuration files
if (Test-Path "CMakeLists.txt") {
    Copy-Item -Path "CMakeLists.txt" -Destination "$OutputDir/custom/" -Force
    Write-Host "✓ CMakeLists.txt copied" -ForegroundColor Green
}

if (Test-Path "custom.qrc") {
    Copy-Item -Path "custom.qrc" -Destination "$OutputDir/custom/" -Force
    Write-Host "✓ custom.qrc copied" -ForegroundColor Green
}

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Cyan

if (Test-Path "README.md") {
    Copy-Item -Path "README.md" -Destination "$OutputDir/docs/" -Force
    Write-Host "✓ README.md copied" -ForegroundColor Green
}

if (Test-Path "MISSION_AREA_PLANNER_README.md") {
    Copy-Item -Path "MISSION_AREA_PLANNER_README.md" -Destination "$OutputDir/docs/" -Force
    Write-Host "✓ Mission Area Planner README copied" -ForegroundColor Green
}

if (Test-Path "MISSION_AREA_PLANNER_TEST_REPORT.md") {
    Copy-Item -Path "MISSION_AREA_PLANNER_TEST_REPORT.md" -Destination "$OutputDir/docs/" -Force
    Write-Host "✓ Test Report copied" -ForegroundColor Green
}

# Copy test files
if (Test-Path "test_geodesic.py") {
    Copy-Item -Path "test_geodesic.py" -Destination "$OutputDir/scripts/" -Force
    Write-Host "✓ Geodesic test script copied" -ForegroundColor Green
}

# Create simple build script
$BuildScript = @"
# Simple build script for Custom QGroundControl
Write-Host "Building Custom QGroundControl with Mission Area Planner..."

# Create build directory
if (-not (Test-Path "../build")) {
    New-Item -ItemType Directory -Path "../build" -Force | Out-Null
}

# Configure and build
Push-Location "../build"
try {
    cmake -DCMAKE_BUILD_TYPE=$BuildType -DQGC_CUSTOM_BUILD=ON ..
    if (`$LASTEXITCODE -eq 0) {
        cmake --build . --config $BuildType --parallel
    }
} finally {
    Pop-Location
}

Write-Host "Build completed!"
"@

$BuildScript | Out-File -FilePath "$OutputDir/scripts/build.ps1" -Encoding UTF8
Write-Host "✓ Build script created" -ForegroundColor Green

# Create deployment manifest
$Manifest = @{
    name = "QGroundControl-Custom-MissionAreaPlanner"
    version = "1.0.0"
    description = "Custom QGroundControl build with Mission Area Planner feature"
    platform = $Platform
    buildType = $BuildType
    buildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    components = @(
        "Mission Area Planner QML Components",
        "Geodesic Calculation Engine", 
        "Custom Resource Override System",
        "Plan View Integration",
        "Map Overlay Visualization"
    )
    dependencies = @(
        "Qt6 Core",
        "Qt6 Location", 
        "Qt6 Positioning",
        "CMake 3.25+",
        "C++ Compiler"
    )
}

$Manifest | ConvertTo-Json -Depth 10 | Out-File -FilePath "$OutputDir/deployment_manifest.json" -Encoding UTF8
Write-Host "✓ Deployment manifest created" -ForegroundColor Green

# Create package README
$PackageReadme = @"
# QGroundControl Custom Build - Mission Area Planner

## Overview
Custom QGroundControl build with Mission Area Planner feature integrated.

## Contents
- **custom/**: Source files and resources
- **docs/**: Documentation and test reports  
- **scripts/**: Build scripts
- **deployment_manifest.json**: Package metadata

## Building
1. Navigate to scripts directory
2. Run: `powershell -ExecutionPolicy Bypass -File build.ps1`

## Features
- Mission Area Planner with geodesic calculations
- Grid generation for systematic coverage
- Real-time map visualization
- Seamless QGC integration

## Documentation
See docs/ directory for detailed documentation.
"@

$PackageReadme | Out-File -FilePath "$OutputDir/README.md" -Encoding UTF8
Write-Host "✓ Package README created" -ForegroundColor Green

# Create ZIP package
$PackageName = "QGroundControl-Custom-MissionAreaPlanner-$Platform-$BuildType-$(Get-Date -Format 'yyyyMMdd')"
$PackagePath = "$OutputDir/$PackageName.zip"

try {
    Compress-Archive -Path "$OutputDir/*" -DestinationPath $PackagePath -Force
    Write-Host "✓ Deployment package created: $PackagePath" -ForegroundColor Green
} catch {
    Write-Warning "Could not create ZIP package. Files are available in: $OutputDir"
}

Write-Host ""
Write-Host "=== Deployment Package Created Successfully ===" -ForegroundColor Green
Write-Host "Package: $PackagePath" -ForegroundColor Yellow
Write-Host "Directory: $OutputDir" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Test the build script: .\scripts\build.ps1" -ForegroundColor White
Write-Host "2. Verify Mission Area Planner functionality" -ForegroundColor White
Write-Host "" 