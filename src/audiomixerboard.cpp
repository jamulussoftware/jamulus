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

#include "audiomixerboard.h"


/******************************************************************************\
* CChanneFader                                                                 *
\******************************************************************************/
CChannelFader::CChannelFader ( QWidget*     pNW,
                               QHBoxLayout* pParentLayout )
{
    // create new GUI control objects and store pointers to them
    pMainGrid = new QVBoxLayout();
    pFader    = new QSlider   ( Qt::Vertical, pNW );
    pcbMute   = new QCheckBox ( "Mute",       pNW );
    pcbSolo   = new QCheckBox ( "Solo",       pNW );
    pLabel    = new QLabel    ( "",           pNW );

    // setup slider
    pFader->setPageStep ( 1 );
    pFader->setTickPosition ( QSlider::TicksBothSides );
    pFader->setRange ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );

    // setup fader tag label (use white background of label)
    pLabel->setTextFormat ( Qt::PlainText ); 	 
	pLabel->setAlignment ( Qt::AlignHCenter );
    pLabel->setStyleSheet (
        "QLabel { border:           2px solid black;"
        "         border-radius:    4px;"
        "         padding:          4px;"
        "         background-color: white;"
        "         color:            black;"
        "         font:             bold; }" );

    // add user controls to grid
    pMainGrid->addWidget( pFader,  0, Qt::AlignHCenter );
    pMainGrid->addWidget( pcbMute, 0, Qt::AlignLeft );
    pMainGrid->addWidget( pcbSolo, 0, Qt::AlignLeft );
    pMainGrid->addWidget( pLabel,  0, Qt::AlignHCenter );

    // add fader layout to audio mixer board layout
    pParentLayout->addLayout ( pMainGrid );

    // reset current fader
    Reset();

    // add help text to controls
    pFader->setWhatsThis ( "<b>Mixer Fader:</b> Adjusts the audio level of "
        "this channel. All connected clients at the server will be assigned "
        "an audio fader at each client." );
    pFader->setAccessibleName ( "Mixer level setting of the connected client "
        "at the server" );

    pcbMute->setWhatsThis ( "<b>Mute:</b> With the Mute checkbox, the current "
        "audio channel can be muted." );
    pcbMute->setAccessibleName ( "Mute button" );

    pcbSolo->setWhatsThis ( "<b>Solo:</b> With the Solo checkbox, the current "
        "audio channel can be set to solo which means that all other channels "
        "except of the current channel are muted.<br>"
        "Only one channel at a time can be set to solo." );
    pcbSolo->setAccessibleName ( "Solo button" );

    pLabel->setWhatsThis ( "<b>Mixer Fader Label:</b> Label (fader tag) "
        "identifying the connected client. The tag name can be set in the "
        "clients main window." );
    pLabel->setAccessibleName ( "Mixer level label (fader tag)" );


    // Connections -------------------------------------------------------------
    QObject::connect ( pFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnGainValueChanged ( int ) ) );

    QObject::connect ( pcbMute, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnMuteStateChanged ( int ) ) );

    QObject::connect ( pcbSolo,
        SIGNAL ( stateChanged ( int ) ),
        SIGNAL ( soloStateChanged ( int ) ) );
}

void CChannelFader::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    switch ( eNewDesign )
    {
    case GD_ORIGINAL:
        // fader
        pFader->setStyleSheet (
            "QSlider { width:         45px;"
            "          border-image:  url(:/png/fader/res/faderbackground.png) repeat;"
            "          border-top:    10px transparent;"
            "          border-bottom: 10px transparent;"
            "          border-left:   20px transparent;"
            "          border-right:  -25px transparent; }"
            "QSlider::groove { image:          url();"
            "                  padding-left:   -38px;"
            "                  padding-top:    -10px;"
            "                  padding-bottom: -15px; }"
            "QSlider::handle { image: url(:/png/fader/res/faderhandle.png); }" );

        // mute button
        pcbMute->setStyleSheet (
            "QCheckBox::indicator { width:  38px;"
            "                       height: 21px; }"
            "QCheckBox::indicator:unchecked {"
            "    image: url(:/png/fader/res/ledbuttonnotpressed.png); }"
            "QCheckBox::indicator:checked {"
            "    image: url(:/png/fader/res/ledbuttonpressed.png); }"
            "QCheckBox { color: rgb(148, 148, 148);"
            "            font:  bold; }" );
        pcbMute->setText ( "MUTE" );

        // solo button
        pcbSolo->setStyleSheet (
            "QCheckBox::indicator { width:  38px;"
            "                       height: 21px; }"
            "QCheckBox::indicator:unchecked {"
            "    image: url(:/png/fader/res/ledbuttonnotpressed.png); }"
            "QCheckBox::indicator:checked {"
            "    image: url(:/png/fader/res/ledbuttonpressed.png); }"
            "QCheckBox { color: rgb(148, 148, 148);"
            "            font:  bold; }" );
        pcbSolo->setText ( "SOLO" );
        break;

    default:
        // reset style sheet and set original paramters
        pFader->setStyleSheet  ( "" );
        pcbMute->setStyleSheet ( "" );
        pcbSolo->setStyleSheet ( "" );
        pcbMute->setText       ( "Mute" );
        pcbSolo->setText       ( "Solo" );
        break;
    }
}

