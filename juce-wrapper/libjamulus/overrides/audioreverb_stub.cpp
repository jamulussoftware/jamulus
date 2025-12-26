#include "../../../src/plugins/audioreverb.h"

void CAudioReverb::Init(const EAudChanConf, const int, const int, const float)
{
    // no-op stub
}

void CAudioReverb::Clear()
{
    // no-op
}

void CAudioReverb::Process(CVector<int16_t>& vecsStereoInOut, const bool, const float)
{
    // no processing — leave buffer untouched
    (void)vecsStereoInOut;
}
