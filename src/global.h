/******************************************************************************\
 * Copyright (c) 2004-2011
 *
 * Author(s):
 *  Volker Fischer
 *
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

#if !defined ( GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_ )
#define GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_

#include <qstring.h>
#include <qevent.h>
#include <qdebug.h>
#include <stdio.h>
#include <math.h>
#include <string>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* Definitions ****************************************************************/
// define this macro to get debug output
//#define _DEBUG_
#undef _DEBUG_

// version and application name (always use this version)
#undef VERSION
#define VERSION                         "3.1.3cvs"
#define APP_NAME                        "llcon"

// default name of the ini-file
#define DEFAULT_INI_FILE_NAME           "llcon.ini"

// file name for logging file
#define DEFAULT_LOG_FILE_NAME           "llconsrvlog.txt"

// default server address
#define DEFAULT_SERVER_ADDRESS          "llcon.dyndns.org"

// download URL
#define LLCON_DOWNLOAD_URL              "http://sourceforge.net/projects/llcon/files"

// defined port number for client and server
#define LLCON_DEFAULT_PORT_NUMBER       22124

// system sample rate (the sound card and audio coder works on this sample rate)
#define SYSTEM_SAMPLE_RATE_HZ           48000 // Hz

// System block size, this is the block size on which the audio coder works.
// All other block sizes must be a multiple of this size
#define SYSTEM_FRAME_SIZE_SAMPLES       128

#define SYSTEM_BLOCK_DURATION_MS_FLOAT  \
    ( static_cast<double> ( SYSTEM_FRAME_SIZE_SAMPLES ) / \
    SYSTEM_SAMPLE_RATE_HZ * 1000 )

// define the allowed audio frame size factors (since the
// "SYSTEM_FRAME_SIZE_SAMPLES" is quite small, it may be that on some
// computers a larger value is required)
#define FRAME_SIZE_FACTOR_PREFERRED     1 // 128 (for frame size 128)
#define FRAME_SIZE_FACTOR_DEFAULT       2 // 256 (for frame size 128)
#define FRAME_SIZE_FACTOR_SAFE          4 // 512 (for frame size 128)

// low complexity CELT encoder (if defined)
#define USE_LOW_COMPLEXITY_CELT_ENC

// define the maximum mono audio buffer size at a sample rate
// of 48 kHz, this is important for defining the maximum number
// of bytes to be expected from the network interface
#define MAX_MONO_AUD_BUFF_SIZE_AT_48KHZ 4096

// Maximum block size for network input buffer. Consider a maximum sample rate
// of 48 kHz and two audio channels and two bytes per sample.
#define MAX_SIZE_BYTES_NETW_BUF         ( MAX_MONO_AUD_BUFF_SIZE_AT_48KHZ * 4 )

// minimum/maximum network buffer size (which can be chosen by slider)
#define MIN_NET_BUF_SIZE_NUM_BL         1 // number of blocks
#define MAX_NET_BUF_SIZE_NUM_BL         20 // number of blocks

// default network buffer size
#define DEF_NET_BUF_SIZE_NUM_BL         10 // number of blocks

// maximum number of recognized sound cards installed in the system,
// definition for "no device"
#define MAX_NUMBER_SOUND_CARDS          10
#define INVALID_SNC_CARD_DEVICE         -1

// define the maximum number of audio channel for input/output we can store
// channel infos for (and therefore this is the maximum number of entries in
// the channel selection combo box regardless of the actual available number
// of channels by the audio device)
#define MAX_NUM_IN_OUT_CHANNELS         32

// maximum number of elemts in the server address combo box
#define MAX_NUM_SERVER_ADDR_ITEMS       6

// defines for LED input level meter
#define NUM_STEPS_INP_LEV_METER         8
#define RED_BOUND_INP_LEV_METER         7
#define YELLOW_BOUND_INP_LEV_METER      5

// maximum number of internet connections (channels)
// if you want to change this paramter, there has to be done code modifications
// on other places, too! The code tag "MAX_NUM_CHANNELS_TAG" shows these places
// (just search for the tag in the entire code)
#define MAX_NUM_CHANNELS                12 // max number channels for server

// actual number of used channels in the server
// this parameter can safely be changed from 1 to MAX_NUM_CHANNELS
// without any other changes in the code
#define USED_NUM_CHANNELS               6 // used number channels for server


// defines the time interval at which the ping time is updated in the GUI
#define PING_UPDATE_TIME_MS             500 // ms

// time-out until a registered server is deleted from the server list if no
// new registering was made in minutes
#define SERVLIST_TIME_OUT_MINUTES       60 // minutes

// poll time for server list (to check if entries are time-out)
#define SERVLIST_POLL_TIME_MINUTES      1 // minute

// time until a slave server registers in the server list
#define SERVLIST_REGIST_INTERV_MINUTES  30 // minutes


// length of the moving average buffer for response time measurement
#define TIME_MOV_AV_RESPONSE_SECONDS    30 // seconds


// Maximum length of fader tag and text message strings (Since for chat messages
// some HTML code is added, we also have to define a second length which includes
// this additionl HTML code. Right now the length of the HTML code is approx. 66
// character. Here, we add some headroom to this number)
#define MAX_LEN_FADER_TAG               16
#define MAX_LEN_CHAT_TEXT               1600
#define MAX_LEN_CHAT_TEXT_PLUS_HTML     1800

// common tool tip bottom line text
#define TOOLTIP_COM_END_TEXT            tr ( \
    "<br><div align=right><font size=-1><i>" \
    "For more information use the ""What's " \
    "This"" help (? menu, right mouse button or Shift+F1)" \
    "</i></font></div>" )

#define _MAXSHORT                       32767
#define _MAXBYTE                        255 // binary: 11111111
#define _MINSHORT                       ( -32768 )

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


/* Definitions for window message system ------------------------------------ */
typedef unsigned int                    _MESSAGE_IDENT;
#define MS_RESET_ALL                    0   // MS: Message
#define MS_SOUND_IN                     1
#define MS_SOUND_OUT                    2
#define MS_JIT_BUF_PUT                  3
#define MS_JIT_BUF_GET                  4
#define MS_PACKET_RECEIVED              5
#define MS_ERROR_IN_THREAD              6
#define MS_SET_JIT_BUF_SIZE             7

#define MUL_COL_LED_RED                 0
#define MUL_COL_LED_YELLOW              1
#define MUL_COL_LED_GREEN               2


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

class CLlconEvent : public QEvent
{
public:
    CLlconEvent ( int iNewMeTy, int iNewSt, int iNewChN = 0 ) : 
        QEvent ( QEvent::Type ( QEvent::User + 11 ) ), iMessType ( iNewMeTy ), iStatus ( iNewSt ),
        iChanNum ( iNewChN ) {}

    int iMessType;
    int iStatus;
    int iChanNum;
};


/* Prototypes for global functions ********************************************/
// command line parsing, TODO do not declare functions globally but in a class
std::string UsageArguments     ( char** argv );
bool        GetFlagArgument    ( char** argv, int& i, std::string strShortOpt, std::string strLongOpt );
bool        GetStringArgument  ( int argc, char** argv, int& i, std::string strShortOpt, std::string strLongOpt, std::string& strArg );
bool        GetNumericArgument ( int argc, char** argv, int& i, std::string strShortOpt, std::string strLongOpt, double rRangeStart, double rRangeStop, double& rValue);

// posting a window message
void PostWinMessage ( const _MESSAGE_IDENT MessID, const int iMessageParam = 0,
                      const int iChanNum = 0 );

#endif /* !defined ( GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_ ) */
