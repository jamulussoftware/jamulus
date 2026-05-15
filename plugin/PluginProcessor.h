#pragma once

#pragma once

#include "../jamulus_core_juce.h"
#include "../audio_fifo.h"
#include "../virtual_sound/virtual_sound.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Resampler.h"
#include "AudioDelay.h"
#include "AudioReverb.h"
#include <array>
#include <atomic>
#include <functional>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class JamulusPluginProcessor
{
public:
    struct MidiLearnEvent
    {
        int   channel    = 1;
        int   controller = 0;
        float normalizedValue = 0.0f;
    };

    static constexpr int kAuxStereoBusCount = 15;

    struct RecordingSettings
    {
        enum Mode
        {
            recordingOff = 0,
            stereoWav,
            stereoOgg,
            stereoMp3,
            multitrackMogg
        };

        int          mode = recordingOff;
        juce::String folderPath;
        bool         stereoUseLiveAutoLevel = true;
        bool         stereoApplyLimiter     = true;
        bool         stemsUseLiveAutoLevel  = false;
    };

    JamulusPluginProcessor();
    ~JamulusPluginProcessor();

    void prepareToPlay ( double sampleRate, int samplesPerBlock );
    void processBlock ( const float** inputs, float** outputs, int numChannels, int numSamples );
    void captureMidiForLearn ( const juce::MidiBuffer& midiMessages );
    bool popNextMidiLearnEvent ( MidiLearnEvent& event );
    void enqueueMidiOutputCc ( int channel, int controller, int value );
    void drainMidiOutput ( juce::MidiBuffer& midiMessages );
    void releaseResources();
    void resetAudioState();

    // Reset callback for GUI to trigger processor-level resets
    std::function<void()> onResetRequested;

    bool isUsingNonQtCore() const;
    void setUseNonQtCore ( bool enabled );

    void connectToServer ( const std::string& addr );
    void disconnectFromServer();
    bool isConnected() const;
    jamulus_client_t getClient();
    void setCurrentServerName ( const std::string& name ) { currentServerName = name; }
    std::string getCurrentServerName() const { return currentServerName; }
    std::string getCurrentServerAddress() const { return currentServerAddress; }

    void setTestToneEnabled ( bool enabled )
    {
        testToneEnabled = enabled;
        phase           = 0.0;
    }
    bool isTestToneEnabled() const { return testToneEnabled; }

    void setMonitorMode ( bool enabled ) { monitorMode = enabled; }
    bool isMonitorMode() const { return monitorMode; }

    void  setMainVolume ( float vol ) { mainVolume = vol; }
    float getMainVolume() const { return mainVolume; }
    void  setChannelReferenceGain ( int channelId, float gain );
    RecordingSettings getRecordingSettings() const;
    void setRecordingSettings ( const RecordingSettings& settings );
    bool startRecording();
    void stopRecording();
    bool isRecordingActive() const;

private:
    struct ChannelControlState
    {
        float gain  = 1.0f;
        float pan   = 0.0f;
        bool  muted = false;
        bool  solo  = false;
        int   route = 0;
    };

    struct HiddenRouteClient
    {
        jamulus_core_t core      = nullptr;
        int            routeCode = 0;
        bool           needsResampling = false;
        LinearResampler inputResampler;
        LinearResampler outputResampler;
    };

    jamulus_core_t      core          = nullptr;
    AudioFifo*          inputFifo       = nullptr;
    AudioFifo*          outputFifo      = nullptr;
    VirtualSoundBackend vsBackend;
    int                 procNumChannels = 2;
    int                 procBlockSize   = 128;
    double              hostSampleRate  = 48000.0;

    // Sample rate conversion
    bool            needsResampling = false;
    LinearResampler inputResampler;  // Host rate -> 48kHz
    LinearResampler outputResampler; // 48kHz -> Host rate

    // Test tone state
    bool   testToneEnabled = false;
    bool   monitorMode     = false;
    double phase           = 0.0;
    double testToneFreq    = 440.0;
    double testToneAmp     = 0.2;
    float  mainVolume      = 1.0f; // Main output volume (0.0 - 1.0)

    // Reverb Control Methods
public:
    void setReverbEnabled ( bool enabled ) { reverbEnabled = enabled; }
    void setReverbMix ( float mix ) { audioReverb.setMix ( mix ); }
    void setReverbDecay ( float seconds ) { audioReverb.setDecay ( seconds ); }
    void setReverbDamping ( float damping ) { audioReverb.setDamping ( damping ); }

private:
    // Internal Reverb
    bool        reverbEnabled = false;
    AudioReverb audioReverb;

    // Delay Control Methods
public:
    void setDelayEnabled ( bool enabled ) { delayEnabled = enabled; }
    void setDelayMix ( float mix ) { audioDelay.setMix ( mix ); }
    void setDelayTime ( float ms ) { audioDelay.setDelayTime ( ms ); }
    void setDelayFeedback ( float fb ) { audioDelay.setFeedback ( fb ); }
    void setDelayPingPong ( bool pp ) { audioDelay.setPingPong ( pp ); }
    void setDelayHighPass ( float freq ) { audioDelay.setHighPassFreq ( freq ); }

private:
    // Internal Delay
    bool       delayEnabled = false;
    AudioDelay audioDelay;

    // Limiter Control Methods
public:
    void  setLimiterEnabled ( bool enabled ) { limiterEnabled = enabled; }
    bool  isLimiterEnabled() const { return limiterEnabled; }
    void  setLimiterThreshold ( float db ) { limiterThresholdDb = db; }
    float getLimiterThreshold() const { return limiterThresholdDb; }
    void  setLimiterRelease ( float ms ) { limiterReleaseMs = ms; }
    float getLimiterRelease() const { return limiterReleaseMs; }

private:
    // Internal Limiter
    bool  limiterEnabled   = false;
    float limiterThresholdDb = 0.0f;
    float limiterReleaseMs   = 120.0f;
    float limiterEnvelope    = 1.0f;

    float lastInputPeakLeft  = 0.0f;
    float lastInputPeakRight = 0.0f;
    float lastOutputPeakLeft  = 0.0f;
    float lastOutputPeakRight = 0.0f;
    mutable std::mutex routeStateMutex;
    std::map<int, ChannelControlState> channelControlStates;
    std::map<int, HiddenRouteClient>   hiddenRouteClients;
    std::array<std::vector<float>, kAuxStereoBusCount> auxBusBuffers;
    mutable std::mutex midiLearnMutex;
    std::deque<MidiLearnEvent> pendingMidiLearnEvents;
    mutable std::mutex midiOutputMutex;
    std::deque<juce::MidiMessage> pendingMidiOutputMessages;
    mutable std::mutex recordingMutex;
    std::string currentServerAddress;
    std::string currentServerName;
    std::string profileName = "JamulusPlus";
    std::string profileCity;
    int         profileInstrument   = 0;
    int         profileCountry      = 0;
    int         profileSkill        = 0;
    bool        autoJitterEnabled   = true;
    int         jitterBufferSize    = 10;
    int         serverJitterBufferSize = 10;
    bool        smallBuffersEnabled = false;
    int         audioChannelsMode   = 1;
    std::map<int, float> channelReferenceGains;
    RecordingSettings    recordingSettings;

    struct RecordingRuntime
    {
        bool active = false;
        std::unique_ptr<juce::TimeSliceThread> writerThread;
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
        std::vector<int> trackedChannelIds;
        std::vector<juce::String> trackedChannelNames;
        juce::File targetFile;
        juce::File sidecarFile;
    } recordingRuntime;

public:
    float getLastInputPeakLeft() const { return lastInputPeakLeft; }
    float getLastInputPeakRight() const { return lastInputPeakRight; }
    float getLastOutputPeakLeft() const { return lastOutputPeakLeft; }
    float getLastOutputPeakRight() const { return lastOutputPeakRight; }

    void setInputFader ( int value );
    int  getInputFader() const { return inputFaderValue; }

    void sendPing();
    bool getNetworkStats ( jamulus_network_stats_t& stats ) const;
    bool getNextChatMessage ( std::string& message );
    void setAutoTranslateEnabled ( bool enabled );
    bool isAutoTranslateEnabled() const { return autoTranslateEnabled; }
    void setTranslateTargetLang ( const juce::String& langCode );
    juce::String getTranslateTargetLang() const { return translateTargetLang; }

    void setChannelGain ( int channelId, float gain );
    void setChannelPan ( int channelId, float pan );
    void setChannelMute ( int channelId, bool muted );
    void setChannelSolo ( int channelId, bool solo );
    void setChannelOutputRoute ( int channelId, int routeCode );
    int  getChannelOutputRoute ( int channelId ) const;
    void copyAuxBusBuffer ( int busIndex, float** outputs, int numChannels, int numSamples ) const;

    int  getNumChannels() const;
    int  getMyChannelId() const;
    bool getChannelInfo ( int index, jamulus_channel_info_t& info ) const;

    void sendChatMessage ( const std::string& message );

    std::string getProfileName() const;
    void        setProfileName ( const std::string& name );

    int  getProfileInstrument() const;
    void setProfileInstrument ( int instrument );

    int  getProfileCountry() const;
    void setProfileCountry ( int country );

    std::string getProfileCountryCode() const;

    std::string getProfileCity() const;
    void        setProfileCity ( const std::string& city );

    int  getProfileSkill() const;
    void setProfileSkill ( int skill );

    int  getNumServers() const;
    bool getServerInfoAt ( int index, jamulus_server_info_t& info ) const;
    void clearServerList();
    void requestServerList ( int directoryType, const std::string& customAddress );
    void requestServerClientList ( const std::string& address );
    void pingServerList();

    bool getAutoJitter() const;
    void setAutoJitter ( bool enabled );
    int  getJitterBuffer() const;
    void setJitterBuffer ( int size );
    int  getServerJitterBuffer() const;
    void setServerJitterBuffer ( int size );
    bool getSmallBuffers() const;
    void setSmallBuffers ( bool enabled );

private:
    int   inputFaderValue = 50;
    float inputFaderGain  = 1.0f;
    bool         autoTranslateEnabled = false;
    juce::String translateTargetLang  = "system";

    void applyBackendSwitchIfRequested();
    void destroyHiddenRouteClients();
    void syncMainCoreChannelState ( int channelId );
    void syncHiddenRouteClient ( int channelId );
    void syncAllHiddenRouteClients();
    void syncSettingsToCore ( jamulus_core_t targetCore ) const;
    void clearAuxBusBuffers ( int numSamples );
    bool shouldMaintainHiddenClientForChannel ( int channelId ) const;
};
