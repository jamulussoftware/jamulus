/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *  
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
                               QHBoxLayout* pParentLayout,
                               QString      sName )
{
    // create new GUI control objects and store pointers to them
    pMainGrid = new QGridLayout ( 2, 1 );
    pFader    = new QSlider     ( Qt::Vertical, pNW );
    pLabel    = new QLabel      ( "", pNW );

    // setup slider
    pFader->setPageStep ( 1 );
    pFader->setTickmarks ( QSlider::Both );
    pFader->setRange ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );
    pFader->setValue ( 0 ); // set init value

    // set label text
    pLabel->setText ( sName );

    // add slider to grid as position 0 / 0
    pMainGrid->addWidget( pFader, 0, 0, Qt::AlignHCenter );

    // add label to grid as position 1 / 0
    pMainGrid->addWidget( pLabel, 1, 0, Qt::AlignHCenter );

    pParentLayout->insertLayout ( 0, pMainGrid );

    // add help text to controls
    QWhatsThis::add(pFader, "<b>Mixer Fader:</b> Adjusts the audio level of this "
		"channel. All connected clients at the server will be assigned an audio "
		"fader at each client");

    QWhatsThis::add(pLabel, "<b>Mixer Fader Label:</b> Label (fader tag) identifying "
		"the connected client. The tag name can be set in the clients main window.");


    // connections -------------------------------------------------------------
    QObject::connect ( pFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnValueChanged ( int ) ) );
}

void CChannelFader::OnValueChanged ( int value )
{
    // convert actual slider range in gain values
    // reverse linear scale and normalize so that maximum gain is 1
    const double dCurGain =
        static_cast<double> ( AUD_MIX_FADER_MAX - value ) / AUD_MIX_FADER_MAX;

    emit valueChanged ( dCurGain );
}

void CChannelFader::SetText ( const std::string sText )
{
    const int iBreakPos = 7;

    // break text at predefined position, if text is too short, break anyway to
    // make sure we have two lines for fader tag
    QString sModText = sText.c_str();

    if ( sModText.length() > iBreakPos )
    {
        sModText.insert ( iBreakPos, QString ( "<br>" ) );
    }
    else
    {
        sModText.append ( QString ( "<br>" ) );
    }

    // use bold text
    sModText.prepend ( "<b>" );
    sModText.append ( "</b>" );

    pLabel->setText ( sModText );
}

CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent,
                                     const char* name,
                                     WFlags f ) : 
    QFrame ( parent, name, f )
{
    // set modified style
    setFrameShape  ( QFrame::StyledPanel );
    setFrameShadow ( QFrame::Sunken );

    // add hboxlayout with horizontal spacer
    pMainLayout = new QHBoxLayout ( this, 11, 0 );
    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this, pMainLayout, "" );
        vecpChanFader[i]->Hide();
    }


    // connections -------------------------------------------------------------
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    QObject::connect(vecpChanFader[0],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh0(double)));
    QObject::connect(vecpChanFader[1],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh1(double)));
    QObject::connect(vecpChanFader[2],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh2(double)));
    QObject::connect(vecpChanFader[3],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh3(double)));
    QObject::connect(vecpChanFader[4],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh4(double)));
    QObject::connect(vecpChanFader[5],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh5(double)));
    QObject::connect(vecpChanFader[6],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh6(double)));
    QObject::connect(vecpChanFader[7],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh7(double)));
    QObject::connect(vecpChanFader[8],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh8(double)));
    QObject::connect(vecpChanFader[9],SIGNAL(valueChanged(double)),this,SLOT(OnValueChangedCh9(double)));
}

void CAudioMixerBoard::HideAll()
{
    // make old controls invisible
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->Hide();
    }
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo )
{
    // search for channels with are already present and preserver their gain
    // setting, for all other channels, reset gain
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
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
                    vecpChanFader[i]->ResetGain();

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

std::string CAudioMixerBoard::GenFaderText ( CChannelShortInfo& ChanInfo )
{
    // if text is empty, show IP address instead
    if ( ChanInfo.strName.empty() )
    {
        // convert IP address to text and show it
        const QHostAddress addrTest ( ChanInfo.iIpAddr );
        return addrTest.toString().latin1();
    }
    else
    {
        // show name of channel
        return ChanInfo.strName;
    }
}
