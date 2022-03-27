# The Jamulus Audio Protocol

Jamulus uses connectionless UDP packets to communicate between the client and server, and additionally for directory server registration. The `src/protocol.cpp` file contains much of the details of the packets themselves, whereas this document is intended to form a higher-level view of the protocol interactions.  
Some of the messages need to be acknowledged, some do not. If a message ID is less than 1000, the message must be acknowledged in under `SEND_MESS_TIMEOUT_MS` ms.

All of this information can be discovered from reading the code, but hopefully is quicker to digest when available in one location. There is a wireshark dissector available too, [here](https://github.com/softins/jamulus-wireshark), if you would like to inspect the packet flow.

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
LENgth of the data preceeds the data and is followed by a CRC for the packet.

Data is sent little-endian, i.e. not network byte-order.

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

## Client Session with a Server

As the protocol is connectionless, the message flow at session start up can happen out of order.  
When a client starts a session with a server, it sends valid audio packets to the server port, to which the server will respond with the audio mix for that client.

The server on a new client connection will:

- Tell the client connection its ID, with a `CLIENT_ID (32, 0x2000)` message.
- Reset the connected client list with a `CONN_CLIENTS_LIST (24, 0x1800)` message.
- Determine if the client supports split messages, with a `REQ_SPLIT_MESSAGE_SUPPORT (34, 0x2200)` message.
- Request the details of the audio packets from the client with a `REQ_NETW_TRANSPORT_PROPS (21, 0x1500)` message,
- Request the number of jitter buffer value to use, with a `REQ_JITT_BUF_SIZE (11, 0x0B00)` message.
- Request the details of the channel info, with a `REQ_CHANNELS_INFOS (23, 0x1700)` message.
- Send the version and OS of the server, with a `VERSION_AND_OS (29, 0x1d00)` message.

This is defined in `CServer::OnNewConnection()`

The client on a new connection will:

- Send its channel info with a `CHANNELS_INFO (25, 0x1900)` message
- Request the list of connected clients with a `REQ_CONN_CLIENT_LIST (16, 0x1000)` message
- Set the server-side jitter buffer value with a `JITT_BUF_SIZE (10, 0x0a00)` message

This is defined in `CClient::OnNewConnection()`

At the end of the session, the client calls the `CLM_DISCONNECTION (1010, 0xf203)` message, until the server stops streaming audio to it.

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


  REQ_CONNECTED_CLIENTS_LIST (16, 0x1000) ---->
      <------------------------------------ ACK(REQ_CONNECTED_CLIENTS_LIST)

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

## General Streaming Messages

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

## Audio Packet Structure

The OPUS codec is used to compress the audio over the network and the packets are documented [here](https://datatracker.ietf.org/doc/html/rfc6716).

Jamulus uses a custom OPUS encoder / decoder, giving some different frame sizes, but always uses a 48kHz sample rate. OPUS and OPUS64 codecs are the only supported options currently.

The packet size will vary based on:

- Stereo vs mono
- Buffer size (64/128/256 samples)
- Use of frame sequence number (from v3.6.0 onwards)

These values are wrapped up into the `NETW_TRANSPORT_PROPS` messages, which the client sends to the server to tell it which values to use.

Both client and server use a jitter buffer for received audio data to prevent audio drop-out. This is configurable.
