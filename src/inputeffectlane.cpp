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

#include "inputeffectlane.h"

/* Implementation *************************************************************/
CInputEffectLane::CInputEffectLane ( QWidget* parent ) : QGroupBox ( parent ), pClient ( nullptr ), iLane ( 0 )
{
    setupUi ( this );

    // Add help text to controls -----------------------------------------------
    QString strBoost = "<b>" + tr ( "Boost" ) + ":</b> " +
                       tr ( "Raises the input volume by up to %1 dB before the other effects. "
                            "If your sound is too quiet, first try to increase the level by "
                            "getting closer to the microphone, with the gain control of your "
                            "audio interface or in your operating system's input settings. "
                            "Only if this fails, switch on the boost. "
                            "If your sound is too loud, sounds distorted and is clipping, the "
                            "boost will not help. Instead, decrease your input level." )
                           .arg ( FX_BOOST_DB_MAX );

    grbBoost->setWhatsThis ( strBoost );
    lblBoostGain->setWhatsThis ( strBoost );
    sldBoost->setWhatsThis ( strBoost );
    lblBoostValue->setWhatsThis ( strBoost );
    grbBoost->setAccessibleName ( tr ( "Boost on/off group box" ) );
    sldBoost->setAccessibleName ( tr ( "Boost gain slider" ) );

    QString strComp = "<b>" + tr ( "Compressor" ) + ":</b> " +
                      tr ( "Compresses the dynamic range of your sound: loud parts are turned "
                           "down and the whole signal is raised to compensate, so your sound "
                           "gets louder and more even. The amount sets how strongly loud parts "
                           "are turned down, the threshold is the level above which the "
                           "compression starts. Input which is already compressed, e.g. by a "
                           "mixer or a pedal, usually needs no compression here." );

    grbComp->setWhatsThis ( strComp );
    lblCompAmount->setWhatsThis ( strComp );
    cbxCompAmount->setWhatsThis ( strComp );
    lblCompThreshold->setWhatsThis ( strComp );
    sldCompThreshold->setWhatsThis ( strComp );
    lblCompThresholdValue->setWhatsThis ( strComp );
    grbComp->setAccessibleName ( tr ( "Compressor on/off group box" ) );
    cbxCompAmount->setAccessibleName ( tr ( "Compressor amount combo box" ) );
    sldCompThreshold->setAccessibleName ( tr ( "Compressor threshold slider" ) );

    QString strReverb = "<b>" + tr ( "Reverb" ) + ":</b> " +
                        tr ( "Adds a reverberation effect to this input channel. Move the "
                             "slider to the right until the desired reverb level is reached." );

    grbReverb->setWhatsThis ( strReverb );
    lblReverbLevel->setWhatsThis ( strReverb );
    sldReverb->setWhatsThis ( strReverb );
    lblReverbValue->setWhatsThis ( strReverb );
    grbReverb->setAccessibleName ( tr ( "Reverb on/off group box" ) );
    sldReverb->setAccessibleName ( tr ( "Reverb level slider" ) );

    QString strMeters = "<b>" + tr ( "Level Meters" ) + ":</b> " +
                        tr ( "The left meter shows the level as it comes from the sound card, "
                             "the right meter shows the level after the effects, which is what "
                             "is sent to the server." );

    lbrInputRaw->setWhatsThis ( strMeters );
    lbrOutput->setWhatsThis ( strMeters );
    lblMeterIn->setWhatsThis ( strMeters );
    lblMeterOut->setWhatsThis ( strMeters );
    lbrInputRaw->setAccessibleName ( tr ( "Input level before the effects" ) );
    lbrOutput->setAccessibleName ( tr ( "Input level after the effects" ) );

    // short tooltips (rich text, so that long tooltips wrap)
    sldBoost->setToolTip (
        "<p>" + tr ( "Raises the input volume before the other effects. Prefer the gain control on your audio interface if it has one." ) + "</p>" );
    grbComp->setToolTip ( "<p>" +
                          tr ( "Compresses the dynamic range: loud parts are turned down and the whole signal is raised to compensate, "
                               "so your sound gets louder and more even." ) +
                          "</p>" );
    cbxCompAmount->setToolTip ( "<p>" + tr ( "Light: vocals, acoustic guitar." ) + "<br>" + tr ( "Moderate: expressive singing, bass, percussion." ) +
                                "<br>" + tr ( "Strong: drums, very dynamic sources." ) + "</p><p>" +
                                tr ( "Note: already compressed input (mixer, pedal) needs none." ) + "</p>" );
    sldCompThreshold->setToolTip ( "<p>" + tr ( "Level above which compression starts. Lower means more compression and more volume." ) + "</p>" );
    sldReverb->setToolTip ( "<p>" + tr ( "Amount of reverb mixed into the signal." ) + "</p>" );
    lbrInputRaw->setToolTip ( "<p>" + tr ( "Level from the sound card, before the effects." ) + "</p>" );
    lbrOutput->setToolTip ( "<p>" + tr ( "Level after the effects, as sent to the server." ) + "</p>" );

    // init controls -----------------------------------------------------------
#ifndef NO_INTERNAL_EFFECTS
    sldBoost->setRange ( 0, FX_BOOST_DB_MAX );
    sldBoost->setTickInterval ( 6 );

    cbxCompAmount->clear();
    cbxCompAmount->addItem ( tr ( "Light (2:1)" ) );
    cbxCompAmount->addItem ( tr ( "Moderate (3:1)" ) );
    cbxCompAmount->addItem ( tr ( "Strong (4:1)" ) );

    sldCompThreshold->setRange ( FX_COMP_THRESHOLD_DB_MIN, FX_COMP_THRESHOLD_DB_MAX );
    sldCompThreshold->setTickInterval ( 10 );

    sldReverb->setRange ( 0, AUD_REVERB_MAX );
    sldReverb->setTickInterval ( AUD_REVERB_MAX / 5 );
#else
    // this build has no internal effects: only the meters are shown
    grbBoost->hide();
    grbComp->hide();
    grbReverb->hide();
#endif

    ResetMeters();

    // Connections -------------------------------------------------------------
    QObject::connect ( grbBoost, &QGroupBox::toggled, this, &CInputEffectLane::OnBoostToggled );
    QObject::connect ( sldBoost, &QSlider::valueChanged, this, &CInputEffectLane::OnBoostSliderChanged );
    QObject::connect ( grbComp, &QGroupBox::toggled, this, &CInputEffectLane::OnCompToggled );
    QObject::connect ( cbxCompAmount,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CInputEffectLane::OnCompAmountActivated );
    QObject::connect ( sldCompThreshold, &QSlider::valueChanged, this, &CInputEffectLane::OnCompThresholdSliderChanged );
    QObject::connect ( grbReverb, &QGroupBox::toggled, this, &CInputEffectLane::OnReverbToggled );
    QObject::connect ( sldReverb, &QSlider::valueChanged, this, &CInputEffectLane::OnReverbSliderChanged );
}

void CInputEffectLane::SetClientAndLane ( CClient* pNCliP, const int iNLane )
{
    pClient = pNCliP;
    iLane   = iNLane;
    UpdateFromClient();
}

void CInputEffectLane::UpdateFromClient()
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient == nullptr )
    {
        return;
    }

    CInputEffectChain& Fx = pClient->GetInputEffects();

    // do not fire signals since this is an update from the effect parameters
    grbBoost->blockSignals ( true );
    sldBoost->blockSignals ( true );
    grbComp->blockSignals ( true );
    cbxCompAmount->blockSignals ( true );
    sldCompThreshold->blockSignals ( true );
    grbReverb->blockSignals ( true );
    sldReverb->blockSignals ( true );

    grbBoost->setChecked ( Fx.Boost ( iLane ).IsEnabled() );
    sldBoost->setValue ( qRound ( Fx.Boost ( iLane ).GetGainDb() ) );
    grbComp->setChecked ( Fx.Compressor ( iLane ).IsEnabled() );
    cbxCompAmount->setCurrentIndex ( Fx.Compressor ( iLane ).GetRatioIdx() );
    sldCompThreshold->setValue ( qRound ( Fx.Compressor ( iLane ).GetThresholdDb() ) );
    grbReverb->setChecked ( Fx.Reverb ( iLane ).IsEnabled() );
    sldReverb->setValue ( Fx.Reverb ( iLane ).GetLevel() );

    grbBoost->blockSignals ( false );
    sldBoost->blockSignals ( false );
    grbComp->blockSignals ( false );
    cbxCompAmount->blockSignals ( false );
    sldCompThreshold->blockSignals ( false );
    grbReverb->blockSignals ( false );
    sldReverb->blockSignals ( false );

    UpdateValueLabels();
#endif
}

void CInputEffectLane::UpdateValueLabels()
{
    lblBoostValue->setText ( tr ( "+%1 dB" ).arg ( sldBoost->value() ) );
    lblCompThresholdValue->setText ( tr ( "%1 dB" ).arg ( sldCompThreshold->value() ) );
    lblReverbValue->setText ( tr ( "%1 %" ).arg ( sldReverb->value() ) );
}

void CInputEffectLane::UpdateMeters ( const double dRawDb, const double dOutDb )
{
    lbrInputRaw->SetValue ( dRawDb );
    lbrOutput->SetValue ( dOutDb );
}

void CInputEffectLane::ResetMeters()
{
    lbrInputRaw->SetValue ( 0 );
    lbrOutput->SetValue ( 0 );
}

void CInputEffectLane::SetMeterStyle ( const EMeterStyle eNewMeterStyle )
{
    // same mapping as the input meter on the main window
    switch ( eNewMeterStyle )
    {
    case MT_LED_STRIPE:
        lbrInputRaw->SetLevelMeterType ( CLevelMeter::MT_LED_STRIPE );
        lbrOutput->SetLevelMeterType ( CLevelMeter::MT_LED_STRIPE );
        break;

    case MT_LED_ROUND_BIG:
    case MT_LED_ROUND_SMALL:
        lbrInputRaw->SetLevelMeterType ( CLevelMeter::MT_LED_ROUND_BIG );
        lbrOutput->SetLevelMeterType ( CLevelMeter::MT_LED_ROUND_BIG );
        break;

    case MT_BAR_WIDE:
    case MT_BAR_NARROW:
        lbrInputRaw->SetLevelMeterType ( CLevelMeter::MT_BAR_WIDE );
        lbrOutput->SetLevelMeterType ( CLevelMeter::MT_BAR_WIDE );
        break;
    }
}

void CInputEffectLane::OnBoostToggled ( bool bChecked )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Boost ( iLane ).SetEnabled ( bChecked );
        emit ParamChanged();
    }
#else
    Q_UNUSED ( bChecked )
#endif
}

void CInputEffectLane::OnBoostSliderChanged ( int iValue )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Boost ( iLane ).SetGainDb ( static_cast<float> ( iValue ) );
        UpdateValueLabels();
        emit ParamChanged();
    }
#else
    Q_UNUSED ( iValue )
#endif
}

void CInputEffectLane::OnCompToggled ( bool bChecked )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Compressor ( iLane ).SetEnabled ( bChecked );
        emit ParamChanged();
    }
#else
    Q_UNUSED ( bChecked )
#endif
}

void CInputEffectLane::OnCompAmountActivated ( int iIdx )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Compressor ( iLane ).SetRatioIdx ( iIdx );
        emit ParamChanged();
    }
#else
    Q_UNUSED ( iIdx )
#endif
}

void CInputEffectLane::OnCompThresholdSliderChanged ( int iValue )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Compressor ( iLane ).SetThresholdDb ( static_cast<float> ( iValue ) );
        UpdateValueLabels();
        emit ParamChanged();
    }
#else
    Q_UNUSED ( iValue )
#endif
}

void CInputEffectLane::OnReverbToggled ( bool bChecked )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Reverb ( iLane ).SetEnabled ( bChecked );
        emit ParamChanged();
    }
#else
    Q_UNUSED ( bChecked )
#endif
}

void CInputEffectLane::OnReverbSliderChanged ( int iValue )
{
#ifndef NO_INTERNAL_EFFECTS
    if ( pClient != nullptr )
    {
        pClient->GetInputEffects().Reverb ( iLane ).SetLevel ( iValue );
        UpdateValueLabels();
        emit ParamChanged();
    }
#else
    Q_UNUSED ( iValue )
#endif
}
