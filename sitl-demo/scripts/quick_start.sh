#!/bin/bash

# ArduPilot SITL Demo Quick Start Script for Linux/macOS
# This script provides easy access to common SITL demo commands

clear
echo "========================================"
echo "  ArduPilot SITL Demo Quick Start"
echo "========================================"
echo

while true; do
    echo "Available commands:"
    echo
    echo "1. Start all SITL services"
    echo "2. Start specific vehicle (copter/plane/rover)"
    echo "3. Stop all services"
    echo "4. Show status"
    echo "5. Check connection"
    echo "6. Run demo automation"
    echo "7. Open web manager"
    echo "8. View logs"
    echo "9. Exit"
    echo

    read -p "Enter your choice (1-9): " choice

    case $choice in
        1)
            echo
            echo "Starting all SITL services..."
            python3 scripts/start_demo.py start
            echo
            read -p "Press Enter to continue..."
            ;;
        2)
            echo
            read -p "Enter vehicle type (copter/plane/rover): " vehicle
            echo "Starting $vehicle SITL service..."
            python3 scripts/start_demo.py start --vehicle $vehicle
            echo
            read -p "Press Enter to continue..."
            ;;
        3)
            echo
            echo "Stopping all SITL services..."
            python3 scripts/start_demo.py stop
            echo
            read -p "Press Enter to continue..."
            ;;
        4)
            echo
            echo "Showing SITL status..."
            python3 scripts/start_demo.py status
            echo
            read -p "Press Enter to continue..."
            ;;
        5)
            echo
            echo "Checking SITL connection..."
            python3 scripts/start_demo.py check
            echo
            read -p "Press Enter to continue..."
            ;;
        6)
            echo
            echo "Running demo automation..."
            python3 scripts/demo_automation.py --demo full
            echo
            read -p "Press Enter to continue..."
            ;;
        7)
            echo
            echo "Opening web manager..."
            if command -v xdg-open > /dev/null; then
                xdg-open http://localhost:8082
            elif command -v open > /dev/null; then
                open http://localhost:8082
            else
                echo "Please open http://localhost:8082 in your browser"
            fi
            echo "Web manager opened at http://localhost:8082"
            echo
            read -p "Press Enter to continue..."
            ;;
        8)
            echo
            echo "Viewing recent logs..."
            python3 scripts/start_demo.py logs --lines 50
            echo
            read -p "Press Enter to continue..."
            ;;
        9)
            echo
            echo "Thank you for using ArduPilot SITL Demo!"
            echo
            exit 0
            ;;
        *)
            echo "Invalid choice. Please try again."
            echo
            read -p "Press Enter to continue..."
            ;;
    esac
    
    clear
    echo "========================================"
    echo "  ArduPilot SITL Demo Quick Start"
    echo "========================================"
    echo
done 