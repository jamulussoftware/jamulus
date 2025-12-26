# Jamulus JUCE Wrapper (Prototype)

This folder contains a minimal, non-invasive wrapper sketch to build Jamulus core into a library and expose a small C API for use by a JUCE VST3 plugin.

Contents:
- `jamulus_wrapper.h/cpp` — C API that constructs `CClient` and exposes basic controls.
- `virtual_sound/virtual_sound.h` — a virtual sound backend stub (passthrough) you can hook into later.
- `plugin/PluginProcessor.*` — a small skeleton showing how a JUCE processor might call the wrapper.
- `CMakeLists.txt` — CMake target `jamulus_wrapper` (STATIC) for building the wrapper. You must still add Jamulus core sources as a linked target.

Notes:
- This is an initial prototype. Real integration requires creating a `libjamulus` target from Jamulus core sources and a proper audio adapter that avoids allocations/locks on the host audio thread.
- Qt event loop is created in the wrapper when necessary. Ensure Qt libraries are available when linking.

Build (example):

```powershell
cd "e:/Web stuff/Jamulus VST Wrapper/jamulus/juce-wrapper"
mkdir build; cd build
cmake -G "Ninja" ..
cmake --build .
```
