#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Deploy Custom QGroundControl Build with Mission Area Planner

.DESCRIPTION
    This script packages and deploys the custom QGroundControl build that includes
    the Mission Area Planner feature. It creates a distributable package with all
    necessary dependencies and documentation.

.PARAMETER BuildType
    The build type (Debug, Release, RelWithDebInfo). Default is Release.

.PARAMETER Platform
    The target platform (Windows, Linux, macOS). Default is Windows.

.PARAMETER OutputDir
    The output directory for the deployment package. Default is ./deploy.

.EXAMPLE
    .\deploy_custom_build.ps1 -BuildType Release -Platform Windows
#>

param(
    [string]$BuildType = "Release",
    [string]$Platform = "Windows",
    [string]$OutputDir = "./deploy"
)

# Set error action preference
$ErrorActionPreference = "Stop"

# Script variables
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = "$ProjectRoot/build"
$CustomDir = "$ProjectRoot/custom"
$DeployDir = "$OutputDir"

Write-Host "=== Custom QGroundControl Build Deployment ===" -ForegroundColor Green
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
Write-Host "Output Directory: $DeployDir" -ForegroundColor Yellow
Write-Host ""

# Function to check if command exists
function Test-Command($Command) {
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

# Function to create directory if it doesn't exist
function New-DirectoryIfNotExists($Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        Write-Host "Created directory: $Path" -ForegroundColor Green
    }
}

# Function to copy files with progress
function Copy-FilesWithProgress($Source, $Destination, $Description) {
    Write-Host "Copying $Description..." -ForegroundColor Cyan
    if (Test-Path $Source) {
        Copy-Item -Path $Source -Destination $Destination -Recurse -Force
        Write-Host "✓ $Description copied successfully" -ForegroundColor Green
    } else {
        Write-Warning "Source not found: $Source"
    }
}

# Check prerequisites
Write-Host "Checking prerequisites..." -ForegroundColor Cyan

# Check for CMake
if (-not (Test-Command "cmake")) {
    Write-Error "CMake not found. Please install CMake and add it to your PATH."
    exit 1
}

# Check for Qt
if (-not (Test-Command "qmake")) {
    Write-Warning "Qt qmake not found. Make sure Qt is installed and in your PATH."
}

# Check for Visual Studio (Windows)
if ($Platform -eq "Windows" -and -not (Test-Command "msbuild")) {
    Write-Warning "MSBuild not found. Make sure Visual Studio is installed."
}

Write-Host "✓ Prerequisites check completed" -ForegroundColor Green
Write-Host ""

# Create deployment directory structure
Write-Host "Creating deployment directory structure..." -ForegroundColor Cyan
New-DirectoryIfNotExists $DeployDir
New-DirectoryIfNotExists "$DeployDir/custom"
New-DirectoryIfNotExists "$DeployDir/docs"
New-DirectoryIfNotExists "$DeployDir/build"
New-DirectoryIfNotExists "$DeployDir/scripts"
Write-Host "✓ Directory structure created" -ForegroundColor Green
Write-Host ""

# Copy custom build files
Write-Host "Copying custom build files..." -ForegroundColor Cyan

# Copy source files
Copy-FilesWithProgress "$CustomDir/src" "$DeployDir/custom/src" "C++ source files"
Copy-FilesWithProgress "$CustomDir/res" "$DeployDir/custom/res" "QML resources"
Copy-FilesWithProgress "$CustomDir/cmake" "$DeployDir/custom/cmake" "CMake configuration"

# Copy build configuration files
Copy-FilesWithProgress "$CustomDir/CMakeLists.txt" "$DeployDir/custom/" "CMakeLists.txt"
Copy-FilesWithProgress "$CustomDir/custom.qrc" "$DeployDir/custom/" "custom.qrc"

# Copy deployment assets
if (Test-Path "$CustomDir/deploy") {
    Copy-FilesWithProgress "$CustomDir/deploy" "$DeployDir/custom/deploy" "deployment assets"
}

Write-Host "✓ Custom build files copied" -ForegroundColor Green
Write-Host ""

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Cyan

# Copy README files
if (Test-Path "$CustomDir/README.md") {
    Copy-FilesWithProgress "$CustomDir/README.md" "$DeployDir/docs/" "README.md"
}

if (Test-Path "$CustomDir/MISSION_AREA_PLANNER_README.md") {
    Copy-FilesWithProgress "$CustomDir/MISSION_AREA_PLANNER_README.md" "$DeployDir/docs/" "Mission Area Planner README"
}

if (Test-Path "$CustomDir/MISSION_AREA_PLANNER_TEST_REPORT.md") {
    Copy-FilesWithProgress "$CustomDir/MISSION_AREA_PLANNER_TEST_REPORT.md" "$DeployDir/docs/" "Test Report"
}

# Copy test files
if (Test-Path "$CustomDir/test_geodesic.py") {
    Copy-FilesWithProgress "$CustomDir/test_geodesic.py" "$DeployDir/scripts/" "Geodesic test script"
}