void CChannelFader::Reset()
{
    // init gain value -> maximum value as definition according to server
    pFader->setValue ( AUD_MIX_FADER_MAX );

    // reset mute/solo check boxes
    pcbMute->setChecked ( false );
    pcbSolo->setChecked ( false );

    // clear label
    pLabel->setText ( "" );

    bOtherChannelIsSolo = false;
}

void CChannelFader::OnGainValueChanged ( int value )
{
    // if mute flag is set or other channel is on solo, do not apply the new
    // fader value
    if ( ( pcbMute->checkState() == Qt::Unchecked ) && !bOtherChannelIsSolo )
    {
        // emit signal for new fader gain value
        emit gainValueChanged ( CalcFaderGain ( value ) );
    }
}

void CChannelFader::OnMuteStateChanged ( int value )
{
    // call muting function
    SetMute ( static_cast<Qt::CheckState> ( value ) == Qt::Checked );
}

void CChannelFader::SetMute ( const bool bState )
{
    if ( bState )
    {
        // mute channel -> send gain of 0
        emit gainValueChanged ( 0 );
    }
    else
    {
        // only unmute if no other channel is on solo
        if ( !bOtherChannelIsSolo )
        {
            // mute was unchecked, get current fader value and apply
            emit gainValueChanged ( CalcFaderGain ( pFader->value() ) );
        }
    }
}

void CChannelFader::ResetSoloState()
{
    // reset solo state -> since solo state means that this channel is not
    // muted but all others, we simply have to uncheck the check box (make
    // sure the setChecked does not fire a signal)
    pcbSolo->blockSignals ( true );
    {
        pcbSolo->setChecked ( false );
    }
    pcbSolo->blockSignals ( false );
}

void CChannelFader::SetOtherSoloState ( const bool bState )
{
    // store state (must be done before the SetMute() call!)
    bOtherChannelIsSolo = bState;

    // check if we are in solo on state, in that case we first have to disable
    // our solo state since only one channel can be set to solo
    if ( bState && pcbSolo->isChecked() )
    {
        // we do not want to fire a signal with the following set function
        // -> block signals temporarily
        pcbSolo->blockSignals ( true );
        {
            pcbSolo->setChecked ( false );
        }
        pcbSolo->blockSignals ( false );
    }

    // if other channel is solo, mute this channel, else enable channel gain
    // (only enable channel gain if local mute switch is not set to on)
    if ( !pcbMute->isChecked() )
    {
        SetMute ( bState );
    }
}

void CChannelFader::SetText ( const QString sText )
{
    const int iBreakPos = MAX_LEN_FADER_TAG / 2;

    // break text at predefined position, if text is too short, break anyway to
    // make sure we have two lines for fader tag
    QString sModText = sText;

    if ( sModText.length() > iBreakPos )
    {
        sModText.insert ( iBreakPos, QString ( "\n" ) );
    }
    else
    {
        // insert line break at the beginning of the string -> make sure
        // if we only have one line that the text appears at the bottom line
        sModText.prepend ( QString ( "\n" ) );
    }

    pLabel->setText ( sModText );
}

double CChannelFader::CalcFaderGain ( const int value )
{
    // convert actual slider range in gain values
    // and normalize so that maximum gain is 1
    return static_cast<double> ( value ) / AUD_MIX_FADER_MAX;
}


