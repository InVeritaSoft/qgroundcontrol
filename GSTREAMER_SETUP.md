# GStreamer Setup for QGroundControl

This document provides instructions for resolving GStreamer detection issues when building QGroundControl on Windows.

## Problem

When building QGroundControl, you may encounter this error:
```
CMake Error at cmake/find-modules/FindGStreamer.cmake:226 (message):
  Could not locate GStreamer - check installation or set environment/cmake variables
```

## Solution

### 1. Install GStreamer

1. Download GStreamer from: https://gstreamer.freedesktop.org/download/
2. Choose the **MSVC x86_64** version (not MinGW)
3. Install to the default location (`C:\gstreamer\1.0\msvc_x86_64`)

### 2. Set Environment Variables

Set the following environment variable to point to your GStreamer installation:

```powershell
# PowerShell
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = "C:\gstreamer\1.0\msvc_x86_64"
```

```cmd
# Command Prompt
set GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\gstreamer\1.0\msvc_x86_64
```

### 3. Use the Setup Scripts

We've provided setup scripts to automate the environment configuration:

#### PowerShell Script
```powershell
.\setup_gstreamer.ps1
```

#### Batch Script
```cmd
setup_gstreamer.bat
```

These scripts will:
- Check if GStreamer is installed in standard locations
- Verify all required directories exist
- Set the correct environment variables
- Provide feedback on what was found/missing

### 4. Verify Installation

After running the setup script, verify that these directories exist:
- `C:\gstreamer\1.0\msvc_x86_64\lib`
- `C:\gstreamer\1.0\msvc_x86_64\include`
- `C:\gstreamer\1.0\msvc_x86_64\bin`
- `C:\gstreamer\1.0\msvc_x86_64\lib\gstreamer-1.0`

### 5. Build QGroundControl

Now you can configure and build QGroundControl:

```cmd
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

## Troubleshooting

### If GStreamer is not found:

1. **Check installation path**: Make sure GStreamer is installed in `C:\gstreamer\1.0\msvc_x86_64`
2. **Verify MSVC version**: Ensure you installed the MSVC x86_64 version, not MinGW
3. **Check environment variables**: Run `echo %GSTREAMER_1_0_ROOT_MSVC_X86_64%` to verify the variable is set
4. **Restart terminal**: Environment variables may need a fresh terminal session

### If directories are missing:

1. **Reinstall GStreamer**: The installation may be incomplete
2. **Check permissions**: Ensure you have read access to the GStreamer directory
3. **Verify download**: Make sure you downloaded the complete MSVC package

### If CMake still fails:

1. **Clear build directory**: Delete the `build` directory and recreate it
2. **Check CMake version**: Ensure you're using a recent version of CMake
3. **Use verbose output**: Add `--debug-output` to CMake command for more details

## Alternative Solutions

### Option 1: Disable GStreamer (if video is not needed)

If you don't need video support, you can disable GStreamer:

```cmd
cmake .. -G "Visual Studio 17 2022" -A x64 -DQGC_ENABLE_GSTREAMER=OFF
```

### Option 2: Use a different GStreamer location

If GStreamer is installed in a different location, set the environment variable accordingly:

```cmd
set GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\Your\Custom\Path\To\GStreamer
```

### Option 3: Set CMake variable directly

You can also set the GStreamer path directly in CMake:

```cmd
cmake .. -G "Visual Studio 17 2022" -A x64 -DGStreamer_ROOT_DIR="C:/gstreamer/1.0/msvc_x86_64"
```

## Additional Notes

- The improved `FindGStreamer.cmake` file now includes better debugging output
- The setup scripts will help identify exactly what's missing
- Make sure to use forward slashes (`/`) in CMake paths, not backslashes (`\`)
- The environment variable must be set before running CMake

## Support

If you continue to have issues:
1. Run the setup script and check the output
2. Verify all required directories exist
3. Check that you're using the correct GStreamer version (MSVC x86_64)
4. Ensure CMake can find the GStreamer installation 