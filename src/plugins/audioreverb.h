/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
    The following code calls MVerb for reverberation.
    MVerb was written by Martin Eastwood.
    https://github.com/martineastwood/mverb
*/

#pragma once
#include "util.h"
#include "libs/mverb/MVerb.h"

class CAudioReverb
{
public:
    CAudioReverb()
    {
        fMaxShort = static_cast<float> ( _MAXSHORT );
        iPreset   = STADIUM;

        // Create MVerb on the heap
        mverb = std::unique_ptr<MVerb<float>> ( new MVerb<float>() );
        mverb->setSampleRate ( SYSTEM_SAMPLE_RATE_HZ );
        loadPreset();
    }

    void Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam );

    void Clear();
    void Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fReverbGain );
    void setPreset ( const int iNPreset )
    {
        // silently fail if preset doesn't exist
        if ( MathUtils::InRange<int> ( iNPreset, SUBTLE, HALVES ) )
        {
            iPreset = iNPreset;
            loadPreset();
        }
    };
    int getPreset() const { return iPreset; };

protected:
    std::unique_ptr<MVerb<float>> mverb;

    void         loadPreset();
    EAudChanConf eAudioChannelConf;
    int          iStereoBlockSizeSam;
    float        fMaxShort;
    int          iPreset;

    int                numFrames;
    std::vector<float> bufL;
    std::vector<float> bufR;

    enum
    {
        SUBTLE          = 0,
        STADIUM         = 1,
        CUPBOARD        = 2,
        DARK            = 3,
        HALVES          = 4,
        NUM_REV_PRESETS = 5
    };

    constexpr static inline float const presets[NUM_REV_PRESETS][MVerb<float>::NUM_PARAMS] = { { .5, .5, .5, .5, .5, 1., 1., .5, .75 },
                                                                                               { 0., .5, 1., .5, 0., 1., 1., .35, .75 },
                                                                                               { 0., .5, 1., .5, 0., 1., 1., 1., .75 },
                                                                                               { .9, .5, .1, .5, 0., 1., 1., 1., .75 },
                                                                                               { .5, .5, .5, .5, .5, 1., 1., .5, .75 }

    };
};
