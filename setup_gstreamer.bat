@echo off
REM GStreamer Setup Script for QGroundControl
REM This script helps set up the environment for building QGroundControl with GStreamer support

echo Setting up GStreamer environment for QGroundControl...

REM Check if GStreamer is installed
if exist "C:\gstreamer\1.0\msvc_x86_64" (
    echo Found GStreamer at: C:\gstreamer\1.0\msvc_x86_64
    set GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\gstreamer\1.0\msvc_x86_64
    set GSTREAMER_PATH=C:\gstreamer\1.0\msvc_x86_64
) else if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64" (
    echo Found GStreamer at: C:\Program Files\gstreamer\1.0\msvc_x86_64
    set GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\Program Files\gstreamer\1.0\msvc_x86_64
    set GSTREAMER_PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64
) else (
    echo GStreamer not found in standard locations!
    echo Please install GStreamer from: https://gstreamer.freedesktop.org/download/
    echo Make sure to install the MSVC x86_64 version.
    pause
    exit /b 1
)

echo Set GSTREAMER_1_0_ROOT_MSVC_X86_64 to: %GSTREAMER_1_0_ROOT_MSVC_X86_64%

REM Verify required directories exist
if exist "%GSTREAMER_PATH%\lib" (
    echo ✓ Found lib directory
) else (
    echo ✗ Missing lib directory
    pause
    exit /b 1
)

if exist "%GSTREAMER_PATH%\include" (
    echo ✓ Found include directory
) else (
    echo ✗ Missing include directory
    pause
    exit /b 1
)

if exist "%GSTREAMER_PATH%\bin" (
    echo ✓ Found bin directory
) else (
    echo ✗ Missing bin directory
    pause
    exit /b 1
)

REM Check for gstreamer-1.0 plugin directory
if exist "%GSTREAMER_PATH%\lib\gstreamer-1.0" (
    echo ✓ Found gstreamer-1.0 plugin directory
) else (
    echo ✗ Missing gstreamer-1.0 plugin directory
    pause
    exit /b 1
)

REM Check for pkg-config
if exist "%GSTREAMER_PATH%\bin\pkg-config.exe" (
    echo ✓ Found pkg-config.exe
) else (
    echo ✗ Missing pkg-config.exe
    pause
    exit /b 1
)

echo.
echo GStreamer environment setup complete!
echo You can now run CMake configuration.
echo Example: cmake .. -G "Visual Studio 17 2022" -A x64
pause 