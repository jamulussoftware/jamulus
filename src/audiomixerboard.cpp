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

    // setup layout
    pMainGrid->setSpacing ( 2 );

    // setup slider
    pFader->setPageStep ( 1 );
    pFader->setTickPosition ( QSlider::TicksBothSides );
    pFader->setRange ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );

    // setup fader tag label (use white background of label)
    QPalette newPalette = pLabel->palette();
    newPalette.setColor ( QPalette::Active, QPalette::Window,
        newPalette.color ( QPalette::Active, QPalette::Base ) );
    newPalette.setColor ( QPalette::Disabled, QPalette::Window,
        newPalette.color ( QPalette::Disabled, QPalette::Base ) );
    newPalette.setColor ( QPalette::Inactive, QPalette::Window,
        newPalette.color ( QPalette::Inactive, QPalette::Base ) );

    pLabel->setPalette ( newPalette );
    pLabel->setAutoFillBackground ( true );
    pLabel->setFrameShape ( QFrame::Box );

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

    pcbMute->setWhatsThis ( "<b>Mute:</b> Mutes the current channel." );

    pcbSolo->setWhatsThis ( "<b>Solo:</b> If Solo checkbox is checked, only "
        "the current channel is active and all other channels are muted. Only "
        "one channel at a time can be set to solo." );

    pLabel->setWhatsThis ( "<b>Mixer Fader Label:</b> Label (fader tag) "
        "identifying the connected client. The tag name can be set in the "
        "clients main window." );


    // connections -------------------------------------------------------------
    QObject::connect ( pFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnGainValueChanged ( int ) ) );

    QObject::connect ( pcbMute, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnMuteStateChanged ( int ) ) );

    QObject::connect ( pcbSolo, SIGNAL ( stateChanged ( int ) ),
        this, SIGNAL ( soloStateChanged ( int ) ) );
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
    // if mute flag is set, do not apply the new fader value
    if ( pcbMute->checkState() == Qt::Unchecked )
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
    // muted but all others, we simply have to uncheck the check box
    pcbSolo->setChecked ( false );
}

void CChannelFader::SetOtherSoloState ( const bool bState )
{
    // store state (must be done before the SetMute() call!)
    bOtherChannelIsSolo = bState;

    // check if we are in solo on state, in that case we first have to disable
    // our solo state since only one channel can be set to solo
    if ( bState && pcbSolo->isChecked() )
    {
        pcbSolo->setChecked ( false );
    }

    // if other channel is solo, mute this channel, else enable channel gain
    // (only enable channel gain if local mute switch is not set to on)
    if ( !pcbMute->isChecked()  )
    {
        SetMute ( bState );
    }
}

void CChannelFader::SetText ( const QString sText )
{
    const int iBreakPos = 8;

    // make sure we insert an HTML space ("&nbsp;") at each beginning and end
    // of line for nicer look

    // break text at predefined position, if text is too short, break anyway to
    // make sure we have two lines for fader tag
    QString sModText = sText;

    if ( sModText.length() > iBreakPos )
    {
        sModText.insert ( iBreakPos, QString ( "&nbsp;<br>&nbsp;" ) );
    }
    else
    {
        // insert line break at the beginning of the string -> make sure
        // if we only have one line that the text appears at the bottom line
        sModText.insert ( 0, QString ( "&nbsp;<br>&nbsp;" ) );
    }

    // use bold centered text
    sModText.prepend ( "<center><b>&nbsp;" );
    sModText.append ( "&nbsp;</b></center>" );

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
CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent, Qt::WindowFlags f ) :
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


    // connections -------------------------------------------------------------
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    QObject::connect ( vecpChanFader[0], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh0 ( double ) ) );
    QObject::connect ( vecpChanFader[1], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh1 ( double ) ) );
    QObject::connect ( vecpChanFader[2], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh2 ( double ) ) );
    QObject::connect ( vecpChanFader[3], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh3 ( double ) ) );
    QObject::connect ( vecpChanFader[4], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh4 ( double ) ) );
    QObject::connect ( vecpChanFader[5], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh5 ( double ) ) );
    QObject::connect ( vecpChanFader[6], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh6 ( double ) ) );
    QObject::connect ( vecpChanFader[7], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh7 ( double ) ) );
    QObject::connect ( vecpChanFader[8], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh8 ( double ) ) );
    QObject::connect ( vecpChanFader[9], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh9 ( double ) ) );

    QObject::connect ( vecpChanFader[0], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh0 ( int ) ) );
    QObject::connect ( vecpChanFader[1], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh1 ( int ) ) );
    QObject::connect ( vecpChanFader[2], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh2 ( int ) ) );
    QObject::connect ( vecpChanFader[3], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh3 ( int ) ) );
    QObject::connect ( vecpChanFader[4], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh4 ( int ) ) );
    QObject::connect ( vecpChanFader[5], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh5 ( int ) ) );
    QObject::connect ( vecpChanFader[6], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh6 ( int ) ) );
    QObject::connect ( vecpChanFader[7], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh7 ( int ) ) );
    QObject::connect ( vecpChanFader[8], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh8 ( int ) ) );
    QObject::connect ( vecpChanFader[9], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChangedCh9 ( int ) ) );
}

void CAudioMixerBoard::HideAll()
{
    // make old controls invisible
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->Hide();
    }
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo )
{
    // search for channels with are already present and preserver their gain
    // setting, for all other channels, reset gain
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        bool bFaderIsUsed = false;

        for ( int j = 0; j < vecChanInfo.Size(); j++ )
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
}

void CAudioMixerBoard::OnChSoloStateChanged ( const int iChannelIdx, const int iValue )
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
