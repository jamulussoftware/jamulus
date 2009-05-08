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


/* Implementation *************************************************************/
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


// TEST solo functionality is not yet implemented, disable control
pcbSolo->setEnabled ( false );


    // add user controls to grid
    pMainGrid->addWidget( pFader,  0, Qt::AlignHCenter );
    pMainGrid->addWidget( pcbMute, 0, Qt::AlignHCenter );
    pMainGrid->addWidget( pcbSolo, 0, Qt::AlignHCenter );
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
        this, SLOT ( OnFaderValueChanged ( int ) ) );

    QObject::connect ( pcbMute, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnMuteStateChanged ( int ) ) );
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
}

void CChannelFader::OnFaderValueChanged ( int value )
{
    // if mute flag is set, do not apply the new fader value
    if ( pcbMute->checkState() == Qt::Unchecked )
    {
        // emit signal for new fader gain value
        emit faderValueChanged ( CalcFaderGain ( value ) );
    }
}

void CChannelFader::OnMuteStateChanged ( int value )
{
    if ( static_cast<Qt::CheckState> ( value ) == Qt::Checked )
    {
        // mute channel -> send gain of 0
        emit faderValueChanged ( 0 );
    }
    else
    {
        // mute was unchecked, get current fader value and apply
        emit faderValueChanged ( CalcFaderGain ( pFader->value() ) );
    }
}

void CChannelFader::SetText ( const QString sText )
{
    const int iBreakPos = 7;

    // break text at predefined position, if text is too short, break anyway to
    // make sure we have two lines for fader tag
    QString sModText = sText;

    if ( sModText.length() > iBreakPos )
    {
        sModText.insert ( iBreakPos, QString ( "<br>" ) );
    }
    else
    {
        // insert line break at the beginning of the string -> make sure
        // if we only have one line that the text appears at the bottom line
        sModText.insert ( 0, QString ( "<br>" ) );
    }

    // use bold text
    sModText.prepend ( "<b>" );
    sModText.append ( "</b>" );

    pLabel->setText ( sModText );
}

double CChannelFader::CalcFaderGain ( const int value )
{
    // convert actual slider range in gain values
    // and normalize so that maximum gain is 1
    return static_cast<double> ( value ) / AUD_MIX_FADER_MAX;
}

CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent, Qt::WindowFlags f ) : QFrame ( parent, f )
{
    // set modified style
    setFrameShape  ( QFrame::StyledPanel );
    setFrameShadow ( QFrame::Sunken );

    // add hboxlayout with horizontal spacer
    pMainLayout = new QHBoxLayout ( this );

    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this, pMainLayout );
        vecpChanFader[i]->Hide();
    }


    // connections -------------------------------------------------------------
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    QObject::connect ( vecpChanFader[0], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh0 ( double ) ) );
    QObject::connect ( vecpChanFader[1], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh1 ( double ) ) );
    QObject::connect ( vecpChanFader[2], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh2 ( double ) ) );
    QObject::connect ( vecpChanFader[3], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh3 ( double ) ) );
    QObject::connect ( vecpChanFader[4], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh4 ( double ) ) );
    QObject::connect ( vecpChanFader[5], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh5 ( double ) ) );
    QObject::connect ( vecpChanFader[6], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh6 ( double ) ) );
    QObject::connect ( vecpChanFader[7], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh7 ( double ) ) );
    QObject::connect ( vecpChanFader[8], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh8 ( double ) ) );
    QObject::connect ( vecpChanFader[9], SIGNAL ( faderValueChanged ( double ) ), this, SLOT ( OnFaderValueChangedCh9 ( double ) ) );
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
            // search if current fader is used
            if ( vecChanInfo[j].iChanID == i )
            {
                // check if fader was already in use -> preserve gain value
                if ( !vecpChanFader[i]->IsVisible() )
                {
                    vecpChanFader[i]->Reset();

                    // show fader
                    vecpChanFader[i]->Show();
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
