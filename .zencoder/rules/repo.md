---
description: Repository Information Overview
alwaysApply: true
---

# Repository Information Overview

## Repository Summary
Jamulus is open-source software that enables musicians to perform in real-time together over the internet. It supports Windows, macOS, Linux, iOS, and Android. The repository also contains a prototype VST3 wrapper to run Jamulus as a plugin.

## Repository Structure
- **src/**: Main C++ source code for the Jamulus application.
- **juce-wrapper/**: Prototype VST3 wrapper using JUCE.
- **android/**, **ios/**, **linux/**, **mac/**, **windows/**: Platform-specific deployment scripts and resources.
- **libs/**: External libraries (Opus, Oboe, ASIO SDK).
- **tools/**: Helper scripts for development and maintenance.
- **.github/**: CI/CD workflows and build scripts.

### Main Repository Components
- **Jamulus Application**: The standalone client/server application.
- **Jamulus VST Wrapper**: A VST3 plugin wrapper for Jamulus.

## Projects

### Jamulus Application
**Configuration File**: `Jamulus.pro` (qmake)

#### Language & Runtime
**Language**: C++ (C++11 for Unix, C++17 for Android)
**Framework**: Qt 5.12+ (Qt 6 supported)
**Build System**: qmake

#### Dependencies
**Main Dependencies**:
- **Qt**: Core, Network, Widgets, Xml, Concurrent, Multimedia, AndroidExtras (Android only)
- **Opus**: Audio codec (bundled or shared)
- **JACK**: Audio connection kit (optional on Linux/Windows/macOS)
- **ASIO**: Audio driver support (Windows optional)
- **Oboe**: High-performance audio (Android only)

#### Build & Installation
**Linux**:
```bash
qmake "CONFIG+=headless serveronly" # Example config
make
sudo make install
```

**Windows**:
```powershell
.\windows\deploy_windows.ps1 "C:\Qt\<Qt32Bit>" "C:\Qt\<Qt64Bit>"
```

**macOS**:
```bash
qmake -spec macx-xcode Jamulus.pro
xcodebuild build
```

#### Docker
No Dockerfile found in the root, but CI uses `ubuntu:20.04` containers for Linux builds.

#### Testing
**Framework**: Custom testbench (implied by `src/testbench.h`) and GitHub Actions workflows.
**Test Location**: `src/` (integrated in source)
**Configuration**: `Jamulus.pro`

### Jamulus VST Wrapper
**Configuration File**: `juce-wrapper/CMakeLists.txt`

#### Language & Runtime
**Language**: C++17
**Framework**: JUCE (optional), Qt 6/5
**Build System**: CMake

#### Dependencies
**Main Dependencies**:
- **Qt**: Core, Network, Widgets, Xml, Concurrent
- **JUCE**: Audio application framework (for VST3 plugin)
- **Jamulus Core**: Linked as a static library (`libjamulus`)

#### Build & Installation
```bash
cd juce-wrapper
mkdir build && cd build
cmake -G "Ninja" ..
cmake --build .
```

#### Testing
**Framework**: Custom test executable `jamulus_test`
**Test Location**: `juce-wrapper/test/`
**Run Command**:
```bash
./build/bin/jamulus_test
```
