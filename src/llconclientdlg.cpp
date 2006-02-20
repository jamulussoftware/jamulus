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

#include "llconclientdlg.h"


/* Implementation *************************************************************/
CLlconClientDlg::CLlconClientDlg ( CClient* pNCliP, QWidget* parent,
	const char* name, bool modal, WFlags f) : pClient ( pNCliP ),
	CLlconClientDlgBase ( parent, name, modal, f )
{
	/* add help text to controls */
	QString strInpLevH = tr("<b>Input level meter:</b> Shows the level of the "
		"input audio signal of the sound card. The level is in dB. Overload "
		"should be avoided.");
	QWhatsThis::add(TextLabelInputLevel, strInpLevH);
	QWhatsThis::add(ProgressBarInputLevelL, strInpLevH);
	QWhatsThis::add(ProgressBarInputLevelR, strInpLevH);

	QWhatsThis::add(PushButtonConnect, tr("<b>Connect / Disconnect Button:"
		"</b> Push this button to connect the server. A valid IP address has "
		"to be specified before. If the client is connected, pressing this "
		"button will disconnect the connection."));

	QWhatsThis::add(TextLabelNameVersion, tr("<b>Version:</b> Shows the "
		"current version of the software."));

	QWhatsThis::add(TextLabelStatus, tr("<b>Status Bar:</b> In the status bar "
		"different messages are displayed. E.g., if an error ocurred or the "
		"status of the connection is shown."));

	QString strServAddrH = tr("<b>Server Address:</b> In this edit control, "
		"the IP address of the server can be set. If an invalid address was "
		"chosen, an error message is shown in the status bar.");
	QWhatsThis::add(TextLabelServerAddr, strServAddrH);
	QWhatsThis::add(LineEditServerAddr, strServAddrH);

	/* set text for version and application name */
	TextLabelNameVersion->
		setText(QString(APP_NAME) + tr(" client ") + QString(VERSION) +
		" (" + QString().setNum(BLOCK_DURATION_MS) + " ms)");

	/* init server address line edit */
	LineEditServerAddr->setText ( pClient->strIPAddress.c_str () );

	/* init status label */
	OnTimerStatus ();

	/* init sample rate offset label */
	TextSamRateOffsValue->setText ( "0 Hz" );

	/* init connection button text */
	PushButtonConnect->setText ( CON_BUT_CONNECTTEXT );

	/* Init timing jitter text label */
	TextLabelStdDevTimer->setText ( "" );

	/* init input level meter bars */
	ProgressBarInputLevelL->setTotalSteps ( NUM_STEPS_INP_LEV_METER );
	ProgressBarInputLevelL->setProgress ( 0 );
	ProgressBarInputLevelR->setTotalSteps ( NUM_STEPS_INP_LEV_METER );
	ProgressBarInputLevelR->setProgress ( 0 );


	/* init slider controls --- */
	/* sound buffer in */
	SliderSndBufIn->setRange(2, AUD_SLIDER_LENGTH);
	const int iCurNumInBuf = pClient->GetSndInterface()->GetInNumBuf();
	SliderSndBufIn->setValue(iCurNumInBuf);
	TextSndBufIn->setText("In: " + QString().setNum(iCurNumInBuf));

	/* sound buffer out */
	SliderSndBufOut->setRange(2, AUD_SLIDER_LENGTH);
	const int iCurNumOutBuf = pClient->GetSndInterface()->GetOutNumBuf();
	SliderSndBufOut->setValue(iCurNumOutBuf);
	TextSndBufOut->setText("Out: " + QString().setNum(iCurNumOutBuf));

	/* network buffer */
	SliderNetBuf->setRange(1, MAX_NET_BUF_SIZE_NUM_BL);
	const int iCurNumNetBuf = pClient->GetChannel()->GetSockBufSize();
	SliderNetBuf->setValue(iCurNumNetBuf);
	TextNetBuf->setText("Size: " + QString().setNum(iCurNumNetBuf));

	/* audio in fader */
	SliderAudInFader->setRange(0, AUD_FADER_IN_MAX);
	const int iCurAudInFader = pClient->GetAudioInFader();
	SliderAudInFader->setValue(iCurAudInFader);
	SliderAudInFader->setTickInterval(AUD_FADER_IN_MAX / 9);

	/* audio reverberation */
	SliderAudReverb->setRange(0, AUD_REVERB_MAX);
	const int iCurAudReverb = pClient->GetReverbLevel();
	SliderAudReverb->setValue ( AUD_REVERB_MAX - iCurAudReverb );
	SliderAudReverb->setTickInterval(AUD_REVERB_MAX / 9);


	/* set radio buttons --- */
	/* reverb channel */
	if (pClient->IsReverbOnLeftChan())
		RadioButtonRevSelL->setChecked(true);
	else
		RadioButtonRevSelR->setChecked(true);


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&?"), new CLlconHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CLlconClientDlgBaseLayout->setMenuBar(pMenu);


	/* connections ---------------------------------------------------------- */
	/* push-buttons */
    QObject::connect(PushButtonConnect, SIGNAL(clicked()),
		this, SLOT(OnConnectDisconBut()));

	/* timers */
	QObject::connect(&TimerSigMet, SIGNAL(timeout()),
		this, SLOT(OnTimerSigMet()));
	QObject::connect(&TimerStatus, SIGNAL(timeout()),
		this, SLOT(OnTimerStatus()));

	/* sliders */
	QObject::connect(SliderSndBufIn, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderSndBufInChange(int)));
	QObject::connect(SliderSndBufOut, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderSndBufOutChange(int)));
	QObject::connect(SliderNetBuf, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderNetBuf(int)));

	QObject::connect(SliderAudInFader, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderAudInFader(int)));
	QObject::connect(SliderAudReverb, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderAudReverb(int)));

	/* radio buttons */
	QObject::connect(RadioButtonRevSelL, SIGNAL(clicked()),
		this, SLOT(OnRevSelL()));
	QObject::connect(RadioButtonRevSelR, SIGNAL(clicked()),
		this, SLOT(OnRevSelR()));


	/* timers --------------------------------------------------------------- */
	/* start timer for status bar */
	TimerStatus.start(STATUSBAR_UPDATE_TIME);
}

CLlconClientDlg::~CLlconClientDlg()
{
	/* if connected, terminate connection */
	if (pClient->IsRunning())
	{
		pClient->Stop();
	}
}

void CLlconClientDlg::closeEvent ( QCloseEvent * Event )
{
	// store IP address
	pClient->strIPAddress = LineEditServerAddr->text().latin1();

	// default implementation of this event handler routine
	Event->accept();
}

void CLlconClientDlg::OnConnectDisconBut()
{
	/* start/stop client, set button text */
	if (pClient->IsRunning())
	{
		pClient->Stop();
		PushButtonConnect->setText(CON_BUT_CONNECTTEXT);

		/* stop timer for level meter bars and reset them */
		TimerSigMet.stop();
		ProgressBarInputLevelL->setProgress(0);
		ProgressBarInputLevelR->setProgress(0);

		/* immediately update status bar */
		OnTimerStatus();
	}
	else
	{
		/* set address and check if address is valid */
		if (pClient->SetServerAddr(LineEditServerAddr->text()))
		{
			pClient->start();
			PushButtonConnect->setText(CON_BUT_DISCONNECTTEXT);

			/* start timer for level meter bar */
			TimerSigMet.start(LEVELMETER_UPDATE_TIME);
		}
		else
		{
			/* Restart timer to ensure that the text is visible at
			   least the time for one complete interval */
			TimerStatus.changeInterval(STATUSBAR_UPDATE_TIME);

			/* show the error in the status bar */
			TextLabelStatus->setText(tr("invalid address"));
		}
	}
}

void CLlconClientDlg::OnSliderSndBufInChange(int value)
{
	pClient->GetSndInterface()->SetInNumBuf(value);
	TextSndBufIn->setText("In: " + QString().setNum(value));
}

void CLlconClientDlg::OnSliderSndBufOutChange(int value)
{
	pClient->GetSndInterface()->SetOutNumBuf(value);
	TextSndBufOut->setText("Out: " + QString().setNum(value));
}

void CLlconClientDlg::OnSliderNetBuf(int value)
{
	pClient->GetChannel()->SetSockBufSize ( MIN_BLOCK_SIZE_SAMPLES, value );
	TextNetBuf->setText("Size: " + QString().setNum(value));
}

void CLlconClientDlg::OnTimerSigMet ()
{
	/* get current input levels */
	double dCurSigLevelL = pClient->MicLevelL ();
	double dCurSigLevelR = pClient->MicLevelR ();

	/* linear transformation of the input level range to the progress-bar
	   range */
	dCurSigLevelL -= LOW_BOUND_SIG_METER;
	dCurSigLevelL *= NUM_STEPS_INP_LEV_METER /
		( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

	// lower bound the signal
	if ( dCurSigLevelL < 0 )
	{
		dCurSigLevelL = 0;
	}

	dCurSigLevelR -= LOW_BOUND_SIG_METER;
	dCurSigLevelR *= NUM_STEPS_INP_LEV_METER /
		( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

	// lower bound the signal
	if ( dCurSigLevelR < 0 )
	{
		dCurSigLevelR = 0;
	}

	/* show current level */
	ProgressBarInputLevelL->setProgress ( (int) ceil ( dCurSigLevelL ) );
	ProgressBarInputLevelR->setProgress ( (int) ceil ( dCurSigLevelR ) );
}

void CLlconClientDlg::OnTimerStatus ()
{
	/* show connection status in status bar */
	if ( pClient->IsConnected () && pClient->IsRunning () )
	{
		TextLabelStatus->setText ( tr ( "connected" ) );
	}
	else
	{
		TextLabelStatus->setText ( tr ( "disconnected" ) );
	}

	/* update sample rate offset label */
	QString strSamRaOffs;
	strSamRaOffs.setNum(pClient->GetChannel()->GetResampleOffset(), 'f', 2);
	TextSamRateOffsValue->setText(strSamRaOffs + " Hz");

	/* response time */
	TextLabelStdDevTimer->setText(QString().
		setNum(pClient->GetTimingStdDev(), 'f', 2) + " ms");
}

void CLlconClientDlg::customEvent(QCustomEvent* Event)
{
	if (Event->type() == QEvent::User + 11)
	{
		const int iMessType = ((CLlconEvent*) Event)->iMessType;
		const int iStatus = ((CLlconEvent*) Event)->iStatus;

		switch(iMessType)
		{
		case MS_SOUND_IN:
			CLEDSoundIn->SetLight(iStatus);
			break;

		case MS_SOUND_OUT:
			CLEDSoundOut->SetLight(iStatus);
			break;

		case MS_JIT_BUF_PUT:
			CLEDNetwPut->SetLight(iStatus);
			break;

		case MS_JIT_BUF_GET:
			CLEDNetwGet->SetLight(iStatus);
			break;

		case MS_RESET_ALL:
			CLEDSoundIn->Reset();
			CLEDSoundOut->Reset();
			CLEDNetwPut->Reset();
			CLEDNetwGet->Reset();
			break;
		}
	}
}
