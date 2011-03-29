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

#include "connectdlg.h"


/* Implementation *************************************************************/
CConnectDlg::CConnectDlg ( QWidget* parent, Qt::WindowFlags f ) :
    QDialog ( parent, f )
{
    setupUi ( this );


    // add help text to controls -----------------------------------------------

// TODO


    // set up list view for connected clients
    ListViewServers->setColumnWidth ( 0, 170 );
    ListViewServers->setColumnWidth ( 1, 130 );
    ListViewServers->setColumnWidth ( 2, 55 );
    ListViewServers->setColumnWidth ( 3, 80 );
    ListViewServers->clear();


//TextLabelPingTime->setText ( "" );


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // only activate ping timer if window is actually shown
    TimerPing.start ( PING_UPDATE_TIME );

//    UpdateDisplay();
}
	
void CConnectDlg::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timer for ping
    TimerPing.stop();
}

void CConnectDlg::OnTimerPing()
{
    // send ping message to server
//    pClient->SendPingMess();
}

void CConnectDlg::OnPingTimeResult ( int iPingTime )
{

// TODO
//    TextLabelPingTime->setText ( sErrorText );
}