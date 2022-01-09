
The Jamulus Audio Protocol
==

Jamulus uses connectionless UDP packets to communicate between the client and server, and additionally for directory server registration. The `src/protocol.cpp` file contains much of the details of the packets themselves, whereas this document is intended to form a higher-level view of the protocol interactions.  
Some of the messages need to be acknowledged, some do not.

All of this information can be discovered from reading the code, but hopefully is quicker to digest when available in one location.

Overview
---

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

---


Client Session with a Server
--
When a client starts a session with a server, it should start sending valid audio packets to the server port.

The server sends a `CLIENT_ID (0x2000)` message, with the client's server identifier. This is ACK'd.    
The server sends a `CONN_CLIENTS_LIST (0x1800)` message, to reset the client's interface. This is ACK'd.  
The server sends a `REQ_SPLIT_MESSAGE_SUPPORT (0x2200)` message. This is ACK'd.    
The client sends a `SPLIT_MESS_SUPPORTED (0x2300)` message, which is ACK'd by the server.        

The server sends a `REQ_NETW_TRANSPORT_PROPS (0x1500)` message, which is ACK'd.    
The client sends a `NETW_TRANSPORT_PROPS (0x1400)` message, with details of the sample rate, channels and codec. This is ACK'd.    

The server sends a `REQ_JITT_BUF_SIZE (0x0B00)` message, which is ACK'd.    
The client sends a `JITT_BUF_SIZE (0x0a00)` message, to set the server's jitter buffer size. The server ACKs this.    
The server sends a `REQ_CHANNELS_INFOS (0x1700)` message, which is ACK'd.    
The client sends a `CHANNEL_INFOS (0x1900)` message, with details of the channel strip. The server ACKs this.    

The server may send a `CHAT_TEXT (0x1200)` message, with the contents of the welcome message. This is ACK'd.    

The client sends a `CHANNEL_INFOS (0x1900)` message, with details of the channel strip. The server ACKs this.    
The server sends a `VERSION_AND_OS (0x1d00)` message, which the client ACK's.    

The client sends a `REQ_CONN_CLIENT_LIST (0x1000)` message, which the server ACK's.  
The server responds with a `CONN_CLIENTS_LIST (0x1800)` message, which the client ACK's    
The server sends a `RECORDER_STATE (0x2100)` message, which the client ACK's.  

---

General Streaming Messages
--

During streaming, the server will periodically send some control messages to the clients.  
These are:  
`CLM_CHANNEL_LEVEL_LIST (0xf703)` - to update the VU meters of the channels.  
`JITT_BUF_SIZE (0x0a00)` - to change the server-side jitter buffer count. This is ACK'd.  
`CLM_PING_MS (0xe903)` - to ascertain round trip delay.  
`NETW_TRANSPORT_PROPS (0x1400)` - to change the buffer sizes, channels etc. This is ACK'd.  
`CHANNEL_GAIN (0x0d00)` - to set the volume level of the chanel on the server for that channel.