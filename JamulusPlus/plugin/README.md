JamulusPlus JUCE plugin scaffold

This folder contains the JUCE plugin and standalone targets for JamulusPlus.

Build (from `JamulusPlus`):

```powershell
# from e:\...\jamulus\JamulusPlus
cmake -S . -B b -G "Visual Studio 17 2022" -A x64
cmake --build .\b --config Release --target jamulus_engine_VST3
cmake --build .\b --config Release --target jamulus_dist
cmake --build .\b --config Release --target jamulus_dist_standalone
```

The resulting binaries are produced under `b\plugin\jamulus_engine_artefacts\Release\` and packaged under `dist\`.