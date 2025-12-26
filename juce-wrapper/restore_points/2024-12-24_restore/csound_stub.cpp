#include "../../../src/sound/asio/sound.h"

// Minimal CSound implementation used when building the wrapper/plugin.
// This avoids pulling in real ASIO driver code while satisfying linkage
// for `CClient` construction. It's a non-functional stub for audio I/O.

CSound::CSound(void (*fpNewCallback)(CVector<int16_t>&, void*), void* arg, const QString& strMIDISetup, const bool, const QString&)
    : CSoundBase("stub", fpNewCallback, arg, strMIDISetup)
{
    bRun = false;
    lNumInChan = NUM_IN_OUT_CHANNELS;
    lNumOutChan = NUM_IN_OUT_CHANNELS;
    lNumInChanPlusAddChan = NUM_IN_OUT_CHANNELS;
    for (int i = 0; i < MAX_NUMBER_SOUND_CARDS; ++i)
        cDriverNames[i] = nullptr;
}

CSound::~CSound()
{
    if (IsRunning())
        Stop();
}

int CSound::Init(const int iNewPrefMonoBufferSize)
{
    // Return the requested buffer size (or a sensible default) to satisfy CClient::Init()
    // which expects this to return the actual buffer size achieved
    return iNewPrefMonoBufferSize > 0 ? iNewPrefMonoBufferSize : 128;
}

void CSound::Start() { bRun = true; bCallbackEntered = true; }
void CSound::Stop() { bRun = false; }

QString CSound::LoadAndInitializeDriver(QString /*strDriverName*/, bool /*bOpenDriverSetup*/)
{
    return QString();
}

void CSound::UnloadCurrentDriver() { }

QString CSound::CheckDeviceCapabilities() { return QString(); }

bool CSound::CheckSampleTypeSupported(const ASIOSampleType /*SamType*/) { return true; }
bool CSound::CheckSampleTypeSupportedForCHMixing(const ASIOSampleType /*SamType*/) { return true; }

int CSound::GetActualBufferSize(const int iDesiredBufferSizeMono) { return iDesiredBufferSizeMono; }

void CSound::ResetChannelMapping()
{
    vSelectedInputChannels.clear();
    vSelectedOutputChannels.clear();
    vSelectedInputChannels.push_back(0);
    vSelectedInputChannels.push_back(1);
    vSelectedOutputChannels.push_back(0);
    vSelectedOutputChannels.push_back(1);
}

// (No additional overrides here.)

// Qt meta-object fallback implementations to satisfy linkage when moc
// generated helpers are not emitted for this override. Returning nullptr
// is safe for our headless stub usage.
const QMetaObject* CSound::metaObject() const { return nullptr; }
void* CSound::qt_metacast(const char*) { return nullptr; }
int CSound::qt_metacall(QMetaObject::Call, int, void**) { return -1; }

// Channel selection setters (no-op for stub)
void CSound::SetLeftInputChannel(const int) {}
void CSound::SetRightInputChannel(const int) {}
void CSound::SetLeftOutputChannel(const int) {}
void CSound::SetRightOutputChannel(const int) {}

// Minimal static callbacks (not used by stub)
void CSound::bufferSwitch(long, ASIOBool) {}
ASIOTime* CSound::bufferSwitchTimeInfo(ASIOTime* /*timeInfo*/, long /*index*/, ASIOBool /*processNow*/) { return nullptr; }
long CSound::asioMessages(long, long, void*, double*) { return 0; }

// Provide a simple ASIOControlPanel definition inside the jamulus_core stub
// so that headless/plugin builds don't require the ASIO SDK or external
// control-panel symbols at final link time.
ASIOError ASIOControlPanel(void) { return -1000; }
