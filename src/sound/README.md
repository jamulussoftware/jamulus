### Copyright (c) 2022-2026

Author(s):
* ann0see
* The Jamulus Development Team

As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
under AGPL 3.0 or any later version.

Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
This code will be licensed under GPL 3.0 (or any later version) from
3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
the combined work, including network use provisions.

---

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see [<https://www.gnu.org/licenses/>](https://www.gnu.org/licenses/).

---

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [<https://www.gnu.org/licenses/>](https://www.gnu.org/licenses/).

# Sound APIs and Sound API related files

This folder contains the related files for all sound APIs.

## Documentation of sound design

This describes how the code behaves today. It covers the device lifecycle and the threading rules
around the audio callback; the areas still missing are listed at the end. Where today's behaviour
only makes sense in light of how it arose, the origin is given as well, so that an accident of
history is not read as a deliberate design.

Each platform provides one `CSound` class deriving from `CSoundBase`, and exactly one of the five
backend subdirectories is compiled in: `asio/` (Windows), `coreaudio-mac/`, `coreaudio-ios/`,
`oboe/` (Android) and `jack/` (where JACK is enabled). A sixth subdirectory, `midi-win/`, holds the
Windows MIDI implementation (`CMidi`) rather than a backend; the default Windows build compiles it
alongside `asio/`, while a `CONFIG+=jackonwindows` build takes `jack/` and neither of the other two.

### Buffer size negotiation

`Init ( iNewPrefMonoBufferSize )` returns the mono buffer size the device actually accepted,
which may differ from the one requested. `CClient::Init()` uses that return value to find out
which sizes a device supports, so it calls `Init()` four times per invocation: once for each of
`FRAME_SIZE_FACTOR_PREFERRED`, `FRAME_SIZE_FACTOR_DEFAULT` and `FRAME_SIZE_FACTOR_SAFE` to fill
`bFraSiFactPrefSupported`, `bFraSiFactDefSupported` and `bFraSiFactSafeSupported`, then once with
the size selected in the settings. Those three flags drive the enabled state of the buffer delay
radio buttons, and the settings dialog polls them once a second; no signal runs from the sound
device to that dialog.

A driver can also change its buffer size on its own, outside any call from Jamulus. Reporting
that back needs a native change-notification callback from the driver API, and only two of the
five backends have one: ASIO's `kAsioBufferSizeChange` (`asioMessages()`, `asio/sound.cpp`) and
JACK's buffer-size callback (`jack/sound.cpp`) both call
`EmitReinitRequestSignal ( RS_ONLY_RESTART_AND_INIT )`, which reaches
`CClient::OnSndCrdReinitRequest` and repeats the negotiation above. The other three backends
can't do this the same way: the two CoreAudio backends only watch device-identity/route events
(`kAudioDevicePropertyDeviceHasChanged`/`IsAlive`/default-device switches on macOS,
`AVAudioSessionRouteChangeNotification` on iOS) and never register a buffer-size-specific
listener, so a size change on its own is invisible to them. Oboe does detect it —
`onAudioInput()` compares the delivered frame count against the requested size on every callback
— but the mismatch is only logged (`qDebug()`), never renegotiated.

### Start, stop and the audio callback

`Init()` is only ever entered with the device stopped. Callers that may be running stop it first
and restart it afterwards, which is the `bWasRunning` pattern throughout `client.cpp`.

The audio callback runs on a thread owned by the driver. `CSoundBase` inherits `QThread`, but
nothing here overrides `run()` or calls `start()`, so no such thread exists — nor has it ever:
neither appears anywhere in this file's tracked history, and no other `QThread`-specific method
is used in the sound layer either. The inheritance predates the current driver-callback
architecture and contributes nothing; changing it to `QObject` looks safe from this file alone,
but that wasn't verified further here.

Every backend but ASIO also overrides `Stop()`, and every override but JACK's calls a
driver-level stop function *before* touching `bRun` at all:

| backend | own `Stop()` calls first |
|---|---|
| ASIO | (no override — `ASIOStop()` happens inside `CSound::Stop()` itself, see below) |
| CoreAudio (macOS) | `AudioDeviceStop()` + `AudioDeviceDestroyIOProcID()` |
| CoreAudio (iOS) | `AudioOutputUnitStop()` |
| Oboe | `closeStreams()` |
| JACK | nothing — goes straight to `CSoundBase::Stop()` |

Inside the callback itself, backends differ again, and not just in which flag they read but in
*when* they read it relative to the mutex:

| backend | audio callback | flag check | mutex behavior |
|---|---|---|---|
| Oboe | `onAudioReady()` | `!bRun`, first line, before any lock | skipped entirely once stopped |
| JACK | `process()` | `IsRunning()`, after the lock | always taken; only the processing is skipped |
| CoreAudio (macOS) | `callbackIO()` | `bRun`, after the lock | always taken; only the processing is skipped |
| CoreAudio (iOS) | `processBufferList()` | none | always taken; always processes |
| ASIO | `bufferSwitch()` | none | always taken (its own `ASIOMutex`); always processes |

`IsRunning()`, `bRun` and `!bRun` all read the same flag (`IsRunning()` is `return bRun;`) —
three spellings of one check. But only Oboe's placement actually avoids the mutex; JACK's and
CoreAudio (macOS)'s checks run *after* the lock is already held, so they cost the same
mutex-contention as not checking at all and only save the processing work itself.

`CSoundBase::Stop()` clears `bRun` and then briefly takes `MutexAudioProcessCallback`, releasing
it as soon as `Stop()` returns:
```cpp
void CSoundBase::Stop() {
    bRun = false;
    QMutexLocker locker ( &MutexAudioProcessCallback );
}
```
CoreAudio (iOS) and ASIO have no flag check anywhere in their callback, so they depend entirely
on their own driver-level stop call above (or, for ASIO, on `ASIOStop()` inside its own
`CSound::Stop()`) actually preventing further callbacks — if the driver fires one more callback
after that call returns, it runs to completion regardless of `bRun`. Whether `AudioOutputUnitStop`
/ `ASIOStop` are synchronous enough to rule that out wasn't tested here; it would need real
hardware.

ASIO defines and owns its own `ASIOMutex` (in `asio/sound.h`) instead of using the shared
`MutexAudioProcessCallback`. Its own `CSound::Stop()` calls `ASIOStop()` first, then
`CSoundBase::Stop()` (whose wait on the unused `MutexAudioProcessCallback` returns immediately
for this backend), then waits on `ASIOMutex` directly to confirm the callback thread is done.

The two are not interchangeable, because `ASIOMutex` covers more ground. It is also held across
the whole of `CSound::Init()` — spanning `ASIODisposeBuffers()`, `ASIOCreateBuffers()` and the
`vecsMultChanAudioSndCrd` reallocation — whereas no other backend locks anything in its `Init()`.
The waits differ too: `CSoundBase::Stop()` blocks unconditionally on its `QMutexLocker`, while
ASIO's `Stop()` uses `tryLock ( 5000 )` and carries on regardless if the callback has not finished
within five seconds. The two differences compound: because ASIO's stop can return while a callback
is still in flight, the lock held across `Init()` is what actually keeps `ASIOCreateBuffers()` off
a live `bufferSwitch()`.

That there are two mutexes at all is chronological rather than a design decision. `ASIOMutex` was
added with the ASIO backend itself in `5eb86941` (2008-07-12), when ASIO was the only backend and
`CSoundBase` did not yet exist (`3fb2d9ca`, 2009-02-22); its drain-on-stop wait followed in
`73f408e4` (2011-12-27). The shared `MutexAudioProcessCallback` arrived nine years after that, in
`ecff80fc` (2020-08-26), to fix a crash when the JACK backend was reconfigured quickly; that commit
touches `linux/sound.cpp` and `soundbase.{h,cpp}` and nothing else. It is a later, independent
re-implementation of a guard ASIO already had, and it was never extended to ASIO. Collapsing the
two into one would therefore not be a rename: it would have to preserve the coverage across
`Init()` and settle which of the two wait policies applies.

### Not yet documented

- device enumeration, and what `SetDev()` does when a device cannot be used
- input and output channel selection, and the input channel mixing in the callbacks
- MIDI: device selection, controller mapping and `ParseMIDIMessage()`
- latency reporting via `GetInOutLatencyMs()`
- the sound card conversion buffer used when a device's buffer size is not a multiple of the
  system frame size