Write-Host "✓ Documentation copied" -ForegroundColor Green
Write-Host ""

# Create build script
Write-Host "Creating build script..." -ForegroundColor Cyan

$BuildScript = @"
#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build Custom QGroundControl with Mission Area Planner

.DESCRIPTION
    This script builds the custom QGroundControl with Mission Area Planner feature.

.PARAMETER BuildType
    The build type (Debug, Release, RelWithDebInfo). Default is Release.

.PARAMETER Platform
    The target platform (Windows, Linux, macOS). Default is Windows.

.EXAMPLE
    .\build_custom_qgc.ps1 -BuildType Release -Platform Windows
#>

param(
    [string]`$BuildType = "Release",
    [string]`$Platform = "Windows"
)

# Set error action preference
`$ErrorActionPreference = "Stop"

# Script variables
`$ScriptDir = Split-Path -Parent `$MyInvocation.MyCommand.Path
`$ProjectRoot = Split-Path -Parent `$ScriptDir
`$BuildDir = "`$ProjectRoot/build"

Write-Host "=== Building Custom QGroundControl ===" -ForegroundColor Green
Write-Host "Build Type: `$BuildType" -ForegroundColor Yellow
Write-Host "Platform: `$Platform" -ForegroundColor Yellow
Write-Host ""

