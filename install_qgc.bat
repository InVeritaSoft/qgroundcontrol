@echo off
echo ========================================
echo QGroundControl Installation Script
echo ========================================
echo.

echo [1/6] Loading Visual Studio 2022 Community environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to load Visual Studio environment
    pause
    exit /b 1
)

echo [2/6] Checking for running QGroundControl processes...
tasklist /FI "IMAGENAME eq QGroundControl.exe" 2>NUL | find /I /N "QGroundControl.exe" >NUL
if "%ERRORLEVEL%"=="0" (
    echo WARNING: QGroundControl.exe is running. Stopping it...
    taskkill /F /IM QGroundControl.exe
    timeout /t 2 /nobreak >NUL
)

echo [3/6] Verifying build exists...
if not exist "build\qt6-Windows\Debug\QGroundControl.exe" (
    echo ERROR: QGroundControl.exe not found. Please run build_qgc.bat first.
    pause
    exit /b 1
)

echo [4/6] Creating installation directory...
set INSTALL_DIR=C:\Program Files\QGroundControl
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%" 2>NUL
    if errorlevel 1 (
        echo WARNING: Could not create Program Files directory. Installing to user directory...
        set INSTALL_DIR=%USERPROFILE%\QGroundControl
        mkdir "%INSTALL_DIR%" 2>NUL
    )
)

echo [5/6] Copying QGroundControl executable...
copy "build\qt6-Windows\Debug\QGroundControl.exe" "%INSTALL_DIR%\" /Y
if errorlevel 1 (
    echo ERROR: Failed to copy executable
    pause
    exit /b 1
)

echo [6/6] Creating desktop shortcut...
set DESKTOP=%USERPROFILE%\Desktop
set SHORTCUT_PATH=%DESKTOP%\QGroundControl.lnk

powershell -Command "$WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%SHORTCUT_PATH%'); $Shortcut.TargetPath = '%INSTALL_DIR%\QGroundControl.exe'; $Shortcut.WorkingDirectory = '%INSTALL_DIR%'; $Shortcut.Description = 'QGroundControl - Ground Control Station'; $Shortcut.Save()"

echo.
echo ========================================
echo Installation completed successfully!
echo ========================================
echo.
echo QGroundControl installed to: %INSTALL_DIR%
echo Desktop shortcut created: %SHORTCUT_PATH%
echo.
echo To run QGroundControl:
echo   1. Double-click the desktop shortcut, OR
echo   2. Run: "%INSTALL_DIR%\QGroundControl.exe"
echo.
echo To uninstall:
echo   1. Delete the folder: %INSTALL_DIR%
echo   2. Delete the desktop shortcut: %SHORTCUT_PATH%
echo.
pause


