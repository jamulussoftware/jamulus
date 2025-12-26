Jamulus JUCE plugin scaffold

This folder contains a minimal JUCE-based VST3 plugin adapter that delegates
real-time audio to the Jamulus wrapper built in `..`.

Prerequisites
- JUCE 6 (CMake config) available on the system. Configure CMake so that
  `find_package(JUCE 6 CONFIG REQUIRED)` succeeds. You can pass `-DJUCE_DIR=` pointing
  to a JUCE build directory with CMake config files.
- A working build of the jamulus wrapper in the parent directory (CMake will
  build it automatically when configuring from the `juce-wrapper` folder).

Build (from `juce-wrapper`):

```powershell
# from e:\...\jamulus\juce-wrapper
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release --target jamulus_vst3
```

The resulting VST3 binary will be placed in `build/bin/Release`.
