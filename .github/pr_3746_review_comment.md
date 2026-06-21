# Code Review: iOS Multichannel Audio Support (#3746)

## 🔍 Executive Summary

**Status:** ⚠️ **Needs safety hardening before merge**

After comparing this implementation against the macOS audio backend and analyzing the design, I've identified **several race conditions and bounds-checking gaps** that could cause crashes or undefined behavior with edge cases—particularly when using actual multichannel USB-C/Lightning interfaces on iPad.

The **core logic is sound**, but defensive protections are missing that exist in the macOS implementation.

---

## 🔴 **Critical Issues:**

### **1. Race Condition: Unprotected Channel Selection in Audio Callback**

**Location:** `processBufferList()`, lines ~145-150

The audio callback reads `iNumChan`, `iLeftCh`, and `iRightCh` **without holding the audio mutex**:

```cpp
const int iNumChan = pSound->buffer.mNumberChannels;      // ⚠️ UNPROTECTED
const int iLeftCh  = pSound->iSelInputLeftChannel;        // ⚠️ UNPROTECTED
const int iRightCh = pSound->iSelInputRightChannel;       // ⚠️ UNPROTECTED
```

Meanwhile, `SwitchDevice()` → `UpdateInputChannelInfo()` can modify these values from another thread (the main UI thread). If this occurs between reading `iLeftCh` and using it in the loop, the code reads:

```cpp
pData[iNumChan * i + iLeftCh]  // Could be out-of-bounds if iLeftCh >= iNumChan
```

**macOS Comparison:** The macOS callback (`sound.cpp:1026-1049`) **does** use mutex protection:
```cpp
QMutexLocker locker ( &pSound->MutexAudioProcessCallback );
```

**Impact:** On a fast device switch (e.g., unplugging a 4-channel interface, then plugging in 2-channel), this could read garbage audio or crash.

**Fix:** Wrap the channel reads with the existing mutex lock.

---

### **2. Missing Bounds Check Before Indexing**

**Location:** `processBufferList()`, lines ~149-152

```cpp
for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
{
    pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( pData[iNumChan * i + iLeftCh] * _MAXSHORT );
    pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( pData[iNumChan * i + iRightCh] * _MAXSHORT );
}
```

**Problem:** No validation that `iLeftCh < iNumChan` and `iRightCh < iNumChan` before using them as indices.

**macOS Comparison:** Macros explicitly bounds-check:
```cpp
if ( ( iSelInBufferLeft >= 0 ) && ( iSelInBufferLeft < static_cast<int> ( inInputData->mNumberBuffers ) ) && ... )
{
    // safe to use iSelInBufferLeft
}
else
{
    // clear buffer and bail out
    pSound->vecsTmpAudioSndCrdStereo.Reset ( 0 );
}
```

**Impact:** If `iNumChan = 2` but `iRightCh = 5` (due to race condition or device list corruption), the interleaved buffer read will access uninitialized memory.

**Fix:** Add defensive clamping before the loop:
```cpp
if ( iLeftCh >= iNumChan )  iLeftCh = 0;
if ( iRightCh >= iNumChan ) iRightCh = ( iNumChan > 1 ) ? 1 : 0;
```

---

### **3. Silent Failure in Device Port Lookup**

**Location:** `SwitchDevice()`, lines ~406-412

```cpp
if ( iPortIdx < availableInputs.count )
{
    [sessionInstance setPreferredInput:availableInputs[iPortIdx] error:&error];
}
// Falls back silently if iPortIdx is out of range—no warning logged
```

**Problem:** If the device list changes between `GetAvailableInOutDevices()` and `SwitchDevice()` (e.g., user unplugs interface), the stored index becomes stale. The function silently falls back to the default input with **no diagnostic output**.

**macOS handles this:** The macOS code enumerates devices fresh on each device switch and validates thoroughly.

**Impact:** Silent audio misconfiguration—user thinks they're using the external interface but the app reverted to built-in mic without any indication.

**Fix:** Add explicit warning log:
```cpp
else
{
    qWarning() << "SwitchDevice: Port index" << iPortIdx << "out of range."
               << "Device list may have changed. Falling back to system default.";
    [sessionInstance setPreferredInput:nil error:&error];
}
```

---

### **4. Unchecked Return from `maximumInputNumberOfChannels`**

**Location:** `SwitchDevice()`, lines ~414-418

```cpp
const NSInteger iMaxChannels = [sessionInstance maximumInputNumberOfChannels];
[sessionInstance setPreferredInputNumberOfChannels:qBound ( 1, iMaxChannels, MAX_NUM_IN_OUT_CHANNELS ) error:&error];
```

**Problem:** If `iMaxChannels` returns 0, negative, or an invalid value (AVFoundation edge cases do happen), `qBound()` will silently clamp it. No warning is logged.

**Impact:** The code proceeds with a clamped channel count that might not match the device's actual capability, leading to silent audio truncation.

**Fix:** Validate and warn:
```cpp
if ( iMaxChannels > 0 )
{
    [sessionInstance setPreferredInputNumberOfChannels:qBound ( static_cast<NSInteger> ( 1 ), iMaxChannels, static_cast<NSInteger> ( MAX_NUM_IN_OUT_CHANNELS ) )
                                                 error:&error];
}
else
{
    qWarning() << "SwitchDevice: AVAudioSession.maximumInputNumberOfChannels returned" << iMaxChannels
               << "-- this is unexpected. Proceeding with default 2 channels.";
}
```

