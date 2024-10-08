/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
    The following code is based on "JCRev: John Chowning's reverberator class"
    by Perry R. Cook and Gary P. Scavone, 1995 - 2004
    which is in "The Synthesis ToolKit in C++ (STK)"
    http://ccrma.stanford.edu/software/stk

    Original description:
    This class is derived from the CLM JCRev function, which is based on the use
    of networks of simple allpass and comb delay filters. This class implements
    three series allpass units, followed by four parallel comb filters, and two
    decorrelation delay lines in parallel at the output.
*/

#include "audioreverb.h"

void CAudioReverb::Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam, const int iSampleRate, const float fT60 )
{
    // store parameters
    eAudioChannelConf   = eNAudioChannelConf;
    iStereoBlockSizeSam = iNStereoBlockSizeSam;

    // delay lengths for 44100 Hz sample rate
    int         lengths[9] = { 1116, 1356, 1422, 1617, 225, 341, 441, 211, 179 };
    const float scaler     = static_cast<float> ( iSampleRate ) / 44100.0f;

    if ( scaler != 1.0f )
    {
        for ( int i = 0; i < 9; i++ )
        {
            int delay = static_cast<int> ( floorf ( scaler * lengths[i] ) );

            if ( ( delay & 1 ) == 0 )
            {
                delay++;
            }

            while ( !isPrime ( delay ) )
            {
                delay += 2;
            }

            lengths[i] = delay;
        }
    }

    for ( int i = 0; i < 3; i++ )
    {
        allpassDelays[i].Init ( lengths[i + 4] );
    }

    for ( int i = 0; i < 4; i++ )
    {
        combDelays[i].Init ( lengths[i] );
        combFilters[i].setPole ( 0.2f );
    }

    setT60 ( fT60, iSampleRate );
    outLeftDelay.Init ( lengths[7] );
    outRightDelay.Init ( lengths[8] );
    allpassCoefficient = 0.7f;
    Clear();
}

bool CAudioReverb::isPrime ( const int number )
{
    /*
        Returns true if argument value is prime. Taken from "class Effect" in
        "STK abstract effects parent class".
    */
    if ( number == 2 )
    {
        return true;
    }

    if ( number & 1 )
    {
        for ( int i = 3; i < static_cast<int> ( sqrtf ( static_cast<float> ( number ) ) ) + 1; i += 2 )
        {
            if ( ( number % i ) == 0 )
            {
                return false;
            }
        }

        return true; // prime
    }
    else
    {
        return false; // even
    }
}

void CAudioReverb::Clear()
{
    // reset and clear all internal state
    allpassDelays[0].Reset ( 0 );
    allpassDelays[1].Reset ( 0 );
    allpassDelays[2].Reset ( 0 );
    combDelays[0].Reset ( 0 );
    combDelays[1].Reset ( 0 );
    combDelays[2].Reset ( 0 );
    combDelays[3].Reset ( 0 );
    combFilters[0].Reset();
    combFilters[1].Reset();
    combFilters[2].Reset();
    combFilters[3].Reset();
    outRightDelay.Reset ( 0 );
    outLeftDelay.Reset ( 0 );
}

void CAudioReverb::setT60 ( const float fT60, const int iSampleRate )
{
    // set the reverberation T60 decay time
    for ( int i = 0; i < 4; i++ )
    {
        combCoefficient[i] = powf ( 10.0f, static_cast<float> ( -3.0f * combDelays[i].Size() / ( fT60 * iSampleRate ) ) );
    }
}

void CAudioReverb::COnePole::setPole ( const float fPole )
{
    // calculate IIR filter coefficients based on the pole value
    fA = -fPole;
    fB = 1.0f - fPole;
}

float CAudioReverb::COnePole::Calc ( const float fIn )
{
    // calculate IIR filter
    fLastSample = fB * fIn - fA * fLastSample;

    return fLastSample;
}

void CAudioReverb::Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fAttenuation )
{
    float fMixedInput, temp, temp0, temp1, temp2;

    for ( int i = 0; i < iStereoBlockSizeSam; i += 2 )
    {
        // we sum up the stereo input channels (in case mono input is used, a zero
        // shall be input for the right channel)
        if ( eAudioChannelConf == CC_STEREO )
        {
            fMixedInput = 0.5f * ( vecsStereoInOut[i] + vecsStereoInOut[i + 1] );
        }
        else
        {
            if ( bReverbOnLeftChan )
            {
                fMixedInput = vecsStereoInOut[i];
            }
            else
            {
                fMixedInput = vecsStereoInOut[i + 1];
            }
        }

        temp  = allpassDelays[0].Get();
        temp0 = allpassCoefficient * temp;
        temp0 += fMixedInput;
        allpassDelays[0].Add ( temp0 );
        temp0 = -( allpassCoefficient * temp0 ) + temp;

        temp  = allpassDelays[1].Get();
        temp1 = allpassCoefficient * temp;
        temp1 += temp0;
        allpassDelays[1].Add ( temp1 );
        temp1 = -( allpassCoefficient * temp1 ) + temp;

        temp  = allpassDelays[2].Get();
        temp2 = allpassCoefficient * temp;
        temp2 += temp1;
        allpassDelays[2].Add ( temp2 );
        temp2 = -( allpassCoefficient * temp2 ) + temp;

        const float temp3 = temp2 + combFilters[0].Calc ( combCoefficient[0] * combDelays[0].Get() );
        const float temp4 = temp2 + combFilters[1].Calc ( combCoefficient[1] * combDelays[1].Get() );
        const float temp5 = temp2 + combFilters[2].Calc ( combCoefficient[2] * combDelays[2].Get() );
        const float temp6 = temp2 + combFilters[3].Calc ( combCoefficient[3] * combDelays[3].Get() );

        combDelays[0].Add ( temp3 );
        combDelays[1].Add ( temp4 );
        combDelays[2].Add ( temp5 );
        combDelays[3].Add ( temp6 );

        const float filtout = temp3 + temp4 + temp5 + temp6;

        outLeftDelay.Add ( filtout );
        outRightDelay.Add ( filtout );

        // inplace apply the attenuated reverb signal (for stereo always apply
        // reverberation effect on both channels)
        if ( ( eAudioChannelConf == CC_STEREO ) || bReverbOnLeftChan )
        {
            vecsStereoInOut[i] = Float2Short ( ( 1.0f - fAttenuation ) * vecsStereoInOut[i] + 0.5f * fAttenuation * outLeftDelay.Get() );
        }

        if ( ( eAudioChannelConf == CC_STEREO ) || !bReverbOnLeftChan )
        {
            vecsStereoInOut[i + 1] = Float2Short ( ( 1.0f - fAttenuation ) * vecsStereoInOut[i + 1] + 0.5f * fAttenuation * outRightDelay.Get() );
        }
    }
}