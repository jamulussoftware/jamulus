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
CLlconServerDlg::CLlconServerDlg ( CServer* pNServP, QWidget* parent,
	const char* name, bool modal, WFlags f ) : pServer ( pNServP ),
	CLlconServerDlgBase ( parent, name, modal, f )
{
	/* set text for version and application name */
	TextLabelNameVersion->
		setText(QString(APP_NAME) + tr(" server ") + QString(VERSION));

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
	ListViewClients->addColumn(tr("Jitter buffer size"));
	ListViewClients->setColumnAlignment(3, Qt::AlignRight);
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


	/* timers --------------------------------------------------------------- */
	/* start timer for GUI controls */
	Timer.start(GUI_CONTRL_UPDATE_TIME);
}

void CLlconServerDlg::OnTimer()
{
	CVector<CHostAddress>	vecHostAddresses;
	CVector<int>			veciJitBufSize;
	double					dCurTiStdDev;

	ListViewMutex.lock();

	pServer->GetConCliParam(vecHostAddresses, veciJitBufSize);

	/* fill list with connected clients */
	for (int i = 0; i < MAX_NUM_CHANNELS; i++)
	{
		if (!(vecHostAddresses[i].InetAddr == QHostAddress((Q_UINT32) 0)))
		{
			/* main text (IP, port number) */
			vecpListViewItems[i]->setText(0, QString().sprintf("%s : %d",
				vecHostAddresses[i].InetAddr.toString().latin1(),
				vecHostAddresses[i].iPort) /* IP, port */);

			/* jitter buffer size (polling for updates) */
			vecpListViewItems[i]->setText(3,
				QString().setNum(veciJitBufSize[i]));

#ifndef _WIN32
			vecpListViewItems[i]->setVisible ( true );
#endif
		}
		else
		{
#ifdef _WIN32
			/* remove text for Windows version */
			vecpListViewItems[i]->setText(0,"");
			vecpListViewItems[i]->setText(3,"");
			vecpListViewItems[i]->setText(4,"");
#else
			vecpListViewItems[i]->setVisible ( false );
#endif
		}
	}

	ListViewMutex.unlock();

	/* response time (if available) */
	if ( pServer->GetTimingStdDev ( dCurTiStdDev ) )
	{
		TextLabelResponseTime->setText(QString().
			setNum(dCurTiStdDev, 'f', 2) + " ms");
	}
	else
	{
		TextLabelResponseTime->setText("---");
	}
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
