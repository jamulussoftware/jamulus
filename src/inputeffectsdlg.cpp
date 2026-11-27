/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  Serhii Popov (uazure@gmail.com)
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
\******************************************************************************/

#include "inputeffectsdlg.h"

/* Implementation *************************************************************/
CInputEffectsDlg::CInputEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ), // use Qt::Window to get min/max window buttons
    pClient ( pNCliP ),
    pSettings ( pNSetP ),
    bMobile ( false ),
    iMobileLane ( 0 )
{
    setupUi ( this );

#if defined( Q_OS_IOS )
    // iOS needs menu to close
    QMenuBar* pMenu  = new QMenuBar ( this );
    QAction*  action = pMenu->addAction ( tr ( "&Close" ) );
    connect ( action, SIGNAL ( triggered() ), this, SLOT ( close() ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );
    bMobile = true;
#endif

#if defined( Q_OS_ANDROID ) || defined( ANDROID )
    // Android too
    QMenuBar* pMenu      = new QMenuBar ( this );
    QMenu*    pCloseMenu = new QMenu ( tr ( "&Close" ), this );
    pCloseMenu->addAction ( tr ( "&Close" ), this, SLOT ( close() ) );
    pMenu->addMenu ( pCloseMenu );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );
    bMobile = true;
#endif

    // Add help text to controls -----------------------------------------------
    QString strLinked = "<b>" + tr ( "Same settings for L and R" ) + ":</b> " +
                        tr ( "If checked, the effects are set once and applied to both input "
                             "channels in the same way. Uncheck it to set up the left and right "
                             "input channel separately, e.g. a microphone on one and an "
                             "instrument on the other channel." );

    chbLinked->setWhatsThis ( strLinked );
    chbLinked->setAccessibleName ( tr ( "Same settings for left and right check box" ) );

    QString strChannel = "<b>" + tr ( "Channel" ) + ":</b> " +
                         tr ( "Select which input channel to set up. \"Both\" sets the effects "
                              "once for the left and right input channel." );

    lblChannel->setWhatsThis ( strChannel );
    cbxChannel->setWhatsThis ( strChannel );
    cbxChannel->setAccessibleName ( tr ( "Input channel selection combo box" ) );

    QString strPreview = "<b>" + tr ( "Preview" ) + ":</b> " +
                         tr ( "Plays your processed sound locally so that you can check the "
                              "effects. Use headphones to avoid feedback. The preview is only "
                              "available while you are not connected to a server." );

    butPreview->setWhatsThis ( strPreview );
    butPreview->setToolTip ( "<p>" + tr ( "Play your processed sound locally to check the effects. Use headphones to avoid feedback." ) + "</p>" );
    butPreview->setAccessibleName ( tr ( "Preview toggle button" ) );

    // local audio input fader
    QString strAudFader = "<b>" + tr ( "Input Balance" ) + ":</b> " +
                          tr ( "Controls the relative levels of the left and right local audio "
                               "channels. For a mono signal it acts as a pan between the two channels. "
                               "For example, if a microphone is connected to "
                               "the right input channel and an instrument is connected to the left "
                               "input channel which is much louder than the microphone, move the "
                               "audio fader in a direction where the label next to the fader shows "
                               "%1, where %2 is the current attenuation indicator." )
                              .arg ( "<i>" + tr ( "L" ) + " -x</i>", "<i>x</i>" );

    lblAudioPan->setWhatsThis ( strAudFader );
    lblAudioPanValue->setWhatsThis ( strAudFader );
    sldAudioPan->setWhatsThis ( strAudFader );
    sldAudioPan->setAccessibleName ( tr ( "Local audio input fader (left/right)" ) );

    // short tooltips (rich text, so that long tooltips wrap)
    chbLinked->setToolTip ( "<p>" + tr ( "Use one set of effects for both input channels." ) + "</p>" );
    cbxChannel->setToolTip ( "<p>" + tr ( "Input channel to set up." ) + "</p>" );
    sldAudioPan->setToolTip ( "<p>" + tr ( "Balance between the left and right input channel." ) + "</p>" );

    // init controls -----------------------------------------------------------
    wdgLaneL->SetClientAndLane ( pClient, 0 );
    wdgLaneR->SetClientAndLane ( pClient, 1 );

#ifdef NO_INTERNAL_EFFECTS
    chbLinked->hide();
    lblChannel->hide();
    cbxChannel->hide();
#else
    lblNoEffects->hide();
#endif

    // init audio in fader
    sldAudioPan->setRange ( AUD_FADER_IN_MIN, AUD_FADER_IN_MAX );
    sldAudioPan->setTickInterval ( AUD_FADER_IN_MAX / 10 ); // a tick at the center position
    UpdateAudioFaderSlider();

    UpdateChannelLayout();
    SetConnected ( false );

    // Connections -------------------------------------------------------------
    QObject::connect ( chbLinked, &QCheckBox::toggled, this, &CInputEffectsDlg::OnLinkedToggled );

    QObject::connect ( cbxChannel,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CInputEffectsDlg::OnChannelActivated );

    QObject::connect ( wdgLaneL, &CInputEffectLane::ParamChanged, this, &CInputEffectsDlg::OnLaneParamChanged );

    QObject::connect ( wdgLaneR, &CInputEffectLane::ParamChanged, this, &CInputEffectsDlg::OnLaneParamChanged );

    QObject::connect ( butPreview, &QPushButton::toggled, this, &CInputEffectsDlg::OnPreviewToggled );

    QObject::connect ( pClient, &CClient::LocalPreviewChanged, this, &CInputEffectsDlg::OnLocalPreviewChanged );

    QObject::connect ( sldAudioPan, &QSlider::valueChanged, this, &CInputEffectsDlg::OnAudioPanValueChanged );
}

void CInputEffectsDlg::UpdateChannelLayout()
{
    const bool bStereo = ( pClient->GetAudioChannels() == CC_STEREO );
    const bool bLinked = pSettings->bInputEffectsLinked;

    if ( bLinked && !bStereo )
    {
        // only the left lane is shown and edited: make sure the right lane
        // processes with the same parameters (e.g. after a switch from stereo
        // mode, where the right lane is not used)
        CopyLaneParams ( 0, 1 );
    }

#ifndef NO_INTERNAL_EFFECTS
    // the linking controls make no sense in stereo mode (one source)
    chbLinked->setVisible ( !bStereo && !bMobile );
    lblChannel->setVisible ( !bStereo && bMobile );
    cbxChannel->setVisible ( !bStereo && bMobile );

    chbLinked->blockSignals ( true );
    chbLinked->setChecked ( bLinked );
    chbLinked->blockSignals ( false );

    cbxChannel->blockSignals ( true );
    cbxChannel->setCurrentIndex ( bLinked ? 0 : iMobileLane + 1 );
    cbxChannel->blockSignals ( false );
#endif

    if ( bStereo )
    {
        wdgLaneL->setTitle ( tr ( "Stereo" ) );
        wdgLaneL->setVisible ( true );
        wdgLaneR->setVisible ( false );
    }
    else if ( bLinked )
    {
        wdgLaneL->setTitle ( tr ( "Left and Right" ) );
        wdgLaneL->setVisible ( true );
        wdgLaneR->setVisible ( false );
    }
    else
    {
        wdgLaneL->setTitle ( tr ( "Left (In 1)" ) );
        wdgLaneR->setTitle ( tr ( "Right (In 2)" ) );
        wdgLaneL->setVisible ( !bMobile || ( iMobileLane == 0 ) );
        wdgLaneR->setVisible ( !bMobile || ( iMobileLane == 1 ) );
    }

    wdgLaneL->UpdateFromClient();
    wdgLaneR->UpdateFromClient();
}

void CInputEffectsDlg::UpdateMeters()
{
    wdgLaneL->UpdateMeters ( pClient->GetLevelForMeterdBRawLeft(), pClient->GetLevelForMeterdBLeft() );
    wdgLaneR->UpdateMeters ( pClient->GetLevelForMeterdBRawRight(), pClient->GetLevelForMeterdBRight() );
}

void CInputEffectsDlg::ResetMeters()
{
    wdgLaneL->ResetMeters();
    wdgLaneR->ResetMeters();
}

void CInputEffectsDlg::SetMeterStyle ( const EMeterStyle eNewMeterStyle )
{
    wdgLaneL->SetMeterStyle ( eNewMeterStyle );
    wdgLaneR->SetMeterStyle ( eNewMeterStyle );
}

void CInputEffectsDlg::SetConnected ( const bool bConnected )
{
    // the sound card is in use while connected
    butPreview->setEnabled ( !bConnected );
}

void CInputEffectsDlg::UpdateAudioFaderSlider()
{
    // update slider and label of audio fader
    const int iCurAudInFader = pClient->GetAudioInFader();
    sldAudioPan->setValue ( iCurAudInFader );

    // show in label the center position and what channel is
    // attenuated
    if ( iCurAudInFader == AUD_FADER_IN_MIDDLE )
    {
        lblAudioPanValue->setText ( tr ( "Center" ) );
    }
    else
    {
        if ( iCurAudInFader > AUD_FADER_IN_MIDDLE )
        {
            // attenuation on right channel
            lblAudioPanValue->setText ( tr ( "L" ) + " -" + QString().setNum ( iCurAudInFader - AUD_FADER_IN_MIDDLE ) );
        }
        else
        {
            // attenuation on left channel
            lblAudioPanValue->setText ( tr ( "R" ) + " -" + QString().setNum ( AUD_FADER_IN_MIDDLE - iCurAudInFader ) );
        }
    }
}

void CInputEffectsDlg::hideEvent ( QHideEvent* )
{
    // the preview belongs to this window
    StopPreview();
}

void CInputEffectsDlg::StopPreview()
{
    if ( pClient->IsLocalPreviewActive() )
    {
        try
        {
            pClient->SetLocalPreview ( false );
        }
        catch ( const CGenErr& )
        {
            // stopping the preview failed, nothing more to do here
        }
    }
}

void CInputEffectsDlg::CopyLaneParams ( const int iFrom, const int iTo )
{
#ifndef NO_INTERNAL_EFFECTS
    CInputEffectChain& Fx = pClient->GetInputEffects();

    Fx.Boost ( iTo ).SetEnabled ( Fx.Boost ( iFrom ).IsEnabled() );
    Fx.Boost ( iTo ).SetGainDb ( Fx.Boost ( iFrom ).GetGainDb() );
    Fx.Compressor ( iTo ).SetEnabled ( Fx.Compressor ( iFrom ).IsEnabled() );
    Fx.Compressor ( iTo ).SetRatioIdx ( Fx.Compressor ( iFrom ).GetRatioIdx() );
    Fx.Compressor ( iTo ).SetThresholdDb ( Fx.Compressor ( iFrom ).GetThresholdDb() );
    Fx.Reverb ( iTo ).SetEnabled ( Fx.Reverb ( iFrom ).IsEnabled() );
    Fx.Reverb ( iTo ).SetLevel ( Fx.Reverb ( iFrom ).GetLevel() );
#else
    Q_UNUSED ( iFrom )
    Q_UNUSED ( iTo )
#endif
}

void CInputEffectsDlg::OnLinkedToggled ( bool bChecked )
{
    pSettings->bInputEffectsLinked = bChecked;

    if ( bChecked )
    {
        // from now on the left settings are used for both channels
        CopyLaneParams ( 0, 1 );
    }

    UpdateChannelLayout();
}

void CInputEffectsDlg::OnChannelActivated ( int iIdx )
{
    // 0: both (linked), 1: left, 2: right
    if ( iIdx == 0 )
    {
        OnLinkedToggled ( true );
    }
    else
    {
        iMobileLane                    = iIdx - 1;
        pSettings->bInputEffectsLinked = false;
        UpdateChannelLayout();
    }
}

void CInputEffectsDlg::OnLaneParamChanged()
{
    if ( pSettings->bInputEffectsLinked && ( pClient->GetAudioChannels() != CC_STEREO ) )
    {
        // only the left lane is visible when linked
        CopyLaneParams ( 0, 1 );
        wdgLaneR->UpdateFromClient();
    }
}

void CInputEffectsDlg::OnPreviewToggled ( bool bChecked )
{
    try
    {
        pClient->SetLocalPreview ( bChecked );
    }
    catch ( const CGenErr& generr )
    {
        QMessageBox::critical ( this, APP_NAME, generr.GetErrorText(), tr ( "Ok" ), nullptr );
    }

    // the client reports the actual state (which may differ from the request)
    OnLocalPreviewChanged ( pClient->IsLocalPreviewActive() );
}

void CInputEffectsDlg::OnLocalPreviewChanged ( bool bActive )
{
    butPreview->blockSignals ( true );
    butPreview->setChecked ( bActive );
    butPreview->blockSignals ( false );

    if ( !bActive )
    {
        ResetMeters();
    }
}

void CInputEffectsDlg::OnAudioPanValueChanged ( int value )
{
    pClient->SetAudioInFader ( value );
    UpdateAudioFaderSlider();
}
