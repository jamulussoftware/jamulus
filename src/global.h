/******************************************************************************\
 * \Copyright (c) 2004-2020
 * \author    Volker Fischer
 *

\mainpage Jamulus source code documentation

\section intro_sec Introduction

The Jamulus software enables musicians to perform real-time jam sessions over the
internet. There is one server running the Jamulus server software which collects
the audio data from each Jamulus client, mixes the audio data and sends the mix
back to each client.


Prefix definitions for the GUI:

label:        lbl
combo box:    cbx
line edit:    edt
list view:    lvw
check box:    chb
radio button: rbt
button:       but
text view:    txv
slider:       sld
LED:          led
group box:    grb
pixmap label: pxl
LED bar:      lbr

 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#pragma once

#if _WIN32
# define _CRT_SECURE_NO_WARNINGS
#endif

#include <QString>
#include <QEvent>
#include <QDebug>
#include <stdio.h>
#include <math.h>
#include <string>
#ifndef _WIN32
# include <unistd.h> // solves a compile problem on recent Ubuntu
#endif
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* Definitions ****************************************************************/
// define this macro to get debug output
//#define _DEBUG_
#undef _DEBUG_

// define this macro if the version and operating system debugging shall
// be enabled in the client (the ping time column in the connect dialog then
// shows the requested information instead of the ping time)
#undef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING

// version and application name (use version from qt prject file)
#undef VERSION
#define VERSION                          APP_VERSION
#define APP_NAME                         "Jamulus"

// Windows registry key name of auto run entry for the server
#define AUTORUN_SERVER_REG_NAME          "Jamulus server"

// default names of the ini-file for client and server
#define DEFAULT_INI_FILE_NAME            "Jamulus.ini"
#define DEFAULT_INI_FILE_NAME_SERVER     "Jamulusserver.ini"

// file name for logging file
#define DEFAULT_LOG_FILE_NAME            "Jamulussrvlog.txt"

// default oldest item to draw in history graph (days ago)
#define DEFAULT_DAYS_HISTORY             60

// System block size, this is the block size on which the audio coder works.
// All other block sizes must be a multiple of this size.
// Note that the UpdateAutoSetting() function assumes a value of 128.
#define SYSTEM_FRAME_SIZE_SAMPLES        64
#define DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ( 2 * SYSTEM_FRAME_SIZE_SAMPLES )

// default server address
#define DEFAULT_SERVER_ADDRESS           "jamulus.fischvolk.de"
#define DEFAULT_SERVER_NAME              "Central Server"

// getting started and software manual URL
#define CLIENT_GETTING_STARTED_URL       "https://github.com/corrados/jamulus/wiki/Software-Manual"
#define SERVER_GETTING_STARTED_URL       "https://github.com/corrados/jamulus/wiki/Running-a-Server"
#define SOFTWARE_MANUAL_URL              "https://github.com/corrados/jamulus/blob/master/src/res/homepage/manual.md"

// determining server internal address uses well-known host and port
// (Google DNS, or something else reliable)
#define WELL_KNOWN_HOST                  "8.8.8.8" // Google
#define WELL_KNOWN_PORT                  53        // DNS
#define IP_LOOKUP_TIMEOUT                500       // ms

// defined port numbers for client and server
#define LLCON_DEFAULT_PORT_NUMBER        22124
#define LLCON_PORT_NUMBER_NORTHAMERICA   22224

// system sample rate (the sound card and audio coder works on this sample rate)
#define SYSTEM_SAMPLE_RATE_HZ            48000 // Hz

// define the allowed audio frame size factors (since the
// "SYSTEM_FRAME_SIZE_SAMPLES" is quite small, it may be that on some
// computers a larger value is required)
#define FRAME_SIZE_FACTOR_PREFERRED      1 // 64 samples accumulated frame size
#define FRAME_SIZE_FACTOR_DEFAULT        2 // 128 samples accumulated frame size
#define FRAME_SIZE_FACTOR_SAFE           4 // 256 samples accumulated frame size

