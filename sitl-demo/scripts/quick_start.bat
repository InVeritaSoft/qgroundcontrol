@echo off
REM ArduPilot SITL Demo Quick Start Script for Windows
REM This script provides easy access to common SITL demo commands

echo.
echo ========================================
echo   ArduPilot SITL Demo Quick Start
echo ========================================
echo.

:menu
echo Available commands:
echo.
echo 1. Start all SITL services
echo 2. Start specific vehicle (copter/plane/rover)
echo 3. Stop all services
echo 4. Show status
echo 5. Check connection
echo 6. Run demo automation
echo 7. Run AreaPlanner demo
echo 8. Open web manager
echo 9. View logs
echo 10. Exit
echo.

set /p choice="Enter your choice (1-10): "

if "%choice%"=="1" goto start_all
if "%choice%"=="2" goto start_specific
if "%choice%"=="3" goto stop_all
if "%choice%"=="4" goto show_status
if "%choice%"=="5" goto check_connection
if "%choice%"=="6" goto run_demo
if "%choice%"=="7" goto run_area_planner
if "%choice%"=="8" goto open_web
if "%choice%"=="9" goto view_logs
if "%choice%"=="10" goto exit
goto menu

:start_all
echo.
echo Starting all SITL services...
python scripts\start_demo.py start
echo.
pause
goto menu

:start_specific
echo.
set /p vehicle="Enter vehicle type (copter/plane/rover): "
echo Starting %vehicle% SITL service...
python scripts\start_demo.py start --vehicle %vehicle%
echo.
pause
goto menu

:stop_all
echo.
echo Stopping all SITL services...
python scripts\start_demo.py stop
echo.
pause
goto menu

:show_status
echo.
echo Showing SITL status...
python scripts\start_demo.py status
echo.
pause
goto menu

:check_connection
echo.
echo Checking SITL connection...
python scripts\start_demo.py check
echo.
pause
goto menu

:run_demo
echo.
echo Running demo automation...
python scripts\demo_automation.py --demo full
echo.
pause
goto menu

:run_area_planner
echo.
echo Running AreaPlanner demo...
python scripts\area_planner_integration.py --demo workflow
echo.
pause
goto menu

:open_web
echo.
echo Opening web manager...
start http://localhost:8082
echo Web manager opened at http://localhost:8082
echo.
pause
goto menu

:view_logs
echo.
echo Viewing recent logs...
python scripts\start_demo.py logs --lines 50
echo.
pause
goto menu

:exit
echo.
echo Thank you for using ArduPilot SITL Demo!
echo.
exit 