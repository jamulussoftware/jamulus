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

void CAudioReverb::SetLevel ( const int iNewLevel )
{
    if ( ( iNewLevel >= 0 ) && ( iNewLevel <= AUD_REVERB_MAX ) )
    {
        iLevel = iNewLevel;
    }
}

void CAudioReverb::Activate ( const int iNNumChannels, const int /* iMaxFrames */ )
{
    // store parameters
    iNumChannels = iNNumChannels;

    const int   iSampleRate = SYSTEM_SAMPLE_RATE_HZ;
    const float fT60        = 1.1f; // reverberation decay time in seconds

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
    Reset();
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

void CAudioReverb::Reset()
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

void CAudioReverb::Process ( float* const* ppfChannels, const int iNumFrames )
{
    // the level is read once per block; 0 means no reverberation
    const int iCurLevel = iLevel;

    if ( iCurLevel == 0 )
    {
        return;
    }

    const float fAttenuation = static_cast<float> ( iCurLevel ) / AUD_REVERB_MAX / 4;

    float fMixedInput, temp, temp0, temp1, temp2;

    for ( int i = 0; i < iNumFrames; i++ )
    {
        // we sum up the channels of the lane (both channels in stereo mode,
        // only the own channel in the mono modes)
        if ( iNumChannels == 2 )
        {
            fMixedInput = 0.5f * ( ppfChannels[0][i] + ppfChannels[1][i] );
        }
        else
        {
            fMixedInput = ppfChannels[0][i];
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

        // inplace apply the attenuated reverb signal to all channels of the lane
        ppfChannels[0][i] = ( 1.0f - fAttenuation ) * ppfChannels[0][i] + 0.5f * fAttenuation * outLeftDelay.Get();

        if ( iNumChannels == 2 )
        {
            ppfChannels[1][i] = ( 1.0f - fAttenuation ) * ppfChannels[1][i] + 0.5f * fAttenuation * outRightDelay.Get();
        }
        else
        {
            // keep the second decorrelation delay line running
            outRightDelay.Get();
        }
    }
}