# Simple build script for Custom QGroundControl with Mission Area Planner
Write-Host "Building Custom QGroundControl with Mission Area Planner..." -ForegroundColor Green

# Create build directory
if (-not (Test-Path "../../build")) {
    New-Item -ItemType Directory -Path "../../build" -Force | Out-Null
    Write-Host "Created build directory" -ForegroundColor Green
}

# Configure and build
Push-Location "../../build"
try {
    Write-Host "Configuring build..." -ForegroundColor Cyan
    cmake -DCMAKE_BUILD_TYPE=Release -DQGC_CUSTOM_BUILD=ON ..
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Building..." -ForegroundColor Cyan
        cmake --build . --config Release --parallel
    } else {
        Write-Error "CMake configuration failed"
        exit 1
    }
} finally {
    Pop-Location
}

Write-Host "Build completed!" -ForegroundColor Green 