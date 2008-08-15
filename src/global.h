/******************************************************************************\
 * Copyright (c) 2004-2008
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
#define VERSION                         "2.1.3cvs"
#define APP_NAME                        "llcon"

// file name for logging file
#define LOG_FILE_NAME                   "llconsrvlog.txt"

// defined port number for client and server
#define LLCON_PORT_NUMBER               22122

// system sample rate
#define SYSTEM_SAMPLE_RATE              24000

// sound card sample rate. Should be always 48 kHz to avoid sound card driver
// internal sample rate conversion which might be buggy
#define SND_CRD_SAMPLE_RATE             48000

// minimum block duration - all other buffer durations must be a multiple
// of this duration
#define MIN_BLOCK_DURATION_MS           2 // ms

#define MIN_BLOCK_SIZE_SAMPLES          ( MIN_BLOCK_DURATION_MS * SYSTEM_SAMPLE_RATE / 1000 )
#define MIN_SND_CRD_BLOCK_SIZE_SAMPLES  ( MIN_BLOCK_DURATION_MS * SND_CRD_SAMPLE_RATE / 1000 )

// maximum value of factor for network block size
#define MAX_NET_BLOCK_SIZE_FACTOR       3

// default network block size factor
#define DEF_NET_BLOCK_SIZE_FACTOR       3

// maximum network buffer size (which can be chosen by slider)
#define MAX_NET_BUF_SIZE_NUM_BL         10 // number of blocks

// default network buffer size
#define DEF_NET_BUF_SIZE_NUM_BL         5 // number of blocks

// number of ticks of audio in/out buffer sliders
# define AUD_SLIDER_LENGTH              8

// maximum number of internet connections (channels)
// if you want to change this paramter, there has to be done code modifications
// on other places, too! The code tag "MAX_NUM_CHANNELS_TAG" shows these places
// (just search for the tag in the entire code)
#define MAX_NUM_CHANNELS                6 // max number channels for server

// sample rate offset estimation algorithm
// time interval for sample rate offset estimation
#define TIME_INT_SAM_OFFS_EST           60 // s

// length of the moving average buffer for response time measurement
#define TIME_MOV_AV_RESPONSE            30 // seconds
#define LEN_MOV_AV_RESPONSE             ( TIME_MOV_AV_RESPONSE * 1000 / MIN_BLOCK_DURATION_MS )

// GUI definition: width/heigth size of LED pixmaps
#define LED_WIDTH_HEIGHT_SIZE_PIXEL     13


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
#define MS_PROTOCOL                     6
#define MS_ERROR_IN_THREAD              7

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
