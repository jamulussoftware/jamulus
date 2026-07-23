### Copyright (c) 2022-2026

Author(s):
* Emlyn Bolton
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

# The Jamulus audio protocol

Jamulus uses connectionless UDP packets to communicate between the Client and Server, and additionally for registration with a Directory. The `src/protocol.cpp` file contains much of the details of the packets themselves, whereas this document is intended to form a higher-level view of the protocol interactions.
Messages with an ID below 1000 are connection-based: each one is acknowledged by an `ACKN (1)` message carrying the same sequence counter, and the sender retransmits it every `SEND_MESS_TIMEOUT_MS` ms until the acknowledgement arrives. Messages with an ID from 1000 to 1999 (`CLM_*`) are connectionless: they work without an established audio connection and are never acknowledged.

All of this information can be discovered from reading the code, but hopefully is quicker to digest when available in one location. There is a Wireshark dissector available too, [here](https://github.com/softins/jamulus-wireshark), if you would like to inspect the packet flow.

---

## Overview

The message packet structure is:

```
+-------------+------------+------------+------------------+--------------+-------------+
| 2 bytes TAG | 2 bytes ID | 1 byte SEQ | 2 bytes data LEN | n bytes DATA | 2 bytes CRC |
+-------------+------------+------------+------------------+--------------+-------------+
```

The TAG bytes are zero bytes.
The ID provides the message type.
The SEQ is a wrapping sequence number for the message
LENgth of the data precedes the data and is followed by a CRC for the packet.

Data is sent little-endian, i.e. not network byte-order.

The CRC is 16 bits, generator polynomial x¹⁶ + x¹² + x⁵ + 1 (CCITT), initial state all ones, calculated over the entire message and transmitted inverted.

Audio and protocol messages share one UDP port. A receiver classifies every incoming datagram by attempting to parse it as a protocol frame — zero TAG bytes, consistent length, valid CRC. Anything that fails this parse is treated as an audio packet (see `CSocket::ProcessPacket()` in `src/socket.cpp`).

Where a message will not fit into the maximum packet size before fragmentation, a split message container is used.

```
+------------+--------------+----------------+--------------+
| 2 bytes ID | 1 byte FRAGS | 1 byte FRAG_ID | n bytes DATA |
+------------+--------------+----------------+--------------+
```

The ID is the message type sent in fragments
FRAGS is the total number of fragments
FRAG_ID is the sequence number of the data in this fragment
DATA is the fragment data to be re-assembled

This forms the data component of the packet above.

## Message reference

Connection-based messages (acknowledged; `PROTMESSID_` prefix omitted). Full payload layouts are in the header comment of `src/protocol.cpp`.

| ID | Name | Purpose |
|---|---|---|
| 1 | `ACKN` | Acknowledges the message ID/counter it carries |
| 10 | `JITT_BUF_SIZE` | Set Jitter Buffer size |
| 11 | `REQ_JITT_BUF_SIZE` | Request Jitter Buffer size |
| 13 | `CHANNEL_GAIN` | Set a Channel's gain in your mix |
| 16 | `REQ_CONN_CLIENTS_LIST` | Request list of connected Clients |
| 18 | `CHAT_TEXT` | Chat text |
| 20 | `NETW_TRANSPORT_PROPS` | Audio transport properties |
| 21 | `REQ_NETW_TRANSPORT_PROPS` | Request audio transport properties |
| 23 | `REQ_CHANNEL_INFOS` | Request Channel info (name, instrument, …) |
| 24 | `CONN_CLIENTS_LIST` | Channel info of all connected Clients |
| 25 | `CHANNEL_INFOS` | Set own Channel info |
| 26 | `OPUS_SUPPORTED` | OPUS codec is supported |
| 27 | `LICENCE_REQUIRED` | Server requires licence agreement |
| 29 | `VERSION_AND_OS` | Version and operating system |
| 30 | `CHANNEL_PAN` | Set a Channel's pan in your mix |
| 31 | `MUTE_STATE_CHANGED` | Your signal was (un)muted at another Client |
| 32 | `CLIENT_ID` | Your Channel ID on the Server |
| 33 | `RECORDER_STATE` | Jam recorder state |
| 34 | `REQ_SPLIT_MESS_SUPPORT` | Request split-message support |
| 35 | `SPLIT_MESS_SUPPORTED` | Split messages are supported |
| 36 | `RAWAUDIO_SUPPORTED` | Raw (uncompressed) audio is supported |

IDs 12, 14, 15, 17, 19, 22 and 28 are legacy messages no longer sent (28, `REQ_CHANNEL_LEVEL_LIST`, is still understood for compatibility with Servers 3.4.6–3.5.12).

`SPECIAL_SPLIT_MESSAGE (2001)` sits outside both ID ranges because it is not a message in its own right: it is the transport container for the fragments of an oversized connection-based message (see the split message container above), with the original message ID carried inside the container. Each fragment frame has its own sequence counter and is acknowledged and retransmitted like any connection-based message.

Connectionless messages (never acknowledged):

| ID | Name | Purpose |
|---|---|---|
| 1001 | `CLM_PING_MS` | Ping time measurement |
| 1002 | `CLM_PING_MS_WITHNUMCLIENTS` | Ping plus number of connected Clients |
| 1003 | `CLM_SERVER_FULL` | Server is full |
| 1004 | `CLM_REGISTER_SERVER` | Register with a Directory |
| 1005 | `CLM_UNREGISTER_SERVER` | Unregister from a Directory |
| 1006 | `CLM_SERVER_LIST` | Full Server list |
| 1007 | `CLM_REQ_SERVER_LIST` | Request Server list |
| 1008 | `CLM_SEND_EMPTY_MESSAGE` | Ask recipient to send `CLM_EMPTY_MESSAGE` to the carried address |
| 1009 | `CLM_EMPTY_MESSAGE` | Empty message (NAT hole punching) |
| 1010 | `CLM_DISCONNECTION` | Disconnect from Server |
| 1011 | `CLM_VERSION_AND_OS` | Version and operating system |
| 1012 | `CLM_REQ_VERSION_AND_OS` | Request version and operating system |
| 1013 | `CLM_CONN_CLIENTS_LIST` | Connected Clients info |
| 1014 | `CLM_REQ_CONN_CLIENTS_LIST` | Request connected Clients info |
| 1015 | `CLM_CHANNEL_LEVEL_LIST` | Channel level list |
| 1016 | `CLM_REGISTER_SERVER_RESP` | Registration result |
| 1017 | `CLM_REGISTER_SERVER_EX` | Register with extended information |
| 1018 | `CLM_RED_SERVER_LIST` | Reduced Server list (less UDP fragmentation) |
| 1019 | `CLM_SERVER_FEATURES` | Server features |
| 1020 | `CLM_REQ_SERVER_FEATURES` | Request Server features |
| 1021 | `CLM_WELCOME_MESSAGE` | Server welcome message |
| 1022 | `CLM_REQ_WELCOME_MESSAGE` | Request Server welcome message |

## Client session with a Server

As the protocol is connectionless, the message flow at session start up can happen out of order.
When a Client starts a session with a Server, it sends valid audio packets to the Server port, to which the Server will respond with the audio mix for that Client.

The Server on a new Client connection will:

- Tell the Client connection its ID, with a `CLIENT_ID (32, 0x2000)` message.
- Reset the connected Client list with a `CONN_CLIENTS_LIST (24, 0x1800)` message.
- Determine if the Client supports split messages, with a `REQ_SPLIT_MESSAGE_SUPPORT (34, 0x2200)` message.
- Request the details of the audio packets from the Client with a `REQ_NETW_TRANSPORT_PROPS (21, 0x1500)` message,
- Request the Jitter Buffer value to use, with a `REQ_JITT_BUF_SIZE (11, 0x0B00)` message.
- Request the details of the Channel info, with a `REQ_CHANNEL_INFOS (23, 0x1700)` message.
- Send the version and OS of the Server, with a `VERSION_AND_OS (29, 0x1d00)` message.

This is defined in `CServer::OnNewConnection()`

The Client on a new connection will:

- Send its Channel info with a `CHANNEL_INFOS (25, 0x1900)` message
- Request the list of connected Clients with a `REQ_CONN_CLIENTS_LIST (16, 0x1000)` message
- Set the Server-side Jitter Buffer value with a `JITT_BUF_SIZE (10, 0x0a00)` message

This is defined in `CClient::OnNewConnection()`

At the end of the session, the Client repeatedly sends a `CLM_DISCONNECTION (1010, 0xf203)` message, until the Server stops streaming audio to it.

A typical flow would be:

```
 Client                                     Server

  AUDIO --------------------------------->
      <------------------------------------ CLIENT_ID (32, 0x2000)
  ACK(CLIENT_ID) ------------------------>

      <------------------------------------ CONN_CLIENTS_LIST (24, 0x1800) (Reset to zero)
  ACK(CONN_CLIENTS_LIST) ---------------->

      <------------------------------------ REQ_SPLIT_MESSAGE_SUPPORT (34, 0x2200)
  SPLIT_MESS_SUPPORTED (35, 0x2300) --------->
  ACK(REQ_SPLIT_MESSAGE_SUPPORT) -------->
      <------------------------------------ ACK(SPLIT_MESS_SUPPORTED)

      <------------------------------------ REQ_NETW_TRANSPORT_PROPS (21, 0x1500)
  NETW_TRANSPORT_PROPS (20, 0x1400) --------->
  ACK(REQ_NETW_TRANSPORT_PROPS) --------->
      <------------------------------------ ACK(NETW_TRANSPORT_PROPS)

      <------------------------------------ REQ_JITT_BUF_SIZE (11, 0x0B00)
  JITT_BUF_SIZE (10, 0x0a00) ---------------->
  ACK(REQ_JITT_BUF_SIZE) ---------------->
      <------------------------------------ ACK(JITT_BUF_SIZE)

      <------------------------------------ REQ_CHANNELS_INFOS (23, 0x1700)
  CHANNEL_INFOS (25, 0x1900) ---------------->
  ACK(REQ_CHANNELS_INFOS) --------------->
      <------------------------------------ ACK(CHANNEL_INFOS)

(Optional welcome message)
      <------------------------------------ CHAT_TEXT (18, 0x1200)
  ACK(CHAT_TEXT) ------------------------>

      <------------------------------------ VERSION_AND_OS (29, 0x1d00)
  ACK(VERSION_AND_OS) ------------------->

  CHANNEL_INFOS (25, 0x1900) ---------------->
      <------------------------------------ ACK(CHANNEL_INFOS)

      <------------------------------------ RECORDER_STATE (33, 0x2100)
  ACK(RECORDER_STATE) ------------------->


  REQ_CONN_CLIENTS_LIST (16, 0x1000) -------->
      <------------------------------------ ACK(REQ_CONN_CLIENTS_LIST)

  REQ_CHANNEL_LEVEL_LIST (28, 0x1c00) -------->

  JITT_BUF_SIZE (10, 0x0a00) ---------------->
      <------------------------------------ JITT_BUF_SIZE (10, 0x0a00)
      <------------------------------------ ACK(JITT_BUF_SIZE)
  ACK(JITT_BUF_SIZE) -------------------->

      <------------------------------------ CONN_CLIENTS_LIST (24, 0x1800)
  ACK(CONN_CLIENTS_LIST) ---------------->

  NETW_TRANSPORT_PROPS (20, 0x1400) --------->
      <------------------------------------ CONN_CLIENTS_LIST (24, 0x1800)
      <------------------------------------ ACK(NETW_TRANSPORT_PROPS)
  ACK(CONN_CLIENTS_LIST) ---------------->
```

## General streaming messages

During streaming, some control messages are used.
Some typical messages could be:

```
 Client                                     Server

      <------------------------------------ CLM_CHANNEL_LEVEL_LIST (1015, 0xf703)

  CHANNEL_GAIN (13, 0x0d00) ----------------->

  CLM_PING_MS (1001, 0xe903) ------------------>

      <------------------------------------ ACK(CHANNEL_GAIN)

      <------------------------------------ CLM_PING_MS (1001, 0xe903)

  MUTE_STATE_CHANGED (31, 0x1f00) ----------->
      <------------------------------------ ACK(MUTE_STATE_CHANGED)

  NETW_TRANSPORT_PROPS (20, 0x1400) --------->
      <------------------------------------ ACK(NETW_TRANSPORT_PROPS) - Reset audio packet sequencing on change

  CHANNEL_PAN (30, 0x1e00) ------------------>
      <------------------------------------ ACK(CHANNEL_PAN)
```

---

## Directory registration and Server lists

A Directory is a Jamulus Server acting as a registry (implemented in `src/serverlist.cpp`, both roles). All Directory traffic uses connectionless messages:

- A Server registers with `CLM_REGISTER_SERVER_EX (1017)` (older versions: `CLM_REGISTER_SERVER (1004)`) and receives `CLM_REGISTER_SERVER_RESP (1016)` carrying the result (registered, list full, version too old, requirements not fulfilled). If no response arrives, registration is retried every 500 ms, up to 5 times.
- Registration is refreshed every 15 minutes; the Directory drops a Server it has not heard from for 33 minutes. `CLM_UNREGISTER_SERVER (1005)` removes the entry immediately at shutdown or when changing Directory through the Server UI.
- A Client requests the list with `CLM_REQ_SERVER_LIST (1007)`. The Directory answers with both `CLM_RED_SERVER_LIST (1018)` (a shorter form that reduces UDP fragmentation) and `CLM_SERVER_LIST (1006)` (the full information). The Client then pings each listed Server with `CLM_PING_MS_WITHNUMCLIENTS (1002)` to display latency and occupancy.
- NAT hole punching: when it answers a list request, the Directory also sends every registered Server a `CLM_SEND_EMPTY_MESSAGE (1008)` carrying the Client's public address; each Server responds by sending `CLM_EMPTY_MESSAGE (1009)` to that address, opening its own NAT/firewall for the Client's subsequent packets. The Directory and its registered Servers also ping each other about once a minute to keep their NAT mappings alive.

---

## Audio packet structure

The OPUS codec is used to compress the audio over the network and the packets are documented [here](https://datatracker.ietf.org/doc/html/rfc6716).

Jamulus uses a custom OPUS encoder / decoder, giving some different frame sizes, but always uses a 48 kHz sample rate. OPUS and OPUS64 codecs are the only supported options currently.

The packet size will vary based on:

- Stereo vs mono
- Buffer size (64/128/256 samples)
- Use of frame sequence number (from v3.6.0 onwards)

These values are wrapped up into the `NETW_TRANSPORT_PROPS` messages, which the Client sends to the Server to tell it which values to use.

Both Client and Server use a Jitter Buffer for received audio data to prevent audio drop-out. This is configurable.
