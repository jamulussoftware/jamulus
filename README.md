# JamulusPlus Build Root

This repository is the authoritative JUCE/non-Qt build root for JamulusPlus. It builds the non-Qt core API, the JUCE plugin targets, and the standalone target from one CMake tree.

Contents:
- `jamulus_core_juce.*` - non-Qt Jamulus core implementation and C API surface.
- `juce_net_abstraction.*` - JUCE-backed networking helpers for the non-Qt core.
- `plugin/` - JUCE plugin and standalone targets.
- `CMakeLists.txt` - top-level non-Qt CMake entry point.

Notes:
- This is the non-Qt path. It does not require Qt packages or Qt runtime DLL deployment.
- Use `b` as the default local build directory.

Build:

```powershell
cmake -S . -B b -G "Visual Studio 17 2022" -A x64
cmake --build .\b --config Release --target jamulus_dist
cmake --build .\b --config Release --target jamulus_dist_standalone
```