// define the minimum allowed number of coded bytes for CELT (the encoder
// gets in trouble if the value is too low)
#define CELT_MINIMUM_NUM_BYTES           10

// Maximum block size for network input buffer. It is defined by the longest
// protocol message which is PROTMESSID_CLM_SERVER_LIST: Worst case:
// (2+2+1+2+2)+200*(4+2+2+1+1+2+20+2+32+2+20)=17609
// We add some headroom to that value.
#define MAX_SIZE_BYTES_NETW_BUF          20000

// minimum/maximum network buffer size (which can be chosen by slider)
#define MIN_NET_BUF_SIZE_NUM_BL          1  // number of blocks
#define MAX_NET_BUF_SIZE_NUM_BL          20 // number of blocks
#define AUTO_NET_BUF_SIZE_FOR_PROTOCOL   ( MAX_NET_BUF_SIZE_NUM_BL + 1 ) // auto set parameter (only used for protocol)

// default network buffer size
#define DEF_NET_BUF_SIZE_NUM_BL          10 // number of blocks

// audio mixer fader maximum value
#define AUD_MIX_FADER_MAX                100

// maximum number of recognized sound cards installed in the system,
// definition for "no device"
#define MAX_NUMBER_SOUND_CARDS           129 // e.g. 16 inputs, 8 outputs + default entry (MacOS)
#define INVALID_SNC_CARD_DEVICE          -1

// define the maximum number of audio channel for input/output we can store
// channel infos for (and therefore this is the maximum number of entries in
// the channel selection combo box regardless of the actual available number
// of channels by the audio device)
#define MAX_NUM_IN_OUT_CHANNELS          64

// maximum number of elemts in the server address combo box
#define MAX_NUM_SERVER_ADDR_ITEMS        6

// maximum number of fader settings to be stored (together with the fader tags)
#define MAX_NUM_STORED_FADER_SETTINGS    200

// range for signal level meter
#define LOW_BOUND_SIG_METER              ( -50.0 ) // dB
#define UPPER_BOUND_SIG_METER            ( 0.0 )   // dB

// Maximum number of connected clients at the server.
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// If you want to change this paramter you have to modify the code on some places, too! The code tag
// "MAX_NUM_CHANNELS_TAG" shows these places (just search for the tag in the entire code)
#endif
#define MAX_NUM_CHANNELS                 50 // max number channels for server

// Maximum number of HealthCheck connections allowed
#define MAX_NUM_HEALTH_CONNECTIONS       25

// actual number of used channels in the server
// this parameter can safely be changed from 1 to MAX_NUM_CHANNELS
// without any other changes in the code
#define DEFAULT_USED_NUM_CHANNELS        10 // default used number channels for server

// Maximum number of servers registered in the server list. If you want to
// change this parameter, you most probably have to adjust MAX_SIZE_BYTES_NETW_BUF.
#define MAX_NUM_SERVERS_IN_SERVER_LIST   200

// defines the time interval at which the ping time is updated in the GUI
#define PING_UPDATE_TIME_MS              500 // ms

// defines the time interval at which the ping time is updated for the server
// list
#define PING_UPDATE_TIME_SERVER_LIST_MS  2000 // ms

// defines the interval between Channel Level updates from the server
#define CHANNEL_LEVEL_UPDATE_INTERVAL    200  // number of frames at 64 samples frame size

// time-out until a registered server is deleted from the server list if no
// new registering was made in minutes
#define SERVLIST_TIME_OUT_MINUTES        60 // minutes

// poll time for server list (to check if entries are time-out)
#define SERVLIST_POLL_TIME_MINUTES       1 // minute

// time interval for sending ping messages to servers in the server list
#define SERVLIST_UPDATE_PING_SERVERS_MS  59000 // ms

// time until a slave server registers in the server list
#define SERVLIST_REGIST_INTERV_MINUTES   15 // minutes

