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
    CAudioReverb();

    void Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam );

    void Clear();
    void Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fReverbGain );
    void setPreset ( const int iNPreset );
    int  getPreset() const { return iPreset; };

protected:
    std::unique_ptr<MVerb<float>> mverb;

    void         loadPreset();
    EAudChanConf eAudioChannelConf;
    int          iStereoBlockSizeSam;
    float        fMaxShort;
    int          iPreset;
    bool         bPresetChangeQueued;

    int                numFrames;
    std::vector<float> bufL;
    std::vector<float> bufR;

    // Parameters are set iteratively by enum. See MVerb.h for reference.
    // NOTE: parameters "GAIN" and "MIX" must be "1."
    constexpr static inline float const presets[RP_NUM_REV_PRESETS][MVerb<float>::NUM_PARAMS] = { { 0., .5, 1., .5, 0., .5, 1., 1., .75 },
                                                                                                  { 0., .5, 1., .5, 0., 1., 1., 1., .75 },
                                                                                                  { 0., .5, 1., .5, 0., .25, 1., 1., .75 },
                                                                                                  { .9, .5, .1, .5, 0., .5, 1., 1., .75 },
                                                                                                  { .5, .5, .5, .5, .5, .75, 1., 1., .5 },
                                                                                                  { .2, .4, .4, .6, .1, .05, 1., 1., .4 },
                                                                                                  { .4, .2, .3, .6, .2, .2, 1., 1., .5 } };
};