---

### **5. Missing Error Logging in Device Switch**

**Location:** `SwitchDevice()` entire function

The function silently ignores `NSError* error` after all AVAudioSession operations:

```cpp
[sessionInstance setPreferredInput:... error:&error];  // error ignored
[sessionInstance setPreferredInputNumberOfChannels:... error:&error];  // error ignored
```

**Problem:** If AVAudioSession rejects the device switch (e.g., permission denied, device unavailable), the error is swallowed with no logging.

**macOS Comparison:** macOS logs device notifications and errors via callbacks.

**Impact:** Silent failure—user selects a device that fails to activate, sees no indication, and audio breaks.

**Fix:** Check and log errors:
```cpp
if ( error )
{
    qWarning() << "SwitchDevice: AVAudioSession error:" << QString::fromNSString ( error.localizedDescription );
}
```

---

## 🟡 **Design Concerns:**

### **6. Changed Device List Indexing Semantics (Fragile but Functional)**

**Old behavior:** Index 0 = System Default, Index 1 = Built-in Mic (hardcoded)
**New behavior:** Index 0 = System Default, Index 1-N = All enumerated ports

The new approach is better, but the index-1 offset is brittle:

```cpp
const NSUInteger iPortIdx = static_cast<NSUInteger> ( iDriverIdx - 1 );
```

**Risk:** If UI code or older settings persist with the old indexing scheme, the offset is wrong. Consider adding a comment clarifying the new scheme and incrementing a device-config version number if you have one.

---

### **7. No Validation of `UpdateInputChannelInfo()` Results**

**Location:** `UpdateInputChannelInfo()`, lines ~417+

```cpp
AVAudioSessionPortDescription* inputPort = sessionInstance.currentRoute.inputs.firstObject;
NSArray<AVAudioSessionChannelDescription*>* channels = inputPort.channels;

for ( int i = 0; i < iNumInChan; i++ )
{
    QString strChanName = QString ( "Channel %1" ).arg ( i + 1 );
    
    if ( channels && ( i < channels.count ) && channels[i].channelName.length > 0 )
    {
        strChanName = QString::fromNSString ( channels[i].channelName );
    }
    
    sChannelNamesInput[i] = QString ( "%1: %2" ).arg ( i + 1 ).arg ( strChanName );
}
```

**Problem:** If `currentRoute.inputs.firstObject` is null (no input device configured), channel names silently default to "Channel 1", "Channel 2", etc. This could mask serious device configuration failures.

**Suggestion:** Add a debug log to catch this scenario:
```cpp
if ( !inputPort )
{
    qDebug() << "UpdateInputChannelInfo: No input port in current audio route. "
             << "Using generic channel names.";
}
```

---

## ✅ **What Works Well:**

- ✓ Separate input/output `AudioStreamBasicDescription` configs is architecturally correct
- ✓ Channel name formatting matches macOS conventions
- ✓ `UpdateInputChannelInfo()` properly re-clamps selections when device changes
- ✓ The use of `qBound()` for clamping is safe **after validation**
- ✓ Loop over `availableInputs` is the right approach for enumerating ports

---

## 📋 **Recommended Actions Before Merge:**

### **Tier 1: Critical (Security/Stability)**

1. **Add mutex protection** in `processBufferList()` when reading channel selection variables
2. **Add bounds checks** before indexing with `iLeftCh` and `iRightCh`
3. **Add error logging** in `SwitchDevice()` for all AVAudioSession failures
4. **Validate `maximumInputNumberOfChannels` result** and warn if unexpected

### **Tier 2: Important (User Experience)**

5. **Add diagnostic logging** if port index goes out of range
6. **Add debug logging** if `UpdateInputChannelInfo()` finds no input port
7. **Document the new device list indexing scheme** in code comments

### **Tier 3: Testing (Hardware Validation)**

8. **Build and test on actual iPad with Xcode debugger**
9. **Test device switching scenarios:**
   - Plug in USB-C audio interface (4 channels) → select channel 3
   - Unplug interface mid-session → verify graceful fallback
   - Switch between built-in mic, headset, and external interface
   - Test edge case: interface that reports 0 channels (shouldn't happen, but...)

---

## 🔗 **Comparison to macOS Implementation**

The macOS backend (`src/sound/coreaudio-mac/sound.cpp`) has:
- Explicit size checks before buffer access (line ~1029)
- Full device enumeration on each switch (not index-based)
- Channel mapping logic with buffer-level indirection (`iSelInBufferLeft`, etc.)
- Comprehensive error handling in device notifications

The iOS code is simpler (fewer devices, no enumeration complexity) but needs the same **defensive rigor**.

---

## 🎯 **Summary**

This PR brings valuable functionality to iOS Jamulus—multichannel USB/Lightning audio support is important. However, the implementation has **threading safety gaps** and **error handling blind spots** that the macOS backend (rightfully) doesn't have.

With the fixes above (especially Tier 1), this becomes a solid, production-ready feature. **Testing on real hardware is non-negotiable** before merge.

Happy to help implement any of these fixes or discuss alternatives!
