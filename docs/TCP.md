# TCP FALLBACK FOR THE JAMULUS PROTOCOL

## THE PROBLEM BEING SOLVED

All Jamulus protocol (non-audio) messages are currently delivered over the same UDP channel as the audio. For most protocol messages, this is fine, but those that send a list of servers from a directory, or a list of clients from a server, can generate a UDP datagram that is too large to fit into a single physical packet. Physical packets are constrained by the MTU of the Ethernet interface (normally 1500 bytes or less), and further by any limitations in links between hops on the internet. Neither the client nor the server has any control over these limitation. It's also possible a large welcome message could require fragmentation.

The UDP protocol itself allows datagrams up to be up to nearly 65535 bytes in size, minus any protocol overhead. IPv4 will allow nearly all of this size to be used, in theory. If the IPv4 datagram being sent by a node (host or router) is too large to fit into a single packet on the outgoing interface, the IP protocol will fragment the packet into pieces that do fit, with IP headers that contain the information needed to order and reassemble the fragments into a single datagram at the receiving end. Normally intermediate hops do not perform any reassembly, but will further fragment an IP packet if it will not fit the MTU of the outgoing interface.

The receiving end's IP stack needs to store all the received fragments as they arrive and can only reassemble them into the original datagram once all fragments have been received. The loss of even one fragment renders the whole datagram lost, and the remaining received fragments consume resources until they time out and are discarded. There are also possibilities for a denial of service attack if an attacker deliberately sends lots of fragments with one or more missing.

If a directory has more than around 35 servers registered (depending on the length of the name, city, etc.), the list of servers sent to a client when requested is certain to be fragmented. Similarly, if a powerful server has a lot of clients connected, e.g. a big band or large choir, the list of clients sent to each connected client could be large enough to get fragmented. In either of these cases, a client that is unable to receive fragmented IP packets will show an empty list or an empty mixer panel.

There are several reasons that fragmented IP datagrams can fail to make it from server to client:

* The configuration of a user's router, either accidentally or deliberately. Sometimes a user can be helped by a knowledgeable friend to check and fix this, but often not.
* The configuration of an intermediate router along the path from server to client. This is fairly rare, but could be a carrier's deliberate choice to avoid the kind of DoS attack mentioned above. For whatever reason, it is outside the control of the user or server operator.
* The IPv6 protocol deliberately has no provision for fragmentation of datagrams at the IP layer of intermediate hops. So this is a complete show-stopper for the use of IPv6 in directories, as there is therefore no support at all for large UDP messages.

The IPv6 limitation means that resolving this issue is a prerequisite to implementing IPv6 support in directories.

## CONNECTIONLESS MODE - CLIENT CONNECT DIALOG

The basic summary is that TCP need only be used as a fallback when it is determined that a UDP message from a directory or server failed to reach the client, probably due to fragmentation, _and_ that the directory or server explicitly supports TCP.

### Current operation when client opens Connect dialog

1. Client sends `CLM_REQ_SERVER_LIST` to the selected directory server to ask for a list of registered servers. It then starts a 2.5 sec re-request timer.

2. Directory server fetches its internal list of registered servers, and sends a `CLM_SEND_EMPTY_MESSAGE` to each listed server, with the IP and UDP port of the requesting client as parameters.

3. Directory server sends `CLM_RED_SERVER_LIST` (reduced server list) to client.

   a. If/when client receives `CLM_RED_SERVER_LIST`, it populates its list of servers with the reduced info, and sets an internal flag to say it has done so. It checks this flag to avoid processing a repeat reduced server list.

   b. If the list is large and fragmented, and the path does not correctly pass fragments, the client will not receive the list.

4. Directory server sends `CLM_SERVER_LIST` to client. It does this immediately after sending the reduced list above.

   a. If/when the client receives `CLM_SERVER_LIST`, it populates its list of servers with the full info, replacing any existing list that contained reduced info. It then stops the 2.5 sec re-request timer mentioned above, so that the server list is not requested again.

   b. If the list is large and fragmented, and the path does not correctly pass fragments, the client will not receive the list, and the request timer will be left running to retry.

5. While client is displaying the server list, it periodically pings each server with `CLM_PING_MS_WITHNUMCLIENTS` including a timestamp in the message.

