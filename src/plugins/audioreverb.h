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
        if ( MathUtils::InRange<int> ( iNPreset, 0, NUM_REV_PRESETS ) )
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
        SUBTLE = 0,
        STADIUM,
        CUPBOARD,
        DARK,
        HALVES,
        DRUMROOM,
        CLUB,
        NUM_REV_PRESETS
    };

    // Parameters are set iteratively by enum. See MVerb.h for reference.
    // NOTE: parameters "GAIN" and "MIX" must be "1."
    constexpr static inline float const presets[NUM_REV_PRESETS][MVerb<float>::NUM_PARAMS] = { { 0., .5, 1., .5, 0., .5, 1., 1., .75 },
                                                                                               { 0., .5, 1., .5, 0., 1., 1., 1., .75 },
                                                                                               { 0., .5, 1., .5, 0., .25, 1., 1., .75 },
                                                                                               { .9, .5, .1, .5, 0., .5, 1., 1., .75 },
                                                                                               { .5, .5, .5, .5, .5, .75, 1., 1., .5 },
                                                                                               { .2, .4, .4, .6, .1, .05, 1., 1., .4 },
                                                                                               { .4, .2, .3, .6, .2, .2, 1., 1., .5 }

    };
};
