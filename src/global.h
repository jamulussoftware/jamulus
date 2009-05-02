/******************************************************************************\
 * Copyright (c) 2004-2009
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
#define _DEBUG_
#undef _DEBUG_

// version and application name (always use this version)
#undef VERSION
#define VERSION                         "2.2.2cvs"
#define APP_NAME                        "llcon"

// file name for logging file
#define DEFAULT_LOG_FILE_NAME           "llconsrvlog.txt"

// default server address
#define DEFAULT_SERVER_ADDRESS          "llcon.dyndns.org"

// defined port number for client and server
#define LLCON_DEFAULT_PORT_NUMBER       22122

// system sample rate
#define SYSTEM_SAMPLE_RATE              24000

// sound card sample rate. Should be always 48 kHz to avoid sound card driver
// internal sample rate conversion which might be buggy
#define SND_CRD_SAMPLE_RATE             48000

// minimum server block duration - all other buffer durations must be a multiple
// of this duration
#define MIN_SERVER_BLOCK_DURATION_MS    2 // ms

#define MIN_SERVER_BLOCK_SIZE_SAMPLES   ( MIN_SERVER_BLOCK_DURATION_MS * SYSTEM_SAMPLE_RATE / 1000 )

// define the maximum mono audio buffer size at a sample rate
// of 48 kHz, this is important for defining the maximum number
// of bytes to be expected from the network interface (we assume
// here that "MAX_NET_BLOCK_SIZE_FACTOR * MIN_SERVER_BLOCK_SIZE_SAMPLES"
// is smaller than this value here)
#define MAX_MONO_AUD_BUFF_SIZE_AT_48KHZ 4096

// maximum value of factor for network block size
#define MAX_NET_BLOCK_SIZE_FACTOR       3

// default network block size factor (only used for server)
#define DEF_NET_BLOCK_SIZE_FACTOR       2

// minimum/maximum network buffer size (which can be chosen by slider)
#define MIN_NET_BUF_SIZE_NUM_BL         1 // number of blocks
#define MAX_NET_BUF_SIZE_NUM_BL         20 // number of blocks

// default network buffer size
#define DEF_NET_BUF_SIZE_NUM_BL         10 // number of blocks

// default maximum upload rate at server (typical DSL upload for good DSL)
#define DEF_MAX_UPLOAD_RATE_KBPS        800 // kbps

// maximum number of recognized sound cards installed in the system,
// definition for "no device"
#define MAX_NUMBER_SOUND_CARDS          10
#define INVALID_SNC_CARD_DEVICE         -1

// defines for LED input level meter
#define NUM_STEPS_INP_LEV_METER         12
#define YELLOW_BOUND_INP_LEV_METER      9
#define RED_BOUND_INP_LEV_METER         11


// maximum number of internet connections (channels)
// if you want to change this paramter, there has to be done code modifications
// on other places, too! The code tag "MAX_NUM_CHANNELS_TAG" shows these places
// (just search for the tag in the entire code)
#define MAX_NUM_CHANNELS                10 // max number channels for server

// actual number of used channels in the server
// this parameter can safely be changed from 1 to MAX_NUM_CHANNELS
// without any other changes in the code
#define USED_NUM_CHANNELS               6 // used number channels for server


// length of the moving average buffer for response time measurement
#define TIME_MOV_AV_RESPONSE            30 // seconds


#define _MAXSHORT                       32767
#define _MAXBYTE                        255 // binary: 11111111
#define _MINSHORT                       ( -32768 )

#if HAVE_STDINT_H
# include <stdint.h>
#elif HAVE_INTTYPES_H
# include <inttypes.h>
#elif defined ( _WIN32 )
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8  uint8_t;
#else
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
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
    QString strErrorType;
    QString strErrorMsg;
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
std::string UsageArguments     ( char **argv );
bool        GetFlagArgument    ( int, char **argv, int &i, std::string strShortOpt, std::string strLongOpt );
bool        GetStringArgument  ( int argc, char **argv, int &i, std::string strShortOpt, std::string strLongOpt, std::string & strArg );
bool        GetNumericArgument ( int argc, char **argv, int &i, std::string strShortOpt, std::string strLongOpt, double rRangeStart, double rRangeStop, double & rValue);

// posting a window message
void PostWinMessage ( const _MESSAGE_IDENT MessID, const int iMessageParam = 0,
                      const int iChanNum = 0 );

#endif /* !defined ( GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_ ) */
