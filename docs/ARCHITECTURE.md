### Copyright (c) 2026

Author(s):
* The Jamulus Development Team

Licensed under AGPL 3.0 or any later version. See [COPYING](../COPYING) for details.

---

# Jamulus Architecture

Jamulus is a client–server system for playing music together in real time. Each client captures local audio, OPUS-encodes it and sends it to a server over UDP. The server decodes every connected client, produces an individual mix for each one, re-encodes and sends it back. All audio flows through the server; clients never exchange audio with each other. One binary contains both roles — `src/main.cpp` starts a client or (with `--server`) a server.

Audio is processed in frames of 64 or 128 samples at 48 kHz — one UDP packet roughly every 1.3–2.7 ms in each direction, per client. That timescale is why the threading rules below are strict: a delay of a few milliseconds anywhere in the path is audible to everyone.

## Source map

| Files | Responsibility |
| --- | --- |
| `src/main.cpp` | Command-line parsing; instantiates client or server |
| `src/client.*` | `CClient`: connects sound card, codec and network on the client side |
| `src/server.*` | `CServer`: the mix engine and channel bookkeeping |
| `src/channel.*` | `CChannel`: one connected client as seen by the server — jitter buffer, gains, per-channel protocol state |
| `src/protocol.*` | `CProtocol`: message framing, acknowledgement/retransmission, split messages |
| `src/socket.*` | `CSocket` / `CHighPrioSocket`: UDP I/O and packet classification |
| `src/buffer.*` | `CNetBuf` / `CNetBufWithStats`: the jitter buffer, including automatic sizing |
| `src/serverlist.*` | `CServerListManager`: directory registration and server lists (both the registering-server and the directory role) |
| `src/serverlogging.*`, `src/recorder/` | Server logging and the jam recorder |
| `src/rpcserver.*`, `src/clientrpc.*`, `src/serverrpc.*` | JSON-RPC interface (see [JSON-RPC.md](JSON-RPC.md)) |
| `src/sound/` | Audio backends (JACK, ASIO, CoreAudio, Oboe), all derived from `CSoundBase` |
| `src/util.*`, `src/settings.*` | Shared helpers and persistent settings |
| `src/*dlg*`, `src/audiomixerboard.*` | Qt GUI — excluded from `CONFIG+=headless` builds |

## Threading model

The server runs three execution contexts:

1. **Socket thread** (`CHighPrioSocket`, started with `QThread::TimeCriticalPriority`): a receive loop classifies each datagram (see below). Audio packets are pushed into the owning channel's jitter buffer directly on this thread; protocol messages are re-emitted as queued Qt signals to the main thread.
2. **Mix timer** (`CHighPrecisionTimer` → `CServer::OnTimer`): the server's heartbeat. Every system frame it pulls one block per connected channel from the jitter buffers, decodes, mixes a personal output for each client, encodes and sends.
3. **Qt main event loop**: protocol message handling, chat, directory registration, JSON-RPC, GUI.

The client is analogous: the sound backend's hardware callback (`CSoundBase`) drives encode → send and receive → decode, a socket thread handles incoming packets, and the main thread runs GUI and protocol.

**Invariant 1 — never block the real-time path.** Contexts 1 and 2 and the sound callback must not perform blocking I/O (disk, DNS, HTTP, database), wait on locks contended by non-real-time threads, or allocate unboundedly per packet. Slow work belongs on the main thread or an async helper; when its result is not ready, the real-time path fails open and carries on. One synchronous lookup here freezes the audio of everyone on the server.

## How a "connection" works

There is no handshake — the protocol is pure UDP. For every received datagram, `CSocket::ProcessPacket()` first tries to parse it as a protocol frame (two zero tag bytes, valid CRC); anything that fails the parse is treated as audio and routed to a channel by source address. A valid audio packet from an unknown address *is* the connection request: the server allocates a channel for it. The channel times out when audio stops arriving (`CChannel::IsConnected()`); an orderly disconnect is the client repeating `CLM_DISCONNECTION` until the server stops streaming to it. The protocol setup exchange (client ID, transport properties, channel info) then runs concurrently with audio, in no guaranteed order — see [JAMULUS_PROTOCOL.md](JAMULUS_PROTOCOL.md).

## The wire protocol is a compatibility contract

Messages come in two classes:

- **Connection-based** (ID < 1000): carried per-channel by a `CProtocol` instance, sequence-counted, acknowledged and retransmitted until acked.
- **Connectionless** (`CLM_*`, ID ≥ 1000): no channel, no acknowledgement — used where no connection exists yet: ping, directory registration, server list queries, NAT hole punching.

The protocol evolves by adding new message IDs: released clients and servers in the wild must keep interoperating with newer ones. Read the header comment in `src/protocol.cpp` and [JAMULUS_PROTOCOL.md](JAMULUS_PROTOCOL.md) before changing protocol code.

**Invariant 2 — all network input is untrusted.** Packets arrive from arbitrary internet hosts. Bounds-check every length and count field before use; a malformed packet must be dropped silently — it must never crash, block, or corrupt state.

## Directory servers

A directory is a Jamulus server acting as a registry. Ordinary servers register with it (`CLM_REGISTER_SERVER_EX`) and re-register periodically as a keepalive; clients ask it for the server list (`CLM_REQ_SERVER_LIST`). Because listed servers may sit behind NAT, the directory also brokers hole punching: it asks a server to send `CLM_SEND_EMPTY_MESSAGE` to a client's address so the client's follow-up packets can get through. Both sides of this are implemented in `CServerListManager`.

## Where common changes go

- **New protocol message**: add the `PROTMESSID_*` define in `protocol.h`, the create/parse functions in `protocol.cpp`, wire the signal into `channel`/`client`/`server`, and document it in [JAMULUS_PROTOCOL.md](JAMULUS_PROTOCOL.md).
- **New sound backend**: subclass `CSoundBase` under `src/sound/`.
- **New JSON-RPC method**: `clientrpc.*`/`serverrpc.*`, then regenerate [JSON-RPC.md](JSON-RPC.md) with `tools/generate_json_rpc_docs.py`.
- **GUI changes**: keep `CONFIG+=headless serveronly` building — no GUI code may be required by the core.
