#!/bin/bash

# Interactive Drawing Test Runner
# This script helps run tests for the AreaPlanEditor interactive drawing functionality

echo "=========================================="
echo "Interactive Drawing Functionality Tests"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results
PASSED=0
FAILED=0
SKIPPED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    
    echo -e "\n${BLUE}Running: ${test_name}${NC}"
    echo "Command: $test_command"
    
    # Run the test
    output=$(eval "$test_command" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC}"
        echo "Output: $output"
        ((FAILED++))
    fi
}

# Function to check if QGroundControl is built
check_qgc_build() {
    echo -e "\n${YELLOW}Checking QGroundControl build...${NC}"
    
    if [ -f "build/qgroundcontrol" ] || [ -f "build/QGroundControl.exe" ]; then
        echo -e "${GREEN}✓ QGroundControl build found${NC}"
        return 0
    else
        echo -e "${RED}✗ QGroundControl build not found${NC}"
        echo "Please build QGroundControl first:"
        echo "  mkdir build && cd build"
        echo "  cmake .."
        echo "  make -j$(nproc)"
        return 1
    fi
}

# Function to check if test mode is enabled
check_test_mode() {
    echo -e "\n${YELLOW}Checking test mode configuration...${NC}"
    
    if grep -q "property bool testMode: true" src/QmlControls/AreaPlanMapVisuals.qml; then
        echo -e "${GREEN}✓ Test mode is enabled${NC}"
        return 0
    else
        echo -e "${RED}✗ Test mode is not enabled${NC}"
        echo "Please set testMode: true in AreaPlanMapVisuals.qml"
        return 1
    fi
}

# Function to check console logging
check_console_logging() {
    echo -e "\n${YELLOW}Checking console logging configuration...${NC}"
    
    if grep -q "console.log" src/QmlControls/AreaPlanMapVisuals.qml; then
        echo -e "${GREEN}✓ Console logging is enabled${NC}"
        return 0
    else
        echo -e "${RED}✗ Console logging not found${NC}"
        return 1
    fi
}

# Function to check C++ compilation
check_cpp_compilation() {
    echo -e "\n${YELLOW}Checking C++ compilation...${NC}"
    
    if [ -f "src/QmlControls/AreaPlanEditor.h" ] && [ -f "src/QmlControls/AreaPlanEditor.cc" ]; then
        echo -e "${GREEN}✓ C++ source files found${NC}"
        
        # Try to compile a simple test
        cat > test_compile.cpp << 'EOF'
#include "src/QmlControls/AreaPlanEditor.h"
int main() {
    AreaPlanEditor editor;
    return 0;
}
EOF
        
        if g++ -c test_compile.cpp -I. -std=c++17 2>/dev/null; then
            echo -e "${GREEN}✓ C++ compilation successful${NC}"
            rm -f test_compile.cpp test_compile.o
            return 0
        else
            echo -e "${RED}✗ C++ compilation failed${NC}"
            rm -f test_compile.cpp test_compile.o
            return 1
        fi
    else
        echo -e "${RED}✗ C++ source files not found${NC}"
        return 1
    fi
}

# Function to check QML files
check_qml_files() {
    echo -e "\n${YELLOW}Checking QML files...${NC}"
    
    local qml_files=(
        "src/QmlControls/AreaPlanEditor.qml"
        "src/QmlControls/AreaPlanMapVisuals.qml"
    )
    
    local all_found=true
    for file in "${qml_files[@]}"; do
        if [ -f "$file" ]; then
            echo -e "${GREEN}✓ Found: $file${NC}"
        else
            echo -e "${RED}✗ Missing: $file${NC}"
            all_found=false
        fi
    done
    
    if [ "$all_found" = true ]; then
        return 0
    else
        return 1
    fi
}

# Function to check CMakeLists.txt integration
check_cmake_integration() {
    echo -e "\n${YELLOW}Checking CMakeLists.txt integration...${NC}"
    
    if grep -q "AreaPlanEditor.qml" src/QmlControls/CMakeLists.txt; then
        echo -e "${GREEN}✓ AreaPlanEditor.qml in CMakeLists.txt${NC}"
    else
        echo -e "${RED}✗ AreaPlanEditor.qml not in CMakeLists.txt${NC}"
        return 1
    fi
    
    if grep -q "AreaPlanMapVisuals.qml" src/QmlControls/CMakeLists.txt; then
        echo -e "${GREEN}✓ AreaPlanMapVisuals.qml in CMakeLists.txt${NC}"
    else
        echo -e "${RED}✗ AreaPlanMapVisuals.qml not in CMakeLists.txt${NC}"
        return 1
    fi
    
    return 0
}

# Function to run manual tests
run_manual_tests() {
    echo -e "\n${YELLOW}Manual Test Instructions${NC}"
    echo "=========================================="
    echo "1. Start QGroundControl"
    echo "2. Open Plan View"
    echo "3. Look for Area Plan Editor tab"
    echo "4. Check for blue border around map (test mode)"
    echo "5. Click 'Start Drawing Mode' button"
    echo "6. Verify red border appears"
    echo "7. Click on map to set center"
    echo "8. Drag to resize area"
    echo "9. Check console for debug logs"
    echo ""
    echo "Expected console output:"
    echo "  MouseArea pressed - interactive: true isDrawingMode: true mapControl: true"
    echo "  Mouse position: [x] [y]"
    echo "  Converted coordinate: [lat], [lon]"
    echo "  Area center set to: [lat] [lon]"
    echo ""
    echo "If you see these logs, the interactive drawing is working!"
}

# Function to show test summary
show_summary() {
    echo -e "\n=========================================="
    echo -e "${BLUE}Test Summary${NC}"
    echo "=========================================="
    echo -e "${GREEN}Passed: $PASSED${NC}"
    echo -e "${RED}Failed: $FAILED${NC}"
    echo -e "${YELLOW}Skipped: $SKIPPED${NC}"
    
    if [ $FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All tests passed! Interactive drawing should work.${NC}"
    else
        echo -e "\n${RED}Some tests failed. Please fix the issues before testing.${NC}"
    fi
}

# Main test execution
main() {
    echo "Starting interactive drawing tests..."
    
    # Run automated checks
    run_test "QGroundControl Build Check" "check_qgc_build"
    run_test "Test Mode Configuration" "check_test_mode"
    run_test "Console Logging Check" "check_console_logging"
    run_test "C++ Compilation Check" "check_cpp_compilation"
    run_test "QML Files Check" "check_qml_files"
    run_test "CMake Integration Check" "check_cmake_integration"
    
    # Show manual test instructions
    run_manual_tests
    
    # Show summary
    show_summary
}

# Run the tests
main "$@" 