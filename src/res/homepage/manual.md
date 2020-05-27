Jamulus Help (Software Manual)
==============================

Main Window
-----------

![Main window](main.jpg)

### Status LEDs

![LEDs](led.png)

The **Status** LED shows the current audio delay status. 

* **Green** - The delay is perfect for a jam session

* **Yellow** - A session is still possible but it may be harder to play

* **Red** - The delay is too large for jamming

The **Buffers** LED indicator shows the current audio/streaming status. If the light is **red**, the audio stream is interrupted. This is caused by one of the following problems:

- The network jitter buffer is not large enough for the current network/audio interface jitter.
- The sound card buffer delay (buffer size) is set to too small a value.
- The upload or download stream rate is too high for the current available internet bandwidth.
- The CPU of the client or server is at 100%.

### Input level

![Input level](inputlevel.jpg)

This shows the input level of the two stereo channels of the current selected audio input.
Make sure not to clip the input signal to avoid distortions of the audio signal.

### Chat button opens the Chat dialog

![Chat dialog](chat.jpg)

The chat text entered in that dialog is transmitted to
all connected clients. If a new chat message arrives and the Chat dialog is not already open, it will
opened automatically for all clients.

### My Profile button opens the Musician Profile dialog

![My profile dialog](profile.jpg)

Use this to set your Alias/Name
which is displayed below your fader in the server audio mixer board. If an instrument and/or country is set,
icons for these selections will also be shown below your fader. The skill setting changes the background colour of
the fader tag and the city entry shows up in the tool tip of the fader tag (see screenshot below).

![Fader tag tool tip](fadertagtooltip.jpg)

### Connect/disconnect button

Opens a dialog where you can select a server to connect to. If you are connected,
pressing this button will end the session.

![Connect dialog](connect.jpg)

Shows a list of available servers which are registered at the Central Server. Select a server
from this list and press the Connect button to connect to this server. Alternatively, double click a server from
the list to connect to it. Permanent servers (those available for longer than 24 hours) are shown in bold font.

If you know the IP address or URL of a server, you can connect to it using the Server Name/Address
field. An optional port number can be added after the IP address or URL using a colon as a separator, e.g,
jamulus.dyndns.org:22124. The field will also show a list of the most recently used server addresses.

### Reverb effect

![Reverberation](reverberation.jpg)

Reverb can be applied to one local mono audio channel or to both channels in stereo mode.
The mono channel selection and the reverberation level can be modified. For example, if a microphone signal is fed
in to the right audio channel of the sound card and a reverb effect needs to be applied, set the channel selector
to the right and move the fader upwards until the desired reverb level is reached.

Reverb requires significant CPU so it should only be used on fast PCs. If the reverb
level fader is set to minimum (wthe default setting), the effect is switched off and does
not cause any additional CPU usage.

### Local audio pan / balance control

![Local audio pan / balance control](audiofader.jpg)

Controls the relative levels of the left and right local audio channels. For a mono signal
it acts as a pan between the two channels. For example, if a microphone is connected to the right input channel and
an instrument is connected to the left input channel which is much louder than the microphone, move the audio fader
in a direction where the label above the fader shows L -x, where x is the current attenuation indicator.

### Server audio mixer

![Audio faders](faders.jpg)

In the audio mixer frame, a fader is shown for each connected client at the server (including yourself).
The faders allow you to adjust the level of what you hear without affecting what others hear.
The VU meter shows the input level at the server - that is, what you are sending.

Using the Mute button prevents the indicated channel being heard in your local mix. Note that you will continue to see the level indicators moving if sound from the muted musician is reaching the server.

The Solo button allows you to hear one or more musicians on their own. Those not soloed will be muted.


Settings Window
---------------

![Settings](settings.jpg)

### Sound card device

![Sound card device Windows](soundcarddevicewindows.jpg)
![Sound card device Mac](soundcarddevicemac.png)

The ASIO driver (sound card) can be selected using Jamulus under the Windows operating system. If the selected ASIO
driver is not valid an error message is shown and the previous valid driver is selected. Under the Mac operating
system the input and output hardware can be selected.

### Input/output channel mapping

![Channel mapping](channelmapping.png)

If the selected sound card device offers more than one input or output channel, the _Input Channel Mapping
and Output Channel Mapping_ settings are visible. For each Jamulus input/output channel (left and right channel)
a different actual sound card channel can be selected.

### Enable Small Network Buffers

Allows support for very small network audio packets. These are only used if the sound card buffer delay is smaller than 128 samples. The smaller the network buffers, the
lower the audio latency. But at the same time the network load increases and the probability of audio dropouts
also increases.

### Buffer delay

![Buffer delay](bufferdelay.png)

The buffer delay setting is a fundamental setting of the Jamulus software. This setting has an influence on many
connection properties. Three buffer sizes are supported:

- 64 samples: The preferred setting. Provides the lowest latency but does not work with all sound cards.
- 128 samples: Should work for most available sound cards.
- 256 samples: Should only be used on very slow computers, or with a slow internet connection.

Some sound card drivers do not allow the buffer delay to be changed from within the Jamulus software.
In this case the buffer delay setting is disabled and has to be changed using the sound card driver. On Windows, press the ASIO Setup button to open the driver settings panel.

![Buffer delay Windows](bufferdelaywindows.jpg)

On Linux, use the Jack configuration tool to change the buffer size.

The actual buffer delay has influence on the connection status, the current upload rate and the overall delay.
The lower the buffer size, the higher the probability of a red light in the status indicator (drop outs) and the
higher the upload rate and the lower the overall delay.

![Buffer delay dependencies](bufferdelaydependency.jpg)

The buffer setting is therefore a trade-off between audio quality and overall delay.

### Jitter buffer with buffer status indicator

![Jitter buffer](jitterbuffer.png)

The jitter buffer compensates for network and sound card timing jitters. The size of this jitter buffer has
therefore influence on the quality of the audio stream (how many dropouts occur) and the overall delay
(the longer the buffer, the higher the delay).

The jitter buffer size can be manually chosen for the local client and the remote server. For the local jitter
buffer, dropouts in the audio stream are indicated by the light below the jitter buffer size faders.
If the light turns to red, a buffer overrun/underrun took place and the audio stream is interrupted.

The jitter buffer setting is therefore a trade-off between audio quality and overall delay.

An auto setting of the jitter buffer size setting is available. If the check Auto is enabled, the jitter buffers
of the local client and the remote server are set automatically based on measurements of the network and sound card
timing jitter. If the Auto check is enabled, the jitter buffer size faders are disabled (they cannot be moved with the mouse).

### Audio channels

![Audio channels](audiochannels.png)

Select the number of audio channels to be used for communication between client and server. There are three modes
available. The mono and stereo modes use one and two audio channels respectively. In mono-in/stereo-out mode
the audio signal which is sent to the server is mono but the return signal is stereo. This is useful if the sound card has the instrument on one input channel and the microphone on the other channel. In that case
the two input signals can be mixed to one mono channel but the server mix can be heard in stereo.

Enabling the stereo streaming mode will increase the stream data rate. Make sure that the current upload rate does
not exceed the available bandwidth of your internet connection.

In stereo streaming mode, no audio channel selection for the reverberation effect will be available on
the main window since the effect is applied on both channels in this case.

### Audio quality

![Audio quality](audioquality.png)

Select the desired audio quality. A low, normal or high audio quality can be selected. The higher the audio quality,
the higher the audio stream data rate. Make sure that the current upload rate does not exceed the available bandwidth
of your internet connection.

### New client level

![New client level](newclientlevel.png)

The new client level setting defines the fader level of a new connected client in percent. I.e. if a new client connects
to the current server, it will get the specified initial fader level if no other fader level of a previous connection of
that client was already stored.

### Fancy skin

![Fancy skin](fancyskin.png)

If enabled, a fancy skin will be applied to the main window.

### Display channel levels

![Display channel levels](displaychannellevels.png)

If enabled, the channel input level for each connected client will be displayed in the mixer.

### Central server address

![Central server address](centralserveraddress.png)

The central server address is the IP address or URL of the central server at which the server list of the connection
dialog is managed. With the central server address type either the local region can be selected of the default
central servers or a manual address can be specified.

### Current connection status parameter

![Indicators](indicators.png)

The ping time is the time required for the audio stream to travel from the client to the server and back again.
This delay is introduced by the network. This delay should be as low as 20-30 ms. If this delay is higher (e.g., 50-60 ms),
your distance to the server is too large or your internet connection is not sufficient.

The overall delay is calculated from the current ping time and the delay which is introduced by the current buffer settings.

The upstream rate depends on the current audio packet size and the audio compression setting. Make sure that the upstream
rate is not higher than the available rate (check the upstream capabilities of your internet connection by, e.g., using
[speedtest.net](http://speedtest.net)). 

