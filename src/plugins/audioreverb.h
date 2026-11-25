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
#include "inputeffect.h"

/* Definitions ****************************************************************/
#define AUD_REVERB_MAX     100
#define AUD_REVERB_DEFAULT 30

/* Classes ********************************************************************/
class CAudioReverb : public CInputEffect
{
public:
    CAudioReverb() : iNumChannels ( 1 ), iLevel ( AUD_REVERB_DEFAULT ) {}

    // reverberation level in the range 0 .. AUD_REVERB_MAX
    void SetLevel ( const int iNewLevel );
    int  GetLevel() const { return iLevel; }

    // the delay lines are sized by the decay time, iMaxFrames is not needed
    virtual void Activate ( const int iNNumChannels, const int /* iMaxFrames */ ) override;
    virtual void Reset() override;
    virtual void Process ( float* const* ppfChannels, const int iNumFrames ) override;

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

    int              iNumChannels;
    std::atomic<int> iLevel;
    CFIFO<float>     allpassDelays[3];
    CFIFO<float> combDelays[4];
    COnePole     combFilters[4];
    CFIFO<float> outLeftDelay;
    CFIFO<float> outRightDelay;
    float        allpassCoefficient;
    float        combCoefficient[4];
};
