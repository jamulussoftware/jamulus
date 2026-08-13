### Copyright (c) 2026

Author(s):
* mcfnord
* The Jamulus Development Team

As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
under AGPL 3.0 or any later version.

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

# Where things live

Code used by both client and server:

- [main.cpp](main.cpp) parses the command line and constructs a `CClient` or a `CServer`.
- [protocol.cpp](protocol.cpp) — `CProtocol`: message framing, acknowledgement and
  retransmission. Wire format: [../docs/JAMULUS_PROTOCOL.md](../docs/JAMULUS_PROTOCOL.md).
- [channel.cpp](channel.cpp) — `CChannel`: the connection and its receive jitter buffer, used by
  both client and server. The client has one; the server an array of `MAX_NUM_CHANNELS`.
- [socket.cpp](socket.cpp) — `CSocket` and `CHighPrioSocket`: the UDP socket shared by all
  sending and receiving, with its receive thread.
- [buffer.h](buffer.h) — `CNetBuf` and `CNetBufWithStats`: the jitter buffer, including the
  automatic size decision.
- [util.h](util.h) / [util.cpp](util.cpp) — `CHighPrecisionTimer`, the server's frame clock, and
  assorted helpers.

Client only: [client.cpp](client.cpp) (`CClient`), the sound layer in [sound/](sound/), the GUI
([clientdlg.cpp](clientdlg.cpp), [clientsettingsdlg.cpp](clientsettingsdlg.cpp),
[audiomixerboard.cpp](audiomixerboard.cpp), [connectdlg.cpp](connectdlg.cpp),
[chatdlg.cpp](chatdlg.cpp)), and [clientrpc.cpp](clientrpc.cpp).

Server only: [server.cpp](server.cpp) (`CServer`: the channels and the mix),
[serverlist.cpp](serverlist.cpp) (directory registration and the server list),
[recorder/](recorder/), [serverlogging.cpp](serverlogging.cpp),
[serverrpc.cpp](serverrpc.cpp) and [serverdlg.cpp](serverdlg.cpp).

The JSON-RPC API ([rpcserver.cpp](rpcserver.cpp), [clientrpc.cpp](clientrpc.cpp),
[serverrpc.cpp](serverrpc.cpp)) is documented in [../docs/JSON-RPC.md](../docs/JSON-RPC.md).

## Threads

| thread | exists | started from | what runs on it |
|---|---|---|---|
| Qt main thread | always | — | the GUI; every protocol message body, parsed and created, on client and server; directory registration; JSON-RPC; and the server's complete frame cycle (see below) |
| `CSocketThread` | always | `CHighPrioSocket::Start()`, at `QThread::TimeCriticalPriority` | a blocking UDP receive loop. Audio packets are decoded into the jitter buffer synchronously, in `CChannel::PutAudioData` (client) or `CServer::PutAudioData` (server). Protocol messages are split across the two threads: `CProtocol::ParseMessageFrame` validates the frame here, then the body is re-emitted as a queued signal and `ParseMessageBody` runs it on the main thread. |
| audio driver threads | client | the sound driver | the backend callback, which runs `CClient::AudioCallback`: Opus decode of the received stream, Opus encode of the sound card input, and the UDP send of the encoded packet |
| `CHighPrecisionT…` | server, except on Windows | `CHighPrecisionTimer::Start()`, at `QThread::TimeCriticalPriority` | only `emit timeout()` once per frame, plus the absolute-time sleep that paces it |
| `CThreadPool` workers | server with `--multithreading`, on more than one core | `CServer`'s constructor | Opus decode and mix/encode/send work, in per-block chunks handed out by `CServer::OnTimer` |
| recorder thread | server with recording | `CJamController` | `CJamRecorder`, fed by queued `AudioFrame` signals from the frame cycle |
| `QThreadPool` global pool | client GUI | the connect dialog | one task per listed server for the ping/info fan-out (`QtConcurrent::run`) |

**The server's frame cycle runs on the main thread.** The `CHighPrecisionTimer` thread only
emits `timeout()`; the slot behind that queued connection, `CServer::OnTimer`, does the jitter
buffer drain, decode, mix, encode and transmit — on the main thread. The TODO in
[util.cpp](util.cpp) notes the same escape from the timer thread. On Windows the pacer is a
plain `QTimer`, also main thread. With `--multithreading` the heavy blocks go to the pool, but
`OnTimer` waits for them — and on a machine reporting one core, `CServer`'s constructor turns the
option back off, so no pool thread is created at all.

## Locks

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
- `CServer::MutexWelcomeMessage`
- `CClient::MutexChannels` — client-side channel number map
- `CClient::MutexGainOrPan` — gain/pan message rate limiter
- `CClient::MutexDriverReinit` — serializes sound device re-initialization
- Sound layer locks (`MutexAudioProcessCallback`, `MutexDevProperties`, per-backend) — see [sound/README.md](sound/README.md)

## Not yet documented

- the jitter buffer's automatic size algorithm (`CNetBufWithStats`)
- the connection lifecycle: how a channel goes from first packet to connected to timed out
- the directory: registration, the server list, and the split of
  [serverlist.cpp](serverlist.cpp) between the directory role and the registered-server role
- the recorder
- [serverlogging.cpp](serverlogging.cpp), [signalhandler.cpp](signalhandler.cpp), the GUI
  classes, and translation loading