# Create build directory
if (-not (Test-Path `$BuildDir)) {
    New-Item -ItemType Directory -Path `$BuildDir -Force | Out-Null
    Write-Host "Created build directory: `$BuildDir" -ForegroundColor Green
}

# Configure build
Write-Host "Configuring build..." -ForegroundColor Cyan
Push-Location `$BuildDir

try {
    # Configure with CMake
    `$CmakeArgs = @(
        "-DCMAKE_BUILD_TYPE=`$BuildType",
        "-DQGC_CUSTOM_BUILD=ON",
        "`$ProjectRoot"
    )
    
    & cmake @CmakeArgs
    if (`$LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Host "✓ Build configured successfully" -ForegroundColor Green
    
    # Build
    Write-Host "Building..." -ForegroundColor Cyan
    
    if (`$Platform -eq "Windows") {
        & cmake --build . --config `$BuildType --parallel
    } else {
        & cmake --build . --config `$BuildType -j`$(nproc)
    }
    
    if (`$LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Host "✓ Build completed successfully" -ForegroundColor Green
    
} finally {
    Pop-Location
}

Write-Host ""
Write-Host "Build completed! QGroundControl executable should be in: `$BuildDir" -ForegroundColor Green
"@

$BuildScript | Out-File -FilePath "$DeployDir/scripts/build_custom_qgc.ps1" -Encoding UTF8
Write-Host "✓ Build script created" -ForegroundColor Green

# Create installation script
Write-Host "Creating installation script..." -ForegroundColor Cyan

$InstallScript = @"
#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Install Custom QGroundControl with Mission Area Planner

.DESCRIPTION
    This script installs the custom QGroundControl build with Mission Area Planner feature.

.PARAMETER InstallDir
    The installation directory. Default is C:\Program Files\QGroundControl-Custom.

.EXAMPLE
    .\install_custom_qgc.ps1 -InstallDir "C:\QGroundControl-Custom"
#>

param(
    [string]`$InstallDir = "C:\Program Files\QGroundControl-Custom"
)

# Set error action preference
`$ErrorActionPreference = "Stop"

Write-Host "=== Installing Custom QGroundControl ===" -ForegroundColor Green
Write-Host "Installation Directory: `$InstallDir" -ForegroundColor Yellow
Write-Host ""

# Check if running as administrator (required for Program Files)
if (`$Platform -eq "Windows" -and `$InstallDir -like "*Program Files*") {
    `$IsAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
    if (-not `$IsAdmin) {
        Write-Error "Administrator privileges required for installation to Program Files"
        exit 1
    }
}

# Create installation directory
if (-not (Test-Path `$InstallDir)) {
    New-Item -ItemType Directory -Path `$InstallDir -Force | Out-Null
    Write-Host "Created installation directory: `$InstallDir" -ForegroundColor Green
}

# Copy files
Write-Host "Copying files..." -ForegroundColor Cyan

`$ScriptDir = Split-Path -Parent `$MyInvocation.MyCommand.Path
`$DeployDir = Split-Path -Parent `$ScriptDir

# Copy executable and libraries
`$BuildDir = "`$DeployDir/../build"
if (Test-Path "`$BuildDir/`$BuildType") {
    Copy-Item -Path "`$BuildDir/`$BuildType/*" -Destination `$InstallDir -Recurse -Force
    Write-Host "✓ Executable and libraries copied" -ForegroundColor Green
} else {
    Write-Warning "Build directory not found. Please build the project first."
}

# Copy documentation
`$DocsDir = "`$InstallDir/docs"
if (-not (Test-Path `$DocsDir)) {
    New-Item -ItemType Directory -Path `$DocsDir -Force | Out-Null
}

if (Test-Path "`$DeployDir/docs") {
    Copy-Item -Path "`$DeployDir/docs/*" -Destination `$DocsDir -Recurse -Force
    Write-Host "✓ Documentation copied" -ForegroundColor Green
}

# Create desktop shortcut (Windows)
if (`$Platform -eq "Windows") {
    `$Desktop = [Environment]::GetFolderPath("Desktop")
    `$ShortcutPath = "`$Desktop\QGroundControl-Custom.lnk"
    
    `$WshShell = New-Object -ComObject WScript.Shell
    `$Shortcut = `$WshShell.CreateShortcut(`$ShortcutPath)
    `$Shortcut.TargetPath = "`$InstallDir\QGroundControl.exe"
    `$Shortcut.WorkingDirectory = `$InstallDir
    `$Shortcut.Description = "QGroundControl Custom Build with Mission Area Planner"
    `$Shortcut.Save()
    
    Write-Host "✓ Desktop shortcut created" -ForegroundColor Green
}

Write-Host ""
Write-Host "Installation completed!" -ForegroundColor Green
Write-Host "QGroundControl Custom Build is installed at: `$InstallDir" -ForegroundColor Yellow
Write-Host "Documentation is available at: `$DocsDir" -ForegroundColor Yellow
"@

$InstallScript | Out-File -FilePath "$DeployDir/scripts/install_custom_qgc.ps1" -Encoding UTF8
Write-Host "✓ Installation script created" -ForegroundColor Green

# Create deployment manifest
Write-Host "Creating deployment manifest..." -ForegroundColor Cyan

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
    files = @{
        source = "custom/src/"
        resources = "custom/res/"
        documentation = "docs/"
        scripts = "scripts/"
        build = "build/"
    }
}

$Manifest | ConvertTo-Json -Depth 10 | Out-File -FilePath "$DeployDir/deployment_manifest.json" -Encoding UTF8
Write-Host "✓ Deployment manifest created" -ForegroundColor Green

# Create README for deployment package
Write-Host "Creating deployment README..." -ForegroundColor Cyan

$DeployReadme = @"
# QGroundControl Custom Build - Mission Area Planner

## Overview
This package contains a custom build of QGroundControl with the Mission Area Planner feature integrated.

## Contents
- **custom/**: Custom build source files and resources
- **docs/**: Documentation and test reports
- **scripts/**: Build and installation scripts
- **build/**: Build configuration and output
- **deployment_manifest.json**: Package metadata

## Quick Start

### Prerequisites
- CMake 3.25 or higher
- Qt6 (Core, Location, Positioning modules)
- C++ compiler (Visual Studio 2019+ on Windows, GCC 9+ on Linux, Xcode 12+ on macOS)

### Building
1. Navigate to the scripts directory
2. Run the build script:
   ```powershell
   .\scripts\build_custom_qgc.ps1 -BuildType Release -Platform Windows
   ```

### Installation
1. Run the installation script:
   ```powershell
   .\scripts\install_custom_qgc.ps1 -InstallDir "C:\QGroundControl-Custom"
   ```

## Features
- **Mission Area Planner**: Define rectangular mission areas with configurable parameters
- **Geodesic Calculations**: Accurate distance and coordinate calculations
- **Grid Generation**: Automatic waypoint generation for systematic coverage
- **Map Integration**: Real-time visualization on QGC map
- **QGC Integration**: Seamless integration with existing QGC workflow

## Documentation
- **MISSION_AREA_PLANNER_README.md**: Detailed feature documentation
- **MISSION_AREA_PLANNER_TEST_REPORT.md**: Comprehensive test results
- **test_geodesic.py**: Geodesic calculation validation script

## Support
For issues and questions, please refer to the documentation or contact the development team.

## License
This custom build is based on QGroundControl and follows the same licensing terms.
"@

$DeployReadme | Out-File -FilePath "$DeployDir/README.md" -Encoding UTF8
Write-Host "✓ Deployment README created" -ForegroundColor Green

# Create deployment package
Write-Host "Creating deployment package..." -ForegroundColor Cyan

$PackageName = "QGroundControl-Custom-MissionAreaPlanner-$Platform-$BuildType-$(Get-Date -Format 'yyyyMMdd')"
$PackagePath = "$DeployDir/$PackageName.zip"

# Create ZIP package
if (Test-Command "Compress-Archive") {
    Compress-Archive -Path "$DeployDir/*" -DestinationPath $PackagePath -Force
    Write-Host "✓ Deployment package created: $PackagePath" -ForegroundColor Green
} else {
    Write-Warning "Compress-Archive not available. Please manually create a ZIP file from: $DeployDir"
}

Write-Host ""
Write-Host "=== Deployment Completed Successfully ===" -ForegroundColor Green
Write-Host "Deployment package: $PackagePath" -ForegroundColor Yellow
Write-Host "Deployment directory: $DeployDir" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Test the build script: .\scripts\build_custom_qgc.ps1" -ForegroundColor White
Write-Host "2. Install the custom build: .\scripts\install_custom_qgc.ps1" -ForegroundColor White
Write-Host "3. Verify Mission Area Planner functionality" -ForegroundColor White
Write-Host "" 