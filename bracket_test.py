#!/usr/bin/env python3
"""
QML Bracket Structure Validator
Tests QML files for proper bracket/brace matching and structure
"""

import re
import sys
from pathlib import Path

def validate_qml_structure(file_path):
    """Validate QML file structure and bracket matching"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: File {file_path} not found")
        return False
    except Exception as e:
        print(f"Error reading file: {e}")
        return False
    
    lines = content.split('\n')
    stack = []
    bracket_map = {'{': '}', '[': ']', '(': ')'}
    line_number = 0
    
    print(f"Validating: {file_path}")
    print("=" * 50)
    
    for line in lines:
        line_number += 1
        stripped_line = line.strip()
        
        # Skip empty lines and comments
        if not stripped_line or stripped_line.startswith('//'):
            continue
            
        # Check for opening brackets
        for char in line:
            if char in bracket_map:
                stack.append((char, line_number))
                print(f"Line {line_number}: Opening '{char}'")
                
        # Check for closing brackets
        for char in line:
            if char in bracket_map.values():
                if not stack:
                    print(f"ERROR Line {line_number}: Unexpected closing '{char}'")
                    return False
                    
                last_open, open_line = stack.pop()
                if bracket_map[last_open] != char:
                    print(f"ERROR Line {line_number}: Mismatched brackets")
                    print(f"  Expected '{bracket_map[last_open]}', got '{char}'")
                    print(f"  Opening bracket '{last_open}' was on line {open_line}")
                    return False
                else:
                    print(f"Line {line_number}: Closing '{char}' matches opening '{last_open}' from line {open_line}")
    
    # Check for unclosed brackets
    if stack:
        print(f"ERROR: {len(stack)} unclosed bracket(s):")
        for bracket, line_num in stack:
            print(f"  '{bracket}' opened on line {line_num}")
        return False
    
    print("✓ All brackets properly matched!")
    return True

def analyze_qml_structure(file_path):
    """Analyze QML structure and provide detailed report"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading file: {e}")
        return
    
    lines = content.split('\n')
    
    print(f"\nDetailed Analysis of: {file_path}")
    print("=" * 60)
    
    # Count different types of structures
    item_count = 0
    rectangle_count = 0
    connections_count = 0
    property_count = 0
    
    for i, line in enumerate(lines, 1):
        stripped = line.strip()
        
        if 'Item {' in stripped:
            item_count += 1
            print(f"Line {i}: Item declaration")
        elif 'Rectangle {' in stripped:
            rectangle_count += 1
            print(f"Line {i}: Rectangle declaration")
        elif 'Connections {' in stripped:
            connections_count += 1
            print(f"Line {i}: Connections declaration")
        elif 'property ' in stripped:
            property_count += 1
            print(f"Line {i}: Property declaration")
    
    print(f"\nStructure Summary:")
    print(f"  Items: {item_count}")
    print(f"  Rectangles: {rectangle_count}")
    print(f"  Connections: {connections_count}")
    print(f"  Properties: {property_count}")
    
    # Check for common QML issues
    print(f"\nCommon Issues Check:")
    
    # Check for proper indentation
    indentation_issues = []
    for i, line in enumerate(lines, 1):
        if line.strip() and not line.startswith(' ') and not line.startswith('\t'):
            if any(keyword in line for keyword in ['{', '}', 'property', 'function']):
                indentation_issues.append(i)
    
    if indentation_issues:
        print(f"  ⚠️  Potential indentation issues on lines: {indentation_issues}")
    else:
        print(f"  ✓ Indentation looks consistent")
    
    # Check for missing semicolons in property declarations
    semicolon_issues = []
    for i, line in enumerate(lines, 1):
        if 'property ' in line and not line.strip().endswith(';'):
            semicolon_issues.append(i)
    
    if semicolon_issues:
        print(f"  ⚠️  Property declarations without semicolons on lines: {semicolon_issues}")
    else:
        print(f"  ✓ Property declarations properly terminated")

def main():
    if len(sys.argv) != 2:
        print("Usage: python bracket_test.py <qml_file>")
        print("Example: python bracket_test.py src/QmlControls/AreaPlanEditor.qml")
        sys.exit(1)
    
    file_path = sys.argv[1]
    
    if not Path(file_path).exists():
        print(f"Error: File {file_path} does not exist")
        sys.exit(1)
    
    print("QML Bracket Structure Validator")
    print("=" * 40)
    
    # Run basic bracket validation
    is_valid = validate_qml_structure(file_path)
    
    # Run detailed analysis
    analyze_qml_structure(file_path)
    
    if is_valid:
        print(f"\n✓ {file_path} has valid bracket structure")
        sys.exit(0)
    else:
        print(f"\n✗ {file_path} has bracket structure errors")
        sys.exit(1)

if __name__ == "__main__":
    main() 