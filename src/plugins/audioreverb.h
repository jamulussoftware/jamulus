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

#pragma once
#include "util.h"

class CAudioReverb
{
public:
    CAudioReverb() {}

    void Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam, const int iSampleRate, const float fT60 = 1.1f );

    void Clear();
    void Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fAttenuation );

protected:
    void setT60 ( const float fT60, const int iSampleRate );
    bool isPrime ( const int number );

    class COnePole
    {
    public:
        COnePole() : fA ( 0 ), fB ( 0 ) { Reset(); }
        void  setPole ( const float fPole );
        float Calc ( const float fIn );
        void  Reset() { fLastSample = 0; }

    protected:
        float fA;
        float fB;
        float fLastSample;
    };

    EAudChanConf eAudioChannelConf;
    int          iStereoBlockSizeSam;
    CFIFO<float> allpassDelays[3];
    CFIFO<float> combDelays[4];
    COnePole     combFilters[4];
    CFIFO<float> outLeftDelay;
    CFIFO<float> outRightDelay;
    float        allpassCoefficient;
    float        combCoefficient[4];
};
