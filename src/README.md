<!--
### Copyright (c) 2026

Author(s):
* mcfnord
* The Jamulus Development Team

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-->

# Main Jamulus codebase

Licensed under the AGPL 3.0 or any later version; full text in [../COPYING](../COPYING).

This directory contains the main code of Jamulus.

Code used by both client and server:

- [main.cpp](main.cpp) parses the command line and constructs a `CClient` or a `CServer`. The entrypoint of the application.
- [protocol.cpp](protocol.cpp) — Programmatic specification of the protocol. `CProtocol` contains message framing, acknowledgement and
  retransmission. Wire format: [../docs/JAMULUS_PROTOCOL.md](../docs/JAMULUS_PROTOCOL.md).
- [channel.cpp](channel.cpp) — One connection to a peer, used by both client and server. `CChannel` holds the connection state and its
  receive jitter buffer.
- [socket.cpp](socket.cpp) — The UDP socket shared by all sending and receiving. `CSocket`, `CHighPrioSocket` and the receive thread.
- [buffer.h](buffer.h) — The jitter buffer. `CNetBuf` and `CNetBufWithStats`, including the automatic size decision.
- [settings.cpp](settings.cpp) — Reads and writes the settings XML file. `CSettings`, specialised as `CClientSettings` and `CServerSettings`.
- [util.h](util.h) / [util.cpp](util.cpp) — Shared helpers. `CHighPrecisionTimer` (the server's frame clock) among others.

Client only:

- [client.cpp](client.cpp) — The client's audio path. `CClient` and its single channel.
- [sound/](sound/) — The sound layer, one backend per platform. See [sound/README.md](sound/README.md).
- [plugins/audioreverb.cpp](plugins/audioreverb.cpp) — The reverb effect. `CAudioReverb`.
- [clientdlg.cpp](clientdlg.cpp), [clientsettingsdlg.cpp](clientsettingsdlg.cpp),
  [audiomixerboard.cpp](audiomixerboard.cpp), [connectdlg.cpp](connectdlg.cpp),
  [chatdlg.cpp](chatdlg.cpp) — The GUI.
- [clientrpc.cpp](clientrpc.cpp) — The client half of the JSON-RPC API.

Server only:

- [server.cpp](server.cpp) — The server's frame cycle. `CServer` holds the channels and builds each client's mix.
- [serverlist.cpp](serverlist.cpp) — Directory registration and the server list.
- [recorder/](recorder/) — Recording of a session. `CJamController` and `CJamRecorder`.
- [serverlogging.cpp](serverlogging.cpp) — The connection log.
- [serverrpc.cpp](serverrpc.cpp) — The server half of the JSON-RPC API.
- [serverdlg.cpp](serverdlg.cpp) — The server GUI.

The JSON-RPC API ([rpcserver.cpp](rpcserver.cpp), [clientrpc.cpp](clientrpc.cpp),
[serverrpc.cpp](serverrpc.cpp)) is documented in [../docs/JSON-RPC.md](../docs/JSON-RPC.md).

## Jamulus Architecture

Jamulus is a client/server system. Each client encodes the audio from its sound device and sends it
to the server over UDP. The server decodes every client's stream and builds a separate mix for each
connected client, using that client's own fader gains, then re-encodes it and sends it back to be
decoded and played out. At both ends, arriving packets go through a jitter buffer first.

Background: [Performing Band Rehearsals on the Internet with Jamulus](https://jamulus.app/PerformingBandRehearsalsontheInternetWithJamulus.pdf),
Volker Fischer's case study, based on three years of weekly online rehearsals.

### Threading

| thread | exists | started from | what runs on it |
|---|---|---|---|
| Qt main thread | always | — | the GUI; every protocol message body, parsed and created, on client and server; directory registration; JSON-RPC; and the server's complete frame cycle (see below) |
| `CSocketThread` | always | `CHighPrioSocket::Start()`, at `QThread::TimeCriticalPriority` | a blocking UDP receive loop. Audio packets are decoded into the jitter buffer synchronously, in `CChannel::PutAudioData` (client) or `CServer::PutAudioData` (server). Protocol messages are split across the two threads: `CProtocol::ParseMessageFrame` validates the frame here, then the body is re-emitted as a queued signal and `ParseMessageBody` runs it on the main thread. |
| audio driver threads | client | the sound driver | the backend callback, which runs `CClient::AudioCallback`: Opus decode of the received stream, Opus encode of the sound card input, and the UDP send of the encoded packet |
| `CHighPrecisionTimer` | server, except on Windows | `CHighPrecisionTimer::Start()`, at `QThread::TimeCriticalPriority` | only `emit timeout()` once per frame, plus the absolute-time sleep that paces it |
| `CThreadPool` workers | server with `--multithreading`, on more than one core | `CServer`'s constructor | Opus decode and mix/encode/send work, in per-block chunks handed out by `CServer::OnTimer` |
| recorder thread | server with recording | `CJamController` | `CJamRecorder`, fed by queued `AudioFrame` signals from the frame cycle |
| `QThreadPool` global pool | client GUI | the connect dialog | one task per listed server for the ping/info fan-out (`QtConcurrent::run`) |

**The server's frame cycle runs on the main thread.** The `CHighPrecisionTimer` thread only
emits `timeout()`. Its queued slot, `CServer::OnTimer`, does the jitter buffer drain, decode,
mix, encode and transmit — on the main thread. The TODO in
[util.cpp](util.cpp) notes the same escape from the timer thread. On Windows the pacer is a
plain `QTimer`, also main thread. With `--multithreading` the heavy blocks go to the pool, but
`OnTimer` waits for them. On a machine reporting one core, `CServer`'s constructor turns the
option back off, so no pool thread is created at all.

#### Locks

The locks taken from more than one thread:

| lock | protects | taken from |
|---|---|---|
| `CChannel::MutexSocketBuf` | the jitter buffer | put on `CSocketThread`; get from the client's driver callback or the server's frame cycle; re-init from the main thread |
| `CSocket::Mutex` | the send path of the shared UDP socket | every `SendPacket()` call: the driver callback (client), the frame cycle and pool workers (server), and protocol code on the main thread |
| `CServer::Mutex` | connect and disconnect of channels against the frame cycle | `CServer::OnTimer` holds it while it collects the connected channels and drains and decodes their jitter buffers, and releases it before mix and send; `CServer::PutAudioData` (`CSocketThread`) and the protocol slots (main thread) take it too |
| `CChannel::Mutex` | per-channel state: the enable flag, gain and pan tables, name | setters in protocol slots on the main thread; getters in the server's frame cycle |
| `CChannel::MutexConvBuf` | the send-side conversion buffer | `PrepAndSendPacket()` on the sending thread; re-init from the main thread |

**Smaller locks:**

- `CProtocol::Mutex` — queue of sent but not yet acknowledged messages
- `CServer::MutexChanOrder` — channel allocation in `FindChannel` and `FreeChannel`
- `CServer::MutexWelcomeMessage` — the welcome message string. Only `OnNewConnection` and
  `SetWelcomeMessage` lock it. `OnCLReqServerFeatures`, `OnCLReqWelcomeMessage` and
  `GetWelcomeMessage` read it without the lock, and all three run on the main thread.
- `CClient::MutexChannels` — client-side channel number map
- `CClient::MutexGainOrPan` — gain/pan message rate limiter
- `CClient::MutexDriverReinit` — serializes sound device re-initialization
- Sound layer locks (`MutexAudioProcessCallback`, `MutexDevProperties`, per-backend) — see [sound/README.md](sound/README.md)
