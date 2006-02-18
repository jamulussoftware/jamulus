/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
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

#if !defined(GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_)
#define GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_

#include <stdio.h>
#include <math.h>
#include <string>
#include <qstring.h>
#include <qevent.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* Definitions ****************************************************************/
/* define this macro to get debug output */
#define _DEBUG_
#undef _DEBUG_

/* version and application name */
#ifndef VERSION
# define VERSION						"0.9.3cvs"
#endif
#define APP_NAME						"llcon"


/* defined port number for client and server */
#define LLCON_PORT_NUMBER				22122

/* sample rate */
#define SAMPLE_RATE						24000

/* sound card sample rate. Should be always 48 kHz to avoid sound card driver
   internal sample rate conversion which might be buggy */
#define SND_CRD_SAMPLE_RATE   			48000

/* minimum block duration - all other buffer durations must be a multiple
   of this duration */
#define MIN_BLOCK_DURATION_MS			2 /* ms */

#define MIN_BLOCK_SIZE_SAMPLES			( MIN_BLOCK_DURATION_MS * SAMPLE_RATE / 1000 )
#define MIN_SND_CRD_BLOCK_SIZE_SAMPLES	( MIN_BLOCK_DURATION_MS * SND_CRD_SAMPLE_RATE / 1000 )

/* first tests showed that with 24000 kHz a block time shorter than 5 ms leads to
   much higher DSL network latencies. A length of 6 ms seems to be optimal */
#define BLOCK_DURATION_MS				6 /* ms */

#define BLOCK_SIZE_SAMPLES				( BLOCK_DURATION_MS * SAMPLE_RATE / 1000 )
#define SND_CRD_BLOCK_SIZE_SAMPLES		( BLOCK_DURATION_MS * SND_CRD_SAMPLE_RATE / 1000 )

/* maximum network buffer size (which can be chosen by slider) */
#define MAX_NET_BUF_SIZE_NUM_BL			12 /* number of blocks */

/* default network buffer size */
#define DEF_NET_BUF_SIZE_NUM_BL			5 /* number of blocks */

// number of ticks of audio in/out buffer sliders
#ifdef _WIN32
# define AUD_SLIDER_LENGTH				30
#else
# define AUD_SLIDER_LENGTH				6
#endif

/* sample rate offset estimation algorithm */
/* time interval for sample rate offset estimation */
#define TIME_INT_SAM_OFFS_EST			60 /* s */

/* time interval of taps for sample rate offset estimation (time stamps) */
#define INTVL_TAPS_SAM_OFF_SET			1 /* s */

#define NUM_BL_TIME_STAMPS				( ( INTVL_TAPS_SAM_OFF_SET * 1000 ) / BLOCK_DURATION_MS )
#define VEC_LEN_SAM_OFFS_EST			( TIME_INT_SAM_OFFS_EST / INTVL_TAPS_SAM_OFF_SET )

/* length of the moving average buffer for response time measurement */
#define TIME_MOV_AV_RESPONSE			30 /* seconds */
#define LEN_MOV_AV_RESPONSE				(TIME_MOV_AV_RESPONSE * 1000 / BLOCK_DURATION_MS)


#define _MAXSHORT						32767
#define _MAXBYTE						255 /* binary: 11111111 */
#define _MINSHORT						(-32768)

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
typedef unsigned int					_MESSAGE_IDENT;
#define MS_RESET_ALL					0	/* MS: Message */
#define MS_SOUND_IN						1
#define MS_SOUND_OUT					2
#define MS_JIT_BUF_PUT					3
#define MS_JIT_BUF_GET					4
#define MS_PACKET_RECEIVED				5

#define MUL_COL_LED_RED					0
#define MUL_COL_LED_YELLOW				1
#define MUL_COL_LED_GREEN				2


/* Classes ********************************************************************/
class CGenErr
{
public:
	CGenErr(QString strNE) : strError(strNE) {}
	QString strError;
};

class CLlconEvent : public QCustomEvent
{
public:
	CLlconEvent(int iNewMeTy, int iNewSt, int iNewChN = 0) : 
		QCustomEvent(QEvent::User + 11), iMessType(iNewMeTy), iStatus(iNewSt),
		iChanNum(iNewChN) {}

	int iMessType;
	int iStatus;
	int iChanNum;
};


/* Prototypes for global functions ********************************************/
/* Posting a window message */
void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam = 0,
					const int iChanNum = 0);


#endif /* !defined(GLOBAL_H__3B123453_4344_BB2B_23E7A0D31912__INCLUDED_) */
