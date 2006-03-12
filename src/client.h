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

#if !defined(CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_)
#define CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_

#include <qthread.h>
#include <qhostaddress.h>
#include <qstring.h>
#include <qdatetime.h>
#include "global.h"
#include "socket.h"
#include "resample.h"
#include "channel.h"
#include "util.h"
#ifdef _WIN32
# include "../windows/sound.h"
#else
# include "../linux/sound.h"
# include <sched.h>
#endif


/* Definitions ****************************************************************/
/* audio in fader range */
#define AUD_FADER_IN_MAX			100

/* audio reverberation range */
#define AUD_REVERB_MAX				100


/* Classes ********************************************************************/
class CClient : public QObject, public QThread
{
	Q_OBJECT

public:
	CClient();
	virtual ~CClient () {}

	void Init ();
	bool Stop ();
	bool IsRunning () { return bRun; }
	bool SetServerAddr ( QString strNAddr );
	double MicLevelL () { return SignalLevelMeterL.MicLevel (); }
	double MicLevelR () { return SignalLevelMeterR.MicLevel (); }
	bool IsConnected () { return Channel.IsConnected (); }

	/* we want to return the standard deviation. For that we need to calculate
	   the sqaure root */
	double GetTimingStdDev () { return sqrt ( RespTimeMoAvBuf.GetAverage () ); }

	int GetAudioInFader () { return iAudioInFader; }
	void SetAudioInFader ( const int iNV ) { iAudioInFader = iNV; }

	int GetReverbLevel () { return iReverbLevel; }
	void SetReverbLevel ( const int iNL ) { iReverbLevel = iNL; }

	bool IsReverbOnLeftChan () { return bReverbOnLeftChan; }
	void SetReverbOnLeftChan ( const bool bIL )
	{
		bReverbOnLeftChan = bIL;
		AudioReverb.Clear ();
	}


	void SetSockBufSize ( const int iNumBlocks )
	{
		// set the new socket size
		Channel.SetSockBufSize ( iNumBlocks );

		// tell the server that size has changed
		Channel.CreateJitBufMes ( iNumBlocks );
	}
	int GetSockBufSize () { return Channel.GetSockBufSize (); }


	CSound* GetSndInterface () { return &Sound; }
	CChannel* GetChannel () { return &Channel; }


	// settings
	string				strIPAddress;

protected:
	virtual void run ();

	/* only one channel is needed for client application */
	CChannel			Channel;

	CSocket				Socket;
	CSound				Sound;
	CSignalLevelMeter	SignalLevelMeterL;
	CSignalLevelMeter	SignalLevelMeterR;

	bool				bRun;
	CVector<double>		vecdNetwData;

	int					iAudioInFader;
	bool				bReverbOnLeftChan;
	int					iReverbLevel;
	CAudioReverb		AudioReverb;

	int					iSndCrdBlockSizeSam;
	int					iBlockSizeSam;

	CVector<short>		vecsAudioSndCrd;
	CVector<double>		vecdAudioSndCrdL;
	CVector<double>		vecdAudioSndCrdR;

	CVector<double>		vecdAudioL;
	CVector<double>		vecdAudioR;

	CVector<short>		vecsNetwork;

	/* resample objects */
	CAudioResample		ResampleObjDownL; /* left channel */
	CAudioResample		ResampleObjDownR; /* right channel */
	CAudioResample		ResampleObjUpL; /* left channel */
	CAudioResample		ResampleObjUpR; /* right channel */

	/* debugging, evaluating */
	CMovingAv<double>	RespTimeMoAvBuf;
	QTime				TimeLastBlock;

public slots:
	void OnSendProtMessage ( CVector<uint8_t> vecMessage );
	void OnReqJittBufSize() { Channel.CreateJitBufMes ( Channel.GetSockBufSize() ); }
};


#endif /* !defined(CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_) */
