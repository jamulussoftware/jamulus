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

#include "llconserverdlg.h"


/* Implementation *************************************************************/
CLlconServerDlg::CLlconServerDlg ( CServer* pNServP, QWidget* parent )
    : QDialog ( parent ), pServer ( pNServP )
{
    setupUi ( this );

    // set text for version and application name
    TextLabelNameVersion->setText ( QString ( APP_NAME ) +
        tr ( " server " ) + QString ( VERSION ) );

    // set up list view for connected clients
    ListViewClients->setColumnWidth ( 0, 170 );
    ListViewClients->setColumnWidth ( 1, 130 );
    ListViewClients->setColumnWidth ( 2, 60 );
    ListViewClients->clear();

    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( USED_NUM_CHANNELS );
    for ( int i = USED_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new CServerListViewItem ( ListViewClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // update central server name line edit
    LineEditCentralServerAddress->setText (
        pServer->GetServerListCentralServerAddress() );

    // update server name line edit
    LineEditServerName->setText ( pServer->GetServerName() );

    // update server city line edit
    LineEditLocationCity->setText ( pServer->GetServerCity() );

    // load country combo box with all available countries
    ComboBoxLocationCountry->setInsertPolicy ( QComboBox::NoInsert );
    ComboBoxLocationCountry->clear();

    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
          iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // add all countries except of the "Default" country
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // store the country enum index together with the string (this is
            // important since we sort the combo box items later on)
            ComboBoxLocationCountry->addItem ( QLocale::countryToString (
                static_cast<QLocale::Country> ( iCurCntry ) ), iCurCntry );
        }
    }

    // sort country combo box items in alphabetical order
    ComboBoxLocationCountry->model()->sort ( 0, Qt::AscendingOrder );

    // select current country
    ComboBoxLocationCountry->setCurrentIndex (
        ComboBoxLocationCountry->findData (
        static_cast<int> ( pServer->GetServerCountry() ) ) );

    // update register server check box
    const bool bCurSerListEnabled = pServer->GetServerListEnabled();

    if ( bCurSerListEnabled )
    {
        cbRegisterServer->setCheckState ( Qt::Checked );
    }
    else
    {
        cbRegisterServer->setCheckState ( Qt::Unchecked );
    }

    // update GUI dependency
    UpdateServerInfosDependency ( bCurSerListEnabled );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );
    pMenu->addMenu ( new CLlconHelpMenu ( this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // check boxes
    QObject::connect ( cbRegisterServer, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnRegisterServerStateChanged ( int ) ) );

    // line edits
    QObject::connect ( LineEditCentralServerAddress,
        SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLineEditCentralServerAddressTextChanged ( const QString& ) ) );

    QObject::connect ( LineEditServerName, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLineEditServerNameTextChanged ( const QString& ) ) );

    QObject::connect ( LineEditLocationCity, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLineEditLocationCityTextChanged ( const QString& ) ) );

    // combo boxes
    QObject::connect ( ComboBoxLocationCountry, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnComboBoxLocationCountryActivated ( int ) ) );

    // timers
    QObject::connect ( &Timer, SIGNAL ( timeout() ), this, SLOT ( OnTimer() ) );


    // Timers ------------------------------------------------------------------
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );
}

void CLlconServerDlg::OnRegisterServerStateChanged ( int value )
{
    const bool bEnabled = ( value == Qt::Checked );

    // update GUI dependency
    UpdateServerInfosDependency ( bEnabled );

    // apply new setting to the server and update it
    pServer->SetServerListEnabled ( bEnabled );
    pServer->UpdateServerList();
}

void CLlconServerDlg::OnLineEditCentralServerAddressTextChanged ( const QString& strNewAddr )
{
    // apply new setting to the server and update it
    pServer->SetServerListCentralServerAddress ( strNewAddr );

// TODO
// only apply this in case the focus is lost and the name has actually changed!
pServer->UpdateServerList();

}

void CLlconServerDlg::OnLineEditServerNameTextChanged ( const QString& strNewName )
{
    // check length
    if ( strNewName.length() <= MAX_LEN_SERVER_NAME )
    {
        // apply new setting to the server and update it
        pServer->SetServerName ( strNewName );
        pServer->UpdateServerList();
    }
    else
    {
        // text is too long, update control with shortend text
        LineEditServerName->setText ( strNewName.left ( MAX_LEN_SERVER_NAME ) );
    }
}

void CLlconServerDlg::OnLineEditLocationCityTextChanged ( const QString& strNewCity )
{
    // check length
    if ( strNewCity.length() <= MAX_LEN_SERVER_CITY )
    {
        // apply new setting to the server and update it
        pServer->SetServerCity ( strNewCity );
        pServer->UpdateServerList();
    }
    else
    {
        // text is too long, update control with shortend text
        LineEditLocationCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CLlconServerDlg::OnComboBoxLocationCountryActivated ( int iCntryListItem )
{
    // apply new setting to the server and update it
    pServer->SetServerCountry ( static_cast<QLocale::Country> (
        ComboBoxLocationCountry->itemData ( iCntryListItem ).toInt() ) );

    pServer->UpdateServerList();
}

void CLlconServerDlg::OnTimer()
{
    CVector<CHostAddress> vecHostAddresses;
    CVector<QString>      vecsName;
    CVector<int>          veciJitBufNumFrames;
    CVector<int>          veciNetwFrameSizeFact;

    ListViewMutex.lock();
    {
        pServer->GetConCliParam ( vecHostAddresses,
                                  vecsName,
                                  veciJitBufNumFrames,
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
                vecpListViewItems[i]->setText ( 3,
                    QString().setNum ( veciJitBufNumFrames[i] ) );

                // out network block size
                vecpListViewItems[i]->setText ( 4,
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
    }
    ListViewMutex.unlock();
}

void CLlconServerDlg::UpdateServerInfosDependency ( const bool bState )
{
    // if register server is not enabled, we disable all the configuration
    // controls for the server list
    LabelCentralServerAddress->setEnabled    ( bState );
    cbDefaultCentralServer->setEnabled       ( bState );
    LineEditCentralServerAddress->setEnabled ( bState );
    GroupBoxServerInfo->setEnabled           ( bState );
}

void CLlconServerDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        ListViewMutex.lock();
        {
            const int iMessType = ( (CLlconEvent*) Event )->iMessType;
            const int iStatus   = ( (CLlconEvent*) Event )->iStatus;
            const int iChanNum  = ( (CLlconEvent*) Event )->iChanNum;

            switch(iMessType)
            {
            case MS_JIT_BUF_PUT:
            case MS_JIT_BUF_GET:
                vecpListViewItems[iChanNum]->SetLight ( iStatus );
                break;
            }
        }
        ListViewMutex.unlock();
    }
}
