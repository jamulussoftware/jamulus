/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
    The following code calls MVerb for reverberation.
    MVerb was written by Martin Eastwood.
    https://github.com/martineastwood/mverb
*/

#include "audioreverb.h"

void CAudioReverb::Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam )
{
    eAudioChannelConf   = eNAudioChannelConf;
    iStereoBlockSizeSam = iNStereoBlockSizeSam;
    fMaxShort           = static_cast<float> ( _MAXSHORT );
    mverb.setParameter ( MVerb<float>::DAMPINGFREQ, 0.5 );
    mverb.setParameter ( MVerb<float>::DENSITY, 0.5 );
    mverb.setParameter ( MVerb<float>::BANDWIDTHFREQ, 0.5 );
    mverb.setParameter ( MVerb<float>::DECAY, 0.5 );
    mverb.setParameter ( MVerb<float>::PREDELAY, 0.5 );
    mverb.setParameter ( MVerb<float>::GAIN, 1. );
    mverb.setParameter ( MVerb<float>::MIX, 1.0 );
    mverb.setParameter ( MVerb<float>::EARLYMIX, 0.5 );
    mverb.setParameter ( MVerb<float>::SIZE, 0.75 );
}

void CAudioReverb::Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fReverbGain )
{
    // Jamulus uses interleaved stereo, mverb operates on a 2-dimensional array instead
    // Calculate the number of frames for each channel ( iStereoBlockSizeSam / 2 )
    const int numFrames = iStereoBlockSizeSam >> 1;

    // These buffers get filled with dry signal and are then passed to mverb
    // They need to be vectors as the windows builds fail when arrays are used
    std::vector<float> bufL ( numFrames );
    std::vector<float> bufR ( numFrames );

    // One buffer to pass to mverb's process function
    float* fInput[2] = { bufL.data(), bufR.data() };

    if ( eAudioChannelConf == CC_STEREO )
    {
        for ( int i = 0, j = 0; j < numFrames; i += 2, j++ )
        {
            // True stereo reverb
            bufL[j] = static_cast<float> ( vecsStereoInOut[i] ) / fMaxShort;
            bufR[j] = static_cast<float> ( vecsStereoInOut[i + 1] ) / fMaxShort;
        }
    }
    else
    {
        for ( int i = 0, j = 0; j < numFrames; i += 2, j++ )
        {
            // For Mono and Mono-in/Stereo-out only one channel is selected and copied into both fInput channels
            bufR[j] = bufL[j] = static_cast<float> ( vecsStereoInOut[i + !bReverbOnLeftChan] ) / fMaxShort;
        }
    }

    mverb.process ( fInput, fInput, numFrames );

    for ( int i = 0, j = 0; j < numFrames; i += 2, j++ )
    {
        // Mix wet and dry signal
        vecsStereoInOut[i]     = Float2Short ( vecsStereoInOut[i] + bufL[j] * fMaxShort * fReverbGain );
        vecsStereoInOut[i + 1] = Float2Short ( vecsStereoInOut[i + 1] + bufR[j] * fMaxShort * fReverbGain );
    }
}
