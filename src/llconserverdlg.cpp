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

#include "llconserverdlg.h"


/* Implementation *************************************************************/
CLlconServerDlg::CLlconServerDlg(QWidget* parent, const char* name, bool modal,
	WFlags f) : CLlconServerDlgBase(parent, name, modal, f)
{
	/* set text for version and application name */
	TextLabelNameVersion->
		setText(QString(APP_NAME) + tr(" server ") + QString(VERSION) +
		" (" + QString().setNum(BLOCK_DURATION_MS) + " ms)");

	/* Create bitmaps */
	/* Define size of the bitmaps */
	const int iXSize = 13;
	const int iYSize = 13;
	BitmCubeGreen.resize(iXSize, iYSize);
	BitmCubeGreen.fill(QColor(0, 255, 0));
	BitmCubeRed.resize(iXSize, iYSize);
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeYellow.resize(iXSize, iYSize);
	BitmCubeYellow.fill(QColor(255, 255, 0));

	/* set up list view for connected clients (We assume that one column is
	   already there) */
	ListViewClients->setColumnText(0, tr("Client IP : Port"));
	ListViewClients->setColumnWidth(0, 170);
	ListViewClients->addColumn(tr("Put"));
	ListViewClients->setColumnAlignment(1, Qt::AlignCenter);
	ListViewClients->addColumn(tr("Get"));
	ListViewClients->setColumnAlignment(2, Qt::AlignCenter);
	ListViewClients->addColumn(tr("Sample-rate offset [Hz]"));
	ListViewClients->clear();

	/* insert items in reverse order because in Windows all of them are
	   always visible -> put first item on the top */
	vecpListViewItems.Init(MAX_NUM_CHANNELS);
	for (int i = MAX_NUM_CHANNELS - 1; i >= 0; i--)
	{
		vecpListViewItems[i] = new CServerListViewItem(ListViewClients);
#ifndef _WIN32
		vecpListViewItems[i]->setVisible(false);
#endif
	}

	/* Init slider control */
	SliderNetBuf->setRange(1, MAX_NET_BUF_SIZE_NUM_BL);
	const int iCurNumNetBuf = Server.GetChannelSet()->GetSockBufSize();
	SliderNetBuf->setValue(iCurNumNetBuf);
	TextNetBuf->setText("Size: " + QString().setNum(iCurNumNetBuf));

	/* start the server */
	Server.Start();

	/* Init timing jitter text label */
	TextLabelResponseTime->setText("");


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&?"), new CLlconHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CLlconServerDlgBaseLayout->setMenuBar(pMenu);


	/* connections ---------------------------------------------------------- */
	/* timers */
	QObject::connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	/* sliders */
	QObject::connect(SliderNetBuf, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderNetBuf(int)));


	/* timers --------------------------------------------------------------- */
	/* start timer for GUI controls */
	Timer.start(GUI_CONTRL_UPDATE_TIME);
}

void CLlconServerDlg::OnTimer()
{
	CVector<CHostAddress>	vecHostAddresses;
	CVector<double>			vecdSamOffs;

	ListViewMutex.lock();

	Server.GetConCliParam(vecHostAddresses, vecdSamOffs);

	/* fill list with connected clients */
	for (int i = 0; i < MAX_NUM_CHANNELS; i++)
	{
		if (!(vecHostAddresses[i].InetAddr == QHostAddress((Q_UINT32) 0)))
		{
			/* main text (IP, port number) */
			vecpListViewItems[i]->setText(0, QString().sprintf("%s : %d",
				vecHostAddresses[i].InetAddr.toString().latin1(),
				vecHostAddresses[i].iPort) /* IP, port */);

			/* sample rate offset */
// FIXME disable sample rate estimation result label since estimation does not work
//			vecpListViewItems[i]->setText(3,
//				QString().sprintf("%5.2f", vecdSamOffs[i]));

#ifndef _WIN32
			vecpListViewItems[i]->setVisible(true);
#endif
		}
#ifndef _WIN32
		else
			vecpListViewItems[i]->setVisible(false);
#endif
	}

	ListViewMutex.unlock();

	/* response time */
	TextLabelResponseTime->setText(QString().
		setNum(Server.GetTimingStdDev(), 'f', 2) + " ms");
}

void CLlconServerDlg::OnSliderNetBuf(int value)
{
	Server.GetChannelSet()->SetSockBufSize( BLOCK_SIZE_SAMPLES, value );
	TextNetBuf->setText("Size: " + QString().setNum(value));
}

void CLlconServerDlg::customEvent(QCustomEvent* Event)
{
	if (Event->type() == QEvent::User + 11)
	{
		ListViewMutex.lock();

		const int iMessType = ((CLlconEvent*) Event)->iMessType;
		const int iStatus = ((CLlconEvent*) Event)->iStatus;
		const int iChanNum = ((CLlconEvent*) Event)->iChanNum;

		switch(iMessType)
		{
		case MS_JIT_BUF_PUT:
			vecpListViewItems[iChanNum]->SetLight(0, iStatus);
			break;

		case MS_JIT_BUF_GET:
			vecpListViewItems[iChanNum]->SetLight(1, iStatus);
			break;
		}

		ListViewMutex.unlock();
	}
}