6. Each pinged server, when it receives the ping, will create a `CLM_PING_MS_WITHNUMCLIENTS` in reply, containing a copy of the received timestamp, and the number of clients currently connected to that server.

7. When the client receives the reply, it can calculate the round-trip time from the received timestamp and the current time.

8. If the number of connected clients returned is different from the previously received number for that server, the client sends a `CLM_REQ_CONN_CLIENTS_LIST` to the server.

   a. The server responds with a list of clients in a `CLM_CONN_CLIENTS_LIST`. Most servers only have a small number of clients connected, and this message is not large enough to need IP fragmentation.

   b. If the server is a large one with many clients connected (e.g. for a choir, big band or WorldJam green room), the client list may be large enough to be fragmented by the IP layer. In that case, it might not be received by the requesting client, although in most cases it will.

9. If/when the client receives the client list from the server, it can display the list of connected clients under the relevant server, if this is enabled in the GUI.

10. The five steps above (5-9) continue until the user clicks Connect or closes the dialog.

### Enhancement for TCP support

1. A server (which may also be a directory) can be configured with the command-line option `--enabletcp` to enable TCP operation.

2. If the directory server has TCP enabled, then *after* it has sent the `CLM_RED_SERVER_LIST` and `CLM_SERVER_LIST` by UDP, it will send a new message `CLM_TCP_SUPPORTED` to the client, with a data field of `CLM_SERVER_LIST`. This data field enables the client to know which request may need to be retried over TCP.

   a. An older version of client that does not support TCP will ignore this message and continue operating in the normal way just on UDP.

   b. A newer client that supports TCP should receive and process the `CLM_TCP_SUPPORTED` message *after* it has received and processed the UDP server list, unless fragmentation (or another cause) prevented the list from arriving.

   c. If such a client has already processed a full server list from `CLM_SERVER_LIST`, it will have no need to open a TCP connection to the directory, so this will be skipped.

   d. If the client receives `CLM_TCP_SUPPORTED` having *not* received and processed a `CLM_SERVER_LIST`, it will open a TCP connection to the directory server, and request the server list again over the TCP connection.

3. If the directory server accepts a TCP connection and receives a `CLM_REQ_SERVER_LIST` over it, it will process the request in the same way as for a UDP request, with the following differences:

   a. There is no need for the directory to send `CLM_SEND_EMPTY_MESSAGE` to the servers in the list, since that was already done in response to the original UDP request.

   b. There is no need for the directory to send `CLM_RED_SERVER_LIST` to the client, since the TCP connection is reliable, so the directory server just sends the `CLM_SERVER_LIST` over the TCP connection.

4. When the client has received the `CLM_SERVER_LIST` over TCP, it closes the TCP connection, populates its list of servers in the connect dialog in the normal way and stops the 2.5 sec re-request timer.

5. The client starts pinging each listed server as normal, using UDP, and the server responds with a ping including the timestamp and number of clients, as described above.

6. As above, if the number of connected clients has changed, the client sends a `CLM_REQ_CONN_CLIENTS_LIST` over UDP in the normal way.

7. If a server in the list supports TCP, when it has sent a reply to the client list request with `CLM_CONN_CLIENTS_LIST`, it will follow it immediately with a `CLM_TCP_SUPPORTED`, with a data field of `CLM_CONN_CLIENTS_LIST`.

   a. An older version of client that does not support TCP will ignore the `CLM_TCP_SUPPORTED` message and continue operating in the normal way just on UDP.

   b. A newer client that supports TCP should received the `CLM_TCP_SUPPORTED` message *after* it has received and processed the UDP client list, unless fragmentation (or another cause) prevented the list from arriving.

   c. If such a client has already processed a client list from `CLM_CONN_CLIENTS_LIST`, it will have no need to open a TCP connection to the server, so this will be skipped.

   d. If the client receives `CLM_TCP_SUPPORTED` having *not* received and processed a `CLM_CONN_CLIENTS_LIST`, it will open a TCP connection to the server, and request the client list again over the TCP connection.

8. If the server accepts a TCP connection and receives a `CLM_REQ_CONN_CLIENTS_LIST` over it, it will process the request in the same way as for a UDP request, but will send the reply over the TCP connection.

9. When the client has received the `CLM_CONN_CLIENTS_LIST` over TCP, it closes the TCP connection and updates the list of clients for that server in the GUI. However, it will note for that server that TCP is needed, and if/when the number of connected clients next changes while the connect dialog is still open, it will immediately request the updated list via TCP instead of UDP.


### Summary

By sending the `CLM_TCP_SUPPORTED` message immediately *after* sending a potentially large list of servers or connected clients, it allows a client easily to determine whether or not it needs to fall back to TCP without the necessity of timeouts or other delays. It will only need to use TCP if it has not already succeeded in receiving the message over UDP.

## CONNECTED MODE

### Existing operation when client clicks on Connect

All these steps use UDP only.

1. Client starts sending an audio stream to the server. This audio stream continues in parallel with the protocol exchange below.

   a. Server does not yet start sending an audio stream to the client.

2. Server sees the audio stream and looks up the source IP:port in its channel table, finding no channel that matches.

3. Server allocates a new channel in the channels array and stores the source IP:port in it. The channel index becomes the Channel ID of the connected client.

4. Server sends a `CLIENT_ID` message to the client, containing the Channel ID mentioned above.

   a. Client replies with `ACKN (CLIENT_ID)`. *Note that all messages that do not begin `CLM_` need to be acked by the receiving side. For clarity, these `ACKN` messages will not be mentioned below.*

5. Server sends `CONN_CLIENTS_LIST` to the client, containing a list of all current clients in the session.

6. Server sends `REQ_SPLIT_MESS_SUPPORT` to ask the client if it supports split messages.

7. Client sends back `SPLIT_MESS_SUPPORTED` immediately.

7. Server sends `REQ_NETW_TRANSPORT_PROPS` to ask for the clients network transport parameters.

8. Client sends `NETW_TRANSPORT_PROPS` containing the codec, packet size, number of channels, bitrate, etc.

9. Server sends `REQ_JITT_BUF_SIZE` to ask for the client's required jitter buffer sizes.

10. Client sends `JIT_BUF_SIZE`, containing the positions of the "server" jitter buffer slider in the Settings dialog. This is telling the server what size jitter buffer to use for receiving audio data from the client. (The position of the "client" jitter buffer slider is not needed by the server, as it is only used locally in the client).

11. Server sends `REQ_CHANNEL_INFOS` to ask for the identity information for the channel.

12. Client sends `CHANNEL_INFOS` containing the identity information from the user's profile settings in the client (country, instrument, skill level, name, city).

13. Now that the server has received the `CHANNEL_INFOS` from the client, it starts to send the mixed audio stream to the client.

14. Server sends `CHAT_TEXT` containing the server welcome message, if any. If there is none, this message is skipped.

15. Server sends `VERSION_AND_OS` to tell the client the version of Jamulus on the server and the server platform.

After this, messages are sent by either side when there is something to notify:

* From server to client:

  - `CLM_CHANNEL_LEVEL_LIST` - list of audio levels for each channel. Sent every 250ms by a timer.
  - `RECORDER_STATE` - current state of the server-based recording. Sent when the state changes?
  - `JITT_BUF_SIZE` - the size of the receiving jitter buffer for this connection on the server. Sent in Auto mode when the value changes.
  - `CLM_PING_MS` - sent in response to a `CLM_PING_MS` received from the client. For client-side ping time calculation.
  - `CONN_CLIENTS_LIST` - list of connected clients. Sent when the list changes due to a client connecting or leaving. This message could be large on a server with many clients.

* From client to server:

  - `CLM_PING_MS` - contains a timestamp and requests the server to send back the same timestamp, so that the round-trip time can be measured. Sent every 500ms by a timer.
  - `NETW_TRANSPORT_PROPS` - specifies codec, packet size, bitrate, etc. Sent when the user changes Audio Channels, Audio Quality, Buffer Delay or Small Network Buffers.
  - `CHANNEL_GAIN` - specifies the user's requested gain for a specific channel. Sent when the user moves a fader, but rate limited to avoid many changes in succession being sent.
  - `JITT_BUF_SIZE` - specifies the requested size of the server's jitter buffer, or Auto.
  - `CLM_DISCONNECTION` - sent when the user clicks on Disconnect, or connects to another server.


### TCP usage in Connected Mode

The connected-mode protocol messages sent over UDP are all sequence numbered and acknowledged, in order to be robust against potential packet loss. Over TCP, such packet loss will not occur, as sequencing and acknowledgement all happen at the TCP network layer.

Consequently, TCP will not be used for connected-mode protocol messages.

The reason for using a TCP connection in an active session is just to provide a reliable path for delivering a list of connected clients that could be large and subject to fragmentation (if it is sent over UDP). So the established TCP connection is only used to deliver client lists, and not other protocol messages.

Therefore, if the server has an active TCP connection from the client, it will use the connectionless `CLM_CONN_CLIENTS_LIST` message to deliver updates for the connected client list. If there is no active TCP connection, updates will be delivered using the connected-mode `CONN_CLIENTS_LIST` over UDP as at present.

So the sequence is as follows:

1. As soon as a TCP-enabled server sees audio from a new client and creates a channel for it, it will send `CLM_TCP_SUPPORTED` to the client, with a data field of `CLM_CLIENT_ID`.

2. The server will then send the connected-mode `CLIENT_ID` message as normal, containing the channel ID that has been allocated.

3. An older version of client that does not support TCP will ignore the `CLM_TCP_SUPPORTED` message and continue operating in the normal way just on UDP.

4. A newer client that supports TCP will receive the `CLM_TCP_SUPPORTED` message, and will note that the server supports TCP. The client will open a long-lived TCP connection to the server (on the same port number as UDP).

5. The server will accept the TCP connection, and will wait for the first message to arrive via that connection.

6. When a newer client receives the `CLIENT_ID` message from a server it knows supports TCP, the client will send, as its first message over the connection, a `CLM_CLIENT_ID` message containing the channel ID that it received over UDP. (`CLM_CLIENT_ID` is a newly-defined connectionless message).

7. The server will lookup the channel specified by the `CLM_CLIENT_ID` message, and *will check that the IP address of the channel matches the remote address of the TCP connection*. If it does not, it will close the connection. This prevents hijacking of a session by sending another client's ID.

8. If the TCP connection matches the client channel, the socket descriptor will be stored in the channel, and the channel pointer will be stored in the TCP Connection instance.

9. Any messages from the client that arrive over TCP will be handled in the same way as messages received over UDP. Responses will be send back over TCP too. At present, there are no such messages defined. Existing protocol messages will continue to use UDP.

10. Updates to the Connected Clients List generated by the server will be sent over TCP as `CLM_CONN_CLIENTS_LIST`, if there is an active socket descriptor stored for the channel. If not, they will be sent over UDP as `CONN_CLIENTS_LIST` in the normal way.

11. In order to keep the long-term TCP connection alive via firewalls, NAT routers, etc., the client will start a periodic timer (e.g. 15 sec) to send a `CLM_EMPTY_MESSAGE` over the TCP connection. This causes no action at the server, but keeps the TCP connection alive.

12. The server will start an idle timeout, resetting it each time a message is received. If the idle timer times out, the server will close the TCP connection.

13. Audio packets will continue to always use the UDP socket.

14. To disconnect, a client will send `CLM_DISCONNECT` over UDP exactly as at present, and will also close the TCP connection.

If the server receives a disconnection of the TCP socket, it will revert to UDP for connected client updates. It could send another `CLM_TCP_SUPPORTED` to invite the client to re-establish a TCP connection.

## OTHER CONSIDERATIONS

The server should only be configured to offer TCP by specifying `--enabletcp` if the server operator has also configured any firewall to allow the inbound TCP connections.

If a server were to offer TCP to the client, but the server's firewall didn't allow the incoming TCP connection, the client request for TCP would wait until its request times out.

This has to be the responsibility of the server/directory operator, and is why TCP operation must be controlled by a command-line option, rather than always enabled. The operator should only enable TCP in the Jamulus server if they know their environment has been configured to support it.

Most operators of small servers of directories will not need to be concerned with TCP at all. _The only server operators who will need to enable TCP support are those running large directories (e.g. Volker, Peter) or those running a large server designed to support many simultaneous client connections._
