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
    CAudioReverb() {}

    void Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam );

    void Clear();
    void Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fReverbGain );

protected:
    MVerb<float> mverb;

    EAudChanConf eAudioChannelConf;
    int          iStereoBlockSizeSam;
    float        fMaxShort;
};