// defines the minimum time a server must run to be a permanent server
#define SERVLIST_TIME_PERMSERV_MINUTES   1440 // minutes, 1440 = 60 min * 24 h

// registration response timeout
#define REGISTER_SERVER_TIME_OUT_MS      500 // ms

// defines the maximum number of times to retry server registration
// when no response is received within the timeout (before reverting
// to SERVLIST_REGIST_INTERV_MINUTES)
#define REGISTER_SERVER_RETRY_LIMIT      5 // count


// Maximum length of fader tag and text message strings (Since for chat messages
// some HTML code is added, we also have to define a second length which includes
// this additionl HTML code. Right now the length of the HTML code is approx. 66
// character. Here, we add some headroom to this number)
#define MAX_LEN_FADER_TAG                16
#define MAX_LEN_CHAT_TEXT                1600
#define MAX_LEN_CHAT_TEXT_PLUS_HTML      1800
#define MAX_LEN_SERVER_NAME              20
#define MAX_LEN_IP_ADDRESS               15
#define MAX_LEN_SERVER_CITY              20
#define MAX_LEN_VERSION_TEXT             20

// common tool tip bottom line text
#define TOOLTIP_COM_END_TEXT             tr ( \
    "<br><div align=right><font size=-1><i>" \
    "For more information use the ""What's " \
    "This"" help (? menu, right mouse button or Shift+F1)" \
    "</i></font></div>" )

#define _MAXSHORT                        32767
#define _MAXBYTE                         255 // binary: 11111111
#define _MINSHORT                        ( -32768 )

#if HAVE_STDINT_H
# include <stdint.h>
#elif HAVE_INTTYPES_H
# include <inttypes.h>
#elif defined ( _WIN32 )
typedef __int64            int64_t;
typedef __int32            int32_t;
typedef __int16            int16_t;
typedef unsigned __int64   uint64_t;
typedef unsigned __int32   uint32_t;
typedef unsigned __int16   uint16_t;
typedef unsigned __int8    uint8_t;
#else
typedef long long          int64_t;
typedef int                int32_t;
typedef short              int16_t;
typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
#endif


/* Pseudo enum definitions -------------------------------------------------- */
// definition for custom event
#define MS_PACKET_RECEIVED               0


/* Classes ********************************************************************/
class CGenErr
{
public:
    CGenErr ( QString strNewErrorMsg, QString strNewErrorType = "" ) :
        strErrorMsg ( strNewErrorMsg ), strErrorType ( strNewErrorType ) {}

    QString GetErrorText()
    {
        // return formatted error text
        if ( strErrorType.isEmpty() )
        {
            return strErrorMsg;
        }
        else
        {
            return strErrorType + ": " + strErrorMsg;
        }
    }

protected:
    QString strErrorMsg;
    QString strErrorType;
};

class CCustomEvent : public QEvent
{
public:
    CCustomEvent ( int iNewMeTy, int iNewSt, int iNewChN = 0 ) :
        QEvent ( QEvent::Type ( QEvent::User + 11 ) ), iMessType ( iNewMeTy ), iStatus ( iNewSt ),
        iChanNum ( iNewChN ) {}

    int iMessType;
    int iStatus;
    int iChanNum;
};


/* Prototypes for global functions ********************************************/
// command line parsing, TODO do not declare functions globally but in a class
QString UsageArguments ( char** argv );

bool    GetFlagArgument ( char**  argv,
                          int&    i,
                          QString strShortOpt,
                          QString strLongOpt );

bool    GetStringArgument ( QTextStream& tsConsole,
                            int          argc,
                            char**       argv,
                            int&         i,
                            QString      strShortOpt,
                            QString      strLongOpt,
                            QString&     strArg );

bool    GetNumericArgument ( QTextStream& tsConsole,
                             int          argc,
                             char**       argv,
                             int&         i,
                             QString      strShortOpt,
                             QString      strLongOpt,
                             double       rRangeStart,
                             double       rRangeStop,
                             double&      rValue);
