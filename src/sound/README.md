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
around the audio callback; the areas still missing are listed at the end.

Each platform provides one `CSound` class deriving from `CSoundBase`, and exactly one of the
subdirectories is compiled in: `asio/` (Windows), `coreaudio-mac/`, `coreaudio-ios/`, `oboe/`
(Android) and `jack/` (where JACK is enabled).

### Buffer size negotiation

`Init ( iNewPrefMonoBufferSize )` returns the mono buffer size the device actually accepted,
which may differ from the one requested. `CClient::Init()` uses that return value to find out
which sizes a device supports, so it calls `Init()` four times per invocation: once for each of
`FRAME_SIZE_FACTOR_PREFERRED`, `FRAME_SIZE_FACTOR_DEFAULT` and `FRAME_SIZE_FACTOR_SAFE` to fill
`bFraSiFactPrefSupported`, `bFraSiFactDefSupported` and `bFraSiFactSafeSupported`, then once with
the size selected in the settings. Those three flags drive the enabled state of the buffer delay
radio buttons, and the settings dialog polls them once a second; no signal runs from the sound
device to that dialog.

A driver may also change the buffer size on its own. `kAsioBufferSizeChange` in the ASIO backend
and JACK's buffer size callback both report that by calling
`EmitReinitRequestSignal ( RS_ONLY_RESTART_AND_INIT )`, which reaches
`CClient::OnSndCrdReinitRequest` and repeats the negotiation above.

### Start, stop and the audio callback

`Init()` is only ever entered with the device stopped. Callers that may be running stop it first
and restart it afterwards, which is the `bWasRunning` pattern throughout `client.cpp`.

The audio callback runs on a thread owned by the driver. Backends keep it away from a device that
is being re-initialised in two ways, and the ASIO backend is the exception to both:

| backend | audio callback | ignores the callback while stopped | takes `MutexAudioProcessCallback` |
|---|---|---|---|
| JACK | `process()` | yes, `IsRunning()` | yes |
| CoreAudio (macOS) | `callbackIO()` | yes, `bRun` | yes |
| CoreAudio (iOS) | `processBufferList()` | no | yes |
| Oboe | `onAudioReady()` | yes, `!bRun` | yes |
| ASIO | `bufferSwitch()` | no | no, it uses its own `ASIOMutex` |

`CSoundBase::Stop()` clears `bRun` and then takes `MutexAudioProcessCallback` to wait for a
callback that is already in flight. The ASIO backend never takes that mutex, so on Windows that
wait returns immediately and `CSound::Stop()` waits on `ASIOMutex` instead.

### Not yet documented

- device enumeration, and what `SetDev()` does when a device cannot be used
- input and output channel selection, and the input channel mixing in the callbacks
- MIDI: device selection, controller mapping and `ParseMIDIMessage()`
- latency reporting via `GetInOutLatencyMs()`
- the sound card conversion buffer used when a device's buffer size is not a multiple of the
  system frame size