/******************************************************************************\
* CAudioMixerBoard                                                             *
\******************************************************************************/
CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent, Qt::WindowFlags ) :
    QGroupBox ( parent )
{
    // set title text and title properties
    setTitle ( "Server" );

    // add hboxlayout
    pMainLayout = new QHBoxLayout ( this );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this, pMainLayout );
        vecpChanFader[i]->Hide();
    }

    // insert horizontal spacer
    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );


    // Connections -------------------------------------------------------------
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    QObject::connect ( vecpChanFader[0],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh0  ( double ) ) );
    QObject::connect ( vecpChanFader[1],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh1  ( double ) ) );
    QObject::connect ( vecpChanFader[2],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh2  ( double ) ) );
    QObject::connect ( vecpChanFader[3],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh3  ( double ) ) );
    QObject::connect ( vecpChanFader[4],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh4  ( double ) ) );
    QObject::connect ( vecpChanFader[5],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh5  ( double ) ) );
    QObject::connect ( vecpChanFader[6],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh6  ( double ) ) );
    QObject::connect ( vecpChanFader[7],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh7  ( double ) ) );
    QObject::connect ( vecpChanFader[8],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh8  ( double ) ) );
    QObject::connect ( vecpChanFader[9],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh9  ( double ) ) );
    QObject::connect ( vecpChanFader[10], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh10 ( double ) ) );
    QObject::connect ( vecpChanFader[11], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh11 ( double ) ) );

    QObject::connect ( vecpChanFader[0],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh0  ( int ) ) );
    QObject::connect ( vecpChanFader[1],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh1  ( int ) ) );
    QObject::connect ( vecpChanFader[2],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh2  ( int ) ) );
    QObject::connect ( vecpChanFader[3],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh3  ( int ) ) );
    QObject::connect ( vecpChanFader[4],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh4  ( int ) ) );
    QObject::connect ( vecpChanFader[5],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh5  ( int ) ) );
    QObject::connect ( vecpChanFader[6],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh6  ( int ) ) );
    QObject::connect ( vecpChanFader[7],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh7  ( int ) ) );
    QObject::connect ( vecpChanFader[8],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh8  ( int ) ) );
    QObject::connect ( vecpChanFader[9],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh9  ( int ) ) );
    QObject::connect ( vecpChanFader[10], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh10 ( int ) ) );
    QObject::connect ( vecpChanFader[11], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh11 ( int ) ) );
}

void CAudioMixerBoard::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    // apply GUI design to child GUI controls
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->SetGUIDesign ( eNewDesign );
    }
}

void CAudioMixerBoard::HideAll()
{
    // make all controls invisible
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->Hide();
    }

    // emit status of connected clients
    emit NumClientsChanged ( 0 ); // -> no clients connected
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo )
{
    // get number of connected clients
    const int iNumConnectedClients = vecChanInfo.Size();

    // search for channels with are already present and preserver their gain
    // setting, for all other channels, reset gain
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        bool bFaderIsUsed = false;

        for ( int j = 0; j < iNumConnectedClients; j++ )
        {
            // check if current fader is used
            if ( vecChanInfo[j].iChanID == i )
            {
                // check if fader was already in use -> preserve gain value
                if ( !vecpChanFader[i]->IsVisible() )
                {
                    vecpChanFader[i]->Reset();

                    // show fader
                    vecpChanFader[i]->Show();
                }
                else
                {
                    // by definition disable all Solo switches if the number of
                    // channels at the server has changed
                    vecpChanFader[i]->SetOtherSoloState ( false );
                    vecpChanFader[i]->ResetSoloState();
                }

                // update text
                vecpChanFader[i]->SetText ( GenFaderText ( vecChanInfo[j] ) );

                bFaderIsUsed = true;
            }
        }

        // if current fader is not used, hide it
        if ( !bFaderIsUsed )
        {
            vecpChanFader[i]->Hide();
        }
    }

    // emit status of connected clients
    emit NumClientsChanged ( iNumConnectedClients );
}

void CAudioMixerBoard::OnChSoloStateChanged ( const int iChannelIdx,
                                              const int iValue )
{
    // if channel iChannelIdx has just activated the solo switch, mute all
    // other channels, else enable them again
    const bool bSetOtherSoloState =
        ( static_cast<Qt::CheckState> ( iValue ) == Qt::Checked );

    // apply "other solo state" for all other channels
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( i != iChannelIdx )
        {
            // check if fader is in use
            if ( vecpChanFader[i]->IsVisible() )
            {
                vecpChanFader[i]->SetOtherSoloState ( bSetOtherSoloState );
            }
        }
    }

    // set "other solo state" always to false for the current fader at which the
    // status was changed because if solo is enabled, it has to be "false" and
    // in case solo is just disabled (check was removed by the user), also no
    // other channel can be solo at this time
    vecpChanFader[iChannelIdx]->SetOtherSoloState ( false );
}

QString CAudioMixerBoard::GenFaderText ( CChannelShortInfo& ChanInfo )
{
    // if text is empty, show IP address instead
    if ( ChanInfo.strName.isEmpty() )
    {
        // convert IP address to text and show it (use dummy port number
        // since it is not used here)
        const CHostAddress TempAddr =
            CHostAddress ( QHostAddress ( ChanInfo.iIpAddr ), 0 );

        return TempAddr.GetIpAddressStringNoLastByte();
    }
    else
    {
        // show name of channel
        return ChanInfo.strName;
    }
}
