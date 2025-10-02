@echo off
echo ========================================
echo QGroundControl Build Script
echo ========================================
echo.

echo [1/4] Loading Visual Studio 2022 Community environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to load Visual Studio environment
    pause
    exit /b 1
)

echo [2/4] Checking for running QGroundControl processes...
tasklist /FI "IMAGENAME eq QGroundControl.exe" 2>NUL | find /I /N "QGroundControl.exe" >NUL
if "%ERRORLEVEL%"=="0" (
    echo WARNING: QGroundControl.exe is running. Stopping it...
    taskkill /F /IM QGroundControl.exe
    timeout /t 2 /nobreak >NUL
)

echo [3/4] Building QGroundControl with parallel compilation...
cmake --build build\qt6-Windows --config Debug --target QGroundControl --parallel 4
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo [4/4] Build completed successfully!
echo.
echo Executable location: build\qt6-Windows\Debug\QGroundControl.exe
echo.
echo To run QGroundControl, execute:
echo   build\qt6-Windows\Debug\QGroundControl.exe
echo.
pause
