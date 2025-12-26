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
    
    // Initialize channel names to avoid crashes when GUI queries them
    for (int i = 0; i < MAX_NUM_IN_OUT_CHANNELS; ++i) {
        channelInputName[i] = QString("Input %1").arg(i + 1);
        channelInfosOutput[i].name[0] = '\0';  // Empty name
    }
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

// Qt meta-object: delegate to CSoundBase (the parent class) which is a proper
// Q_OBJECT. CSound itself doesn't have Q_OBJECT so it inherits the meta-object
// from CSoundBase. We must NOT return nullptr as that breaks Qt signal/slot.
const QMetaObject* CSound::metaObject() const { return &CSoundBase::staticMetaObject; }
void* CSound::qt_metacast(const char* clname) { return CSoundBase::qt_metacast(clname); }
int CSound::qt_metacall(QMetaObject::Call c, int id, void** a) { return CSoundBase::qt_metacall(c, id, a); }

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
