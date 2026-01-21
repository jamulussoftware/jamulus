# Restore Point: VST Audio & Crash Fixes

## Current State
The project is a VST3 wrapper for Jamulus, built using JUCE. Recent work has focused on fixing audio routing bugs (Mute/Solo/Boost), ensuring persistent settings, and resolving a critical DAW crash during plugin unloading.

## Completed Tasks

### 1. Audio Logic & Mixer Fixes
- **Mute/Solo Fix**: Implemented stable tracking of Mute and Solo states in `JamulusGui.h`. Audio output is now correctly affected by these states, not just the meters.
- **Solo Burst Bug**: Resolved an issue where soloed channels would momentarily unmute when "Auto Level" was disabled. Logic is now centralized in the `timerCallback`.
- **Boost Functionality**: Fixed the Boost button to work correctly with and without Auto Level. For "Me", it uses the native Jamulus input boost; for remote users, it applies a digital +6dB gain.
- **Me Track Normalization**: Forced the local user's ID to `-1` for consistent local settings management, preventing settings from being lost when the server-assigned ID changed.

### 2. Networking & Responsiveness
- **Event Loop Processing**: Integrated `jamulus_process_events()` into the `timerCallback` (30Hz) to ensure Qt events (like gain/pan rate-limiting timers) fire correctly within the DAW environment.
- **Connection Stability**: Added a `jamulus_client_disconnect` call and a 100ms delay during re-connections to the same server to prevent audio loss/corrupted state.

### 3. Persistence
- **User Settings**: Implemented JSON-based persistence for per-user Gain, Pan, and Mono/Stereo settings.
- **Mixer State**: Profiles and general audio/network settings are correctly saved and loaded.

### 4. Stability & Crash Prevention (Plugin Unload)
- **Background Thread Management**: Added a virtual destructor to `WrapperClient` in `jamulus_wrapper.cpp` to ensure the audio worker thread is joined before the object is destroyed.
- **Qt Resource Lifecycle**: Added reference counting for global clients. The event pump timer and shared GUI dialogs are now stopped and cleaned up (using `deleteLater`) only when the last plugin instance is destroyed.
- **Ref-Counted Shutdown**: Implemented a "final pump" of the Qt event loop during the last client's destruction to handle final cleanup tasks before DLL unmapping.

## Core Files
- `JamulusGui.h`: Contains the main mixer UI, fader/button logic, and the centralized `timerCallback` for gain dispatching.
- `jamulus_wrapper.cpp`: Bridge between JUCE/C++ and the Jamulus Qt core. Handles audio FIFO, background threads, and Qt application lifecycle.
- `PluginProcessor.cpp`: Standard JUCE audio processor; manages the `jamulus_client` lifecycle and resampling.

## Next Steps
- **Validation**: Verify the unload stability in additional hosts (FL Studio, Ableton) if possible.
- **UI Polish**: Improve the "Chat" and "Settings" component aesthetics to match the main mixer.
- **Performance**: Monitor CPU usage of the `timerCallback` if the number of remote users is very high (>50).
