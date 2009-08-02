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

#include "llconserverdlg.h"


/* Implementation *************************************************************/
CLlconServerDlg::CLlconServerDlg ( CServer* pNServP, QWidget* parent )
    : pServer ( pNServP ), QDialog ( parent )
{
    setupUi ( this );

    // set text for version and application name
    TextLabelNameVersion->setText ( QString ( APP_NAME ) +
        tr ( " server " ) + QString ( VERSION ) );

    // set up list view for connected clients
    ListViewClients->setColumnWidth ( 0, 170 );
    ListViewClients->setColumnWidth ( 1, 130 );
    ListViewClients->setColumnWidth ( 2, 40 );
    ListViewClients->setColumnWidth ( 3, 40 );
    ListViewClients->clear();

    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( USED_NUM_CHANNELS );
    for ( int i = USED_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new CServerListViewItem ( ListViewClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // Init timing jitter text label
    TextLabelResponseTime->setText ( "" );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );
    pMenu->addMenu ( new CLlconHelpMenu ( this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &Timer, SIGNAL ( timeout() ), this, SLOT ( OnTimer() ) );


    // Timers ------------------------------------------------------------------
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );
}

void CLlconServerDlg::OnTimer()
{
    CVector<CHostAddress>   vecHostAddresses;
    CVector<QString>        vecsName;
    CVector<int>            veciJitBufNumFrames;
    CVector<int>            veciNetwFrameSizeFact;
    double                  dCurTiStdDev;

    ListViewMutex.lock();

    pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames,
        veciNetwFrameSizeFact );

    // fill list with connected clients
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( !( vecHostAddresses[i].InetAddr == QHostAddress ( (quint32) 0 ) ) )
        {
            // IP, port number
            vecpListViewItems[i]->setText ( 0, QString("%1 : %2" ).
                arg ( vecHostAddresses[i].InetAddr.toString() ).
                arg ( vecHostAddresses[i].iPort ) );

            // name
            vecpListViewItems[i]->setText ( 1, vecsName[i] );

            // jitter buffer size (polling for updates)
            vecpListViewItems[i]->setText ( 4,
                QString().setNum ( veciJitBufNumFrames[i] ) );

            // out network block size
            vecpListViewItems[i]->setText ( 5,
                QString().setNum ( static_cast<double> (
                veciNetwFrameSizeFact[i] * SYSTEM_BLOCK_DURATION_MS_FLOAT
                ), 'f', 2 ) );

            vecpListViewItems[i]->setHidden ( false );
        }
        else
        {
            vecpListViewItems[i]->setHidden ( true );
        }
    }

    ListViewMutex.unlock();

    // response time (if available)
    if ( pServer->GetTimingStdDev ( dCurTiStdDev ) )
    {
        TextLabelResponseTime->setText ( QString().
            setNum ( dCurTiStdDev, 'f', 2 ) + " ms" );
    }
    else
    {
        TextLabelResponseTime->setText ( "---" );
    }
}

void CLlconServerDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        ListViewMutex.lock();

        const int iMessType = ( (CLlconEvent*) Event )->iMessType;
        const int iStatus   = ( (CLlconEvent*) Event )->iStatus;
        const int iChanNum  = ( (CLlconEvent*) Event )->iChanNum;

        switch(iMessType)
        {
        case MS_JIT_BUF_PUT:
            vecpListViewItems[iChanNum]->SetLight ( 0, iStatus );
            break;

        case MS_JIT_BUF_GET:
            vecpListViewItems[iChanNum]->SetLight ( 1, iStatus );
            break;
        }

        ListViewMutex.unlock();
    }
}
