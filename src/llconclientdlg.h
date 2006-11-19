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

#include <qlabel.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qslider.h>
#include <qradiobutton.h>
#include <qmenubar.h>
#include <qlayout.h>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#include "clientsettingsdlg.h"
#ifdef _WIN32
# include "../windows/moc/llconclientdlgbase.h"
#else
# include "moc/llconclientdlgbase.h"
#endif


/* Definitions ****************************************************************/
/* text strings for connection button for connect and disconnect */
#define CON_BUT_CONNECTTEXT			"C&onnect"
#define CON_BUT_DISCONNECTTEXT		"D&isconnect"

/* steps for input level meter */
#define NUM_STEPS_INP_LEV_METER		100

/* update time for GUI controls */
#define LEVELMETER_UPDATE_TIME		100 /* ms */
#define STATUSBAR_UPDATE_TIME		1000 /* ms */

/* range for signal level meter */
#define LOW_BOUND_SIG_METER			( -50.0 ) /* dB */
#define UPPER_BOUND_SIG_METER		( 0.0 ) /* dB */


/* Classes ********************************************************************/
class CLlconClientDlg : public CLlconClientDlgBase
{
	Q_OBJECT

public:
	CLlconClientDlg ( CClient* pNCliP, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, WFlags f = 0 );

	virtual ~CLlconClientDlg ();

protected:
	class CChannelFader
	{
	public:
		CChannelFader ( QWidget* pNW, QHBoxLayout* pNPtLy );

	protected:
		QGridLayout MainGrid;
		QSlider		Fader;
		QLabel		Label;

		QHBoxLayout* pParentLayout;
	};

	CClient*				pClient;
	bool					bConnected;
	QTimer					TimerSigMet;
	QTimer					TimerStatus;

	virtual void			customEvent ( QCustomEvent* Event );
	virtual void			closeEvent ( QCloseEvent * Event );
	void					UpdateDisplay();

	QPopupMenu*				pSettingsMenu;
	QMenuBar*				pMenu;

	CClientSettingsDlg		ClientSettingsDlg;

	CVector<CChannelFader>	vecpChanFader;

public slots:
	void OnConnectDisconBut();
	void OnTimerSigMet();
	void OnTimerStatus() { UpdateDisplay(); }
	void OnOpenGeneralSettings();
	void OnSliderAudInFader ( int value ) { pClient->SetAudioInFader(value); }
	void OnSliderAudReverb ( int value )
		{ pClient->SetReverbLevel ( AUD_REVERB_MAX - value ); }
	void OnRevSelL() { pClient->SetReverbOnLeftChan(true); }
	void OnRevSelR() { pClient->SetReverbOnLeftChan(false); }
};
