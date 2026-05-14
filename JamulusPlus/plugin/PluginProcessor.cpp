#include "PluginProcessor.h"
#include "Resampler.h"
#include "DebugLogger.h"
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>

static constexpr double JAMULUS_SAMPLE_RATE = 48000.0;

namespace
{
juce::String makeSafeRecordingName ( const juce::String& text )
{
    auto lowered = text.toLowerCase().trim();
    juce::String cleaned;
    bool lastWasDash = false;

    for ( auto ch : lowered )
    {
        if ( juce::CharacterFunctions::isLetterOrDigit ( ch ) )
        {
            cleaned << ch;
            lastWasDash = false;
        }
        else if ( !lastWasDash )
        {
            cleaned << '-';
            lastWasDash = true;
        }
    }

    while ( cleaned.startsWithChar ( '-' ) )
        cleaned = cleaned.substring ( 1 );
    while ( cleaned.endsWithChar ( '-' ) )
        cleaned = cleaned.dropLastCharacters ( 1 );

    return cleaned.isNotEmpty() ? cleaned : "jamulus-session";
}

juce::File getDefaultRecordingFolder()
{
    auto dir = juce::File::getSpecialLocation ( juce::File::userDocumentsDirectory )
                   .getChildFile ( "JamulusPlus Recordings" );
    if ( !dir.exists() )
        dir.createDirectory();
    return dir;
}

void applyGainToInterleaved ( std::vector<float>& samples, float gain )
{
    if ( gain == 1.0f )
        return;

    for ( auto& sample : samples )
        sample *= gain;
}

void clampSoftLimitInterleaved ( std::vector<float>& samples )
{
    constexpr float limiterCeiling    = 0.9885531f; // -0.1 dBFS
    constexpr float softClipThreshold = 0.92f;

    for ( auto& sample : samples )
    {
        const float sign  = ( sample < 0.0f ) ? -1.0f : 1.0f;
        const float absIn = std::abs ( sample );

        if ( absIn <= softClipThreshold )
            continue;

        float absOut = limiterCeiling;
        if ( absIn < limiterCeiling )
        {
            const float t      = ( absIn - softClipThreshold ) / ( limiterCeiling - softClipThreshold );
            const float shaped = t * t * ( 3.0f - 2.0f * t );
            absOut             = softClipThreshold + ( limiterCeiling - softClipThreshold ) * shaped;
        }

        sample = sign * std::min ( absOut, limiterCeiling );
    }
}

struct OutputRouteDescriptor
{
    bool isMasterStereo = true;
    bool renderToMaster = false;
    bool renderToAux    = false;
    int  auxBusIndex    = -1;
    int  channelMode    = 2; // 0 = left only, 1 = right only, 2 = stereo
};

OutputRouteDescriptor decodeOutputRoute ( int routeCode )
{
    if ( routeCode <= 0 )
        return {};

    if ( routeCode == 1 )
        return { false, true, false, -1, 0 };

    if ( routeCode == 2 )
        return { false, true, false, -1, 1 };

    const int auxIndex = ( routeCode - 3 ) / 3;
    const int mode     = ( routeCode - 3 ) % 3;
    if ( auxIndex < 0 || auxIndex >= JamulusPluginProcessor::kAuxStereoBusCount )
        return {};

    return { false, false, true, auxIndex, mode == 0 ? 2 : ( mode == 1 ? 0 : 1 ) };
}

void writeInterleavedToBus ( const std::vector<float>& interleaved, float** outputs, int numChannels, int numSamples, int channelMode )
{
    if ( outputs == nullptr || numChannels <= 0 )
        return;

    for ( int sample = 0; sample < numSamples; ++sample )
    {
        const float left  = interleaved[static_cast<size_t> ( sample ) * 2 + 0];
        const float right = interleaved[static_cast<size_t> ( sample ) * 2 + 1];

        if ( channelMode == 2 )
        {
            if ( numChannels > 0 && outputs[0] )
                outputs[0][sample] = left;
            if ( numChannels > 1 && outputs[1] )
                outputs[1][sample] = right;
        }
        else if ( channelMode == 0 )
        {
            const float mono = 0.5f * ( left + right );
            if ( numChannels > 0 && outputs[0] )
                outputs[0][sample] = mono;
            if ( numChannels > 1 && outputs[1] )
                outputs[1][sample] = 0.0f;
        }
        else
        {
            const float mono = 0.5f * ( left + right );
            if ( numChannels > 0 && outputs[0] )
                outputs[0][sample] = 0.0f;
            if ( numChannels > 1 && outputs[1] )
                outputs[1][sample] = mono;
        }
    }
}

void mixStemIntoMain ( const std::vector<float>& stem, std::vector<float>& mainOut, int channelMode )
{
    const size_t totalSamples = std::min ( stem.size(), mainOut.size() );
    if ( channelMode == 2 )
    {
        for ( size_t i = 0; i < totalSamples; ++i )
            mainOut[i] += stem[i];
        return;
    }

    for ( size_t frame = 0; frame + 1 < totalSamples; frame += 2 )
    {
        const float mono = 0.5f * ( stem[frame] + stem[frame + 1] );
        if ( channelMode == 0 )
            mainOut[frame] += mono;
        else
            mainOut[frame + 1] += mono;
    }
}
}

JamulusPluginProcessor::JamulusPluginProcessor()
{
    DebugLogger::instance().log ( "[JamulusPluginProcessor] Constructor called" );
}

JamulusPluginProcessor::~JamulusPluginProcessor()
{
    DebugLogger::instance().log ( "[JamulusPluginProcessor] Destructor called" );
    releaseResources();
    destroyHiddenRouteClients();
    if ( core )
    {
        jamulus_core_stop ( core );
        jamulus_core_destroy ( core );
        core = nullptr;
    }
}

bool JamulusPluginProcessor::isUsingNonQtCore() const
{
#if defined( JAMULUS_NONQT_CORE )
    return true;
#else
    return false;
#endif
}

void JamulusPluginProcessor::setUseNonQtCore ( bool )
{
}

void JamulusPluginProcessor::applyBackendSwitchIfRequested()
{
}

void JamulusPluginProcessor::connectToServer ( const std::string& addr )
{
    currentServerAddress = addr;

    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating core in connectToServer" );
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( !core )
        {
            DebugLogger::instance().log ( "[JamulusPluginProcessor] ERROR: jamulus_core_create returned nullptr in connectToServer" );
            return;
        }
        syncSettingsToCore ( core );
    }

    if ( jamulus_core_is_running ( core ) != 0 )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Stopping running core before connectToServer restart" );
        jamulus_core_stop ( core );
    }

    DebugLogger::instance().log ( "[JamulusPluginProcessor] Setting server address to: " + addr );
    const int setAddrRc = jamulus_core_set_server_addr ( core, addr.c_str() );
    DebugLogger::instance().log ( "[JamulusPluginProcessor] jamulus_core_set_server_addr rc=" + std::to_string ( setAddrRc ) );
    if ( setAddrRc != 0 )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] ERROR: server address rejected, aborting connectToServer" );
        return;
    }

    int rc = jamulus_core_start ( core );
    DebugLogger::instance().log ( "[JamulusPluginProcessor] jamulus_core_start rc=" + std::to_string ( rc ) );
    syncAllHiddenRouteClients();
}

void JamulusPluginProcessor::disconnectFromServer()
{
    stopRecording();
    if ( core )
    {
        jamulus_core_disconnect ( core );
    }
    currentServerName.clear();
    currentServerAddress.clear();
    destroyHiddenRouteClients();
}

void JamulusPluginProcessor::setChannelReferenceGain ( int channelId, float gain )
{
    std::lock_guard<std::mutex> lock ( routeStateMutex );
    channelReferenceGains[channelId] = juce::jlimit ( 0.0f, 1.0f, gain );
}

JamulusPluginProcessor::RecordingSettings JamulusPluginProcessor::getRecordingSettings() const
{
    std::lock_guard<std::mutex> lock ( recordingMutex );
    return recordingSettings;
}

void JamulusPluginProcessor::setRecordingSettings ( const RecordingSettings& settings )
{
    {
        std::lock_guard<std::mutex> lock ( recordingMutex );
        recordingSettings = settings;
    }

    syncAllHiddenRouteClients();
}

bool JamulusPluginProcessor::isRecordingActive() const
{
    std::lock_guard<std::mutex> lock ( recordingMutex );
    return recordingRuntime.active;
}

bool JamulusPluginProcessor::isConnected() const
{
    if ( core )
        return jamulus_core_is_connected ( core ) != 0;
    return false;
}

jamulus_client_t JamulusPluginProcessor::getClient()
{
    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating core in getClient" );
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( core )
            syncSettingsToCore ( core );
    }

    return reinterpret_cast<jamulus_client_t> ( core );
}

void JamulusPluginProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
    try
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] prepareToPlay called. SampleRate: " + std::to_string ( sampleRate ) +
                                      ", BlockSize: " + std::to_string ( samplesPerBlock ) );
        procNumChannels = 2;
        procBlockSize   = samplesPerBlock > 0 ? samplesPerBlock : 128;
        hostSampleRate  = sampleRate;
        audioReverb.init ( static_cast<int> ( sampleRate ) );
        audioDelay.init ( static_cast<int> ( sampleRate ), 1000 );

        needsResampling = std::abs ( hostSampleRate - JAMULUS_SAMPLE_RATE ) >= 1.0;
        if ( needsResampling )
        {
            DebugLogger::instance().log ( "[JamulusPluginProcessor] Host sample rate differs from Jamulus, enabling resampling" );
            inputResampler.prepare ( hostSampleRate, JAMULUS_SAMPLE_RATE, procNumChannels );
            outputResampler.prepare ( JAMULUS_SAMPLE_RATE, hostSampleRate, procNumChannels );
        }
    }
    catch ( const std::exception& e )
    {
        DebugLogger::instance().log ( std::string ( "[JamulusPluginProcessor] prepareToPlay exception: " ) + e.what() );
    }
    catch ( ... )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] prepareToPlay unknown exception" );
    }
}

bool JamulusPluginProcessor::startRecording()
{
    stopRecording();

    RecordingSettings settings;
    {
        std::lock_guard<std::mutex> lock ( recordingMutex );
        settings = recordingSettings;
    }

    if ( settings.mode == RecordingSettings::recordingOff || hostSampleRate <= 0.0 )
        return false;

    std::vector<int> trackedIds;
    std::vector<juce::String> trackedNames;

    const int myChannelId = getMyChannelId();
    const int numChannels = getNumChannels();
    for ( int index = 0; index < numChannels; ++index )
    {
        jamulus_channel_info_t info{};
        if ( !getChannelInfo ( index, info ) )
            continue;

        if ( info.id == myChannelId )
            continue;

        trackedIds.push_back ( info.id );
        trackedNames.push_back ( juce::String::fromUTF8 ( info.name ) );
    }

    if ( trackedIds.empty() )
        return false;

    auto folder = settings.folderPath.isNotEmpty() ? juce::File ( settings.folderPath ) : getDefaultRecordingFolder();
    if ( !folder.exists() )
        folder.createDirectory();

    const auto serverStem = makeSafeRecordingName ( juce::String ( currentServerName.empty() ? currentServerAddress : currentServerName ) );
    const auto timeStamp  = juce::Time::getCurrentTime().formatted ( "%Y%m%d_%H%M%S" );
    const auto baseName   = timeStamp + "_" + serverStem;

    juce::String extension = "wav";
    int writerChannels = 2;

    switch ( settings.mode )
    {
        case RecordingSettings::stereoOgg: extension = "ogg"; break;
        case RecordingSettings::stereoMp3: extension = "mp3"; break;
        case RecordingSettings::multitrackMogg:
            extension = "mogg";
            writerChannels = static_cast<int> ( trackedIds.size() ) * 2;
            break;
        default: break;
    }

    auto targetFile = folder.getChildFile ( baseName + "." + extension );
    std::unique_ptr<juce::OutputStream> outStream ( targetFile.createOutputStream() );
    if ( !outStream )
        return false;
    auto* rawStream = outStream.release();

    std::unique_ptr<juce::AudioFormatWriter> formatWriter;
    juce::StringPairArray metadata;

    if ( settings.mode == RecordingSettings::stereoWav )
    {
        juce::WavAudioFormat format;
        formatWriter.reset ( format.createWriterFor ( rawStream, hostSampleRate, 2, 24, metadata, 0 ) );
    }
    else if ( settings.mode == RecordingSettings::stereoOgg || settings.mode == RecordingSettings::multitrackMogg )
    {
        juce::OggVorbisAudioFormat format;
        formatWriter.reset ( format.createWriterFor ( rawStream, hostSampleRate, static_cast<unsigned int> ( writerChannels ), 16, metadata, 0 ) );
    }
    else if ( settings.mode == RecordingSettings::stereoMp3 )
    {
        delete rawStream;
        return false;
    }

    if ( !formatWriter )
    {
        delete rawStream;
        return false;
    }

    auto writerThread = std::make_unique<juce::TimeSliceThread> ( "Jamulus Record Writer" );
    writerThread->startThread();
    auto threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> ( formatWriter.release(), *writerThread, 32768 );

    {
        std::lock_guard<std::mutex> lock ( recordingMutex );
        recordingRuntime.active            = true;
        recordingRuntime.writerThread      = std::move ( writerThread );
        recordingRuntime.writer            = std::move ( threadedWriter );
        recordingRuntime.trackedChannelIds = trackedIds;
        recordingRuntime.trackedChannelNames = trackedNames;
        recordingRuntime.targetFile        = targetFile;
        if ( settings.mode == RecordingSettings::multitrackMogg )
            recordingRuntime.sidecarFile = folder.getChildFile ( baseName + "_channels.txt" );
    }

    if ( settings.mode == RecordingSettings::multitrackMogg )
    {
        juce::StringArray lines;
        for ( int i = 0; i < static_cast<int> ( trackedNames.size() ); ++i )
            lines.add ( juce::String ( i * 2 + 1 ) + "/" + juce::String ( i * 2 + 2 ) + " = " + trackedNames[static_cast<size_t> ( i ) ] );
        recordingRuntime.sidecarFile.replaceWithText ( lines.joinIntoString ( "\n" ) );
    }

    syncAllHiddenRouteClients();
    return true;
}

void JamulusPluginProcessor::stopRecording()
{
    std::lock_guard<std::mutex> lock ( recordingMutex );
    recordingRuntime.writer.reset();
    if ( recordingRuntime.writerThread )
    {
        recordingRuntime.writerThread->stopThread ( 2000 );
        recordingRuntime.writerThread.reset();
    }
    recordingRuntime.active = false;
    recordingRuntime.trackedChannelIds.clear();
    recordingRuntime.trackedChannelNames.clear();
    recordingRuntime.targetFile = juce::File();
    recordingRuntime.sidecarFile = juce::File();
}

void JamulusPluginProcessor::processBlock ( const float** inputs, float** outputs, int numChannels, int numSamples )
{
    applyBackendSwitchIfRequested();
    clearAuxBusBuffers ( numSamples );

    // always process internally as stereo to ensure effects/jamulus work correctly
    const int          internalChannels = 2;
    std::vector<float> internalIn ( static_cast<size_t> ( numSamples ) * internalChannels );

    // 1. Prepare Input (Host -> Internal Stereo)
    if ( testToneEnabled )
    {
        constexpr double PI       = 3.14159265358979323846;
        double           phaseInc = 2.0 * PI * testToneFreq / hostSampleRate;
        for ( int i = 0; i < numSamples; ++i )
        {
            float sample = static_cast<float> ( std::sin ( phase ) * testToneAmp );
            phase += phaseInc;
            if ( phase > 2.0 * PI )
                phase -= 2.0 * PI;
            // Write to both channels
            internalIn[static_cast<size_t> ( i ) * 2 + 0] = sample;
            internalIn[static_cast<size_t> ( i ) * 2 + 1] = sample;
        }
    }
    else
    {
        for ( int i = 0; i < numSamples; ++i )
        {
            if ( numChannels >= 2 )
            {
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = inputs[0] ? inputs[0][i] : 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = inputs[1] ? inputs[1][i] : 0.0f;
            }
            else if ( numChannels == 1 )
            {
                float sample                                  = inputs[0] ? inputs[0][i] : 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = sample;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = sample;
            }
            else
            {
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = 0.0f;
            }
        }
    }

    const float g = inputFaderGain;
    if ( g != 1.0f )
    {
        const int total = numSamples * internalChannels;
        for ( int i = 0; i < total; ++i )
            internalIn[static_cast<size_t> ( i )] *= g;
    }

    float inPeakL = 0.0f;
    float inPeakR = 0.0f;
    for ( int i = 0; i < numSamples; ++i )
    {
        const float l = internalIn[static_cast<size_t> ( i ) * 2 + 0];
        const float r = internalIn[static_cast<size_t> ( i ) * 2 + 1];
        if ( std::abs ( l ) > inPeakL )
            inPeakL = std::abs ( l );
        if ( std::abs ( r ) > inPeakR )
            inPeakR = std::abs ( r );
    }
    lastInputPeakLeft  = inPeakL;
    lastInputPeakRight = inPeakR;

    std::vector<float> internalOut ( static_cast<size_t> ( numSamples ) * internalChannels );

    if ( core )
    {
        if ( needsResampling )
        {
            int                maxJamulusFrames = static_cast<int> ( numSamples * JAMULUS_SAMPLE_RATE / hostSampleRate ) + 16;
            std::vector<float> jamulusIn ( maxJamulusFrames * internalChannels );
            std::vector<float> jamulusOut ( maxJamulusFrames * internalChannels );

            int jamulusFrames = inputResampler.resampleTo48k ( internalIn.data(), numSamples, jamulusIn.data(), maxJamulusFrames );

            int rc = jamulus_core_process_audio ( core, jamulusIn.data(), jamulusOut.data(), jamulusFrames, internalChannels );
            if ( rc != 0 )
            {
                std::fill ( jamulusOut.begin(), jamulusOut.end(), 0.0f );
            }

            outputResampler.resampleFrom48k ( jamulusOut.data(), jamulusFrames, internalOut.data(), numSamples );
        }
        else
        {
            int rc = jamulus_core_process_audio ( core, internalIn.data(), internalOut.data(), numSamples, internalChannels );
            if ( rc != 0 )
            {
                std::fill ( internalOut.begin(), internalOut.end(), 0.0f );
            }
        }
    }
    else
    {
        std::fill ( internalOut.begin(), internalOut.end(), 0.0f );
    }
    // Apply Reverb (if enabled) - works on Stereo Internal Buffer
    if ( reverbEnabled )
    {
        audioReverb.process ( internalOut.data(), numSamples );
    }

    // Apply Delay (if enabled) - works on Stereo Internal Buffer
    if ( delayEnabled )
    {
        audioDelay.process ( internalOut.data(), numSamples );
    }

    if ( monitorMode )
    {
        for ( size_t i = 0; i < internalOut.size(); ++i )
        {
            internalOut[i] += internalIn[i];
        }
    }

    {
        const float epsilon = 1e-6f;
        float       outPeakL = 0.0f;
        float       outPeakR = 0.0f;
        for ( int i = 0; i < numSamples; ++i )
        {
            const float l = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            const float r = internalOut[static_cast<size_t> ( i ) * 2 + 1];
            if ( std::abs ( l ) > outPeakL )
                outPeakL = std::abs ( l );
            if ( std::abs ( r ) > outPeakR )
                outPeakR = std::abs ( r );
        }

        if ( outPeakL < epsilon && outPeakR < epsilon && ( lastInputPeakLeft > epsilon || lastInputPeakRight > epsilon ) )
        {
            const size_t total = static_cast<size_t> ( numSamples ) * 2;
            for ( size_t i = 0; i < total; ++i )
                internalOut[i] = internalIn[i];

            outPeakL = 0.0f;
            outPeakR = 0.0f;
            for ( int i = 0; i < numSamples; ++i )
            {
                const float l = internalOut[static_cast<size_t> ( i ) * 2 + 0];
                const float r = internalOut[static_cast<size_t> ( i ) * 2 + 1];
                if ( std::abs ( l ) > outPeakL )
                    outPeakL = std::abs ( l );
                if ( std::abs ( r ) > outPeakR )
                    outPeakR = std::abs ( r );
            }
        }
    }

    if ( !currentServerAddress.empty() )
        syncAllHiddenRouteClients();

    RecordingSettings recordingSettingsSnapshot;
    bool recordingActiveSnapshot = false;
    std::vector<int> recordingTrackedIds;
    {
        std::lock_guard<std::mutex> lock ( recordingMutex );
        recordingSettingsSnapshot = recordingSettings;
        recordingActiveSnapshot   = recordingRuntime.active && recordingRuntime.writer != nullptr;
        recordingTrackedIds       = recordingRuntime.trackedChannelIds;
    }

    juce::AudioBuffer<float> stereoRecordBuffer;
    juce::AudioBuffer<float> stemRecordBuffer;
    if ( recordingActiveSnapshot )
    {
        if ( recordingSettingsSnapshot.mode == RecordingSettings::multitrackMogg )
            stemRecordBuffer.setSize ( juce::jmax ( 2, static_cast<int> ( recordingTrackedIds.size() ) * 2 ), numSamples, false, true, true );
        else if ( recordingSettingsSnapshot.mode != RecordingSettings::recordingOff )
            stereoRecordBuffer.setSize ( 2, numSamples, false, true, true );
    }

    {
        std::vector<float> silentIn ( static_cast<size_t> ( numSamples ) * internalChannels, 0.0f );

        std::lock_guard<std::mutex> lock ( routeStateMutex );
        for ( auto& [channelId, hiddenClient] : hiddenRouteClients )
        {
            if ( hiddenClient.core == nullptr )
                continue;

            const auto routeIt = channelControlStates.find ( channelId );
            if ( routeIt == channelControlStates.end() )
                continue;

            const auto route = decodeOutputRoute ( routeIt->second.route );

            std::vector<float> stemOut ( static_cast<size_t> ( numSamples ) * internalChannels, 0.0f );
            if ( hiddenClient.needsResampling )
            {
                const int maxJamulusFrames = static_cast<int> ( numSamples * JAMULUS_SAMPLE_RATE / hostSampleRate ) + 16;
                std::vector<float> jamulusIn ( static_cast<size_t> ( maxJamulusFrames ) * internalChannels, 0.0f );
                std::vector<float> jamulusOut ( static_cast<size_t> ( maxJamulusFrames ) * internalChannels, 0.0f );
                const int jamulusFrames =
                    hiddenClient.inputResampler.resampleTo48k ( silentIn.data(), numSamples, jamulusIn.data(), maxJamulusFrames );

                if ( jamulus_core_process_audio ( hiddenClient.core, jamulusIn.data(), jamulusOut.data(), jamulusFrames, internalChannels ) == 0 )
                    hiddenClient.outputResampler.resampleFrom48k ( jamulusOut.data(), jamulusFrames, stemOut.data(), numSamples );
            }
            else
            {
                jamulus_core_process_audio ( hiddenClient.core, silentIn.data(), stemOut.data(), numSamples, internalChannels );
            }

            const float liveGain = routeIt->second.muted ? 0.0f : routeIt->second.gain;
            auto refGainIt = channelReferenceGains.find ( channelId );
            const float refGain = routeIt->second.muted ? 0.0f : ( refGainIt != channelReferenceGains.end() ? refGainIt->second : routeIt->second.gain );

            if ( route.renderToMaster )
            {
                auto liveStem = stemOut;
                applyGainToInterleaved ( liveStem, liveGain );
                mixStemIntoMain ( liveStem, internalOut, route.channelMode );
            }
            else if ( route.renderToAux && route.auxBusIndex >= 0 && route.auxBusIndex < kAuxStereoBusCount )
            {
                applyGainToInterleaved ( stemOut, liveGain );
                auxBusBuffers[static_cast<size_t> ( route.auxBusIndex )] = stemOut;
            }

            if ( !recordingActiveSnapshot )
                continue;

            auto trackIt = std::find ( recordingTrackedIds.begin(), recordingTrackedIds.end(), channelId );
            if ( trackIt == recordingTrackedIds.end() )
                continue;

            const int trackIndex = static_cast<int> ( std::distance ( recordingTrackedIds.begin(), trackIt ) );
            const bool useLiveRecordGain = recordingSettingsSnapshot.mode == RecordingSettings::multitrackMogg
                                               ? recordingSettingsSnapshot.stemsUseLiveAutoLevel
                                               : recordingSettingsSnapshot.stereoUseLiveAutoLevel;
            const float recordGain = useLiveRecordGain ? liveGain : refGain;

            auto recordStem = stemOut;
            applyGainToInterleaved ( recordStem, recordGain );

            if ( recordingSettingsSnapshot.mode == RecordingSettings::multitrackMogg )
            {
                const int leftChannel  = trackIndex * 2;
                const int rightChannel = leftChannel + 1;
                for ( int sample = 0; sample < numSamples; ++sample )
                {
                    stemRecordBuffer.setSample ( leftChannel, sample, recordStem[static_cast<size_t> ( sample ) * 2 + 0] );
                    stemRecordBuffer.setSample ( rightChannel, sample, recordStem[static_cast<size_t> ( sample ) * 2 + 1] );
                }
            }
            else
            {
                for ( int sample = 0; sample < numSamples; ++sample )
                {
                    stereoRecordBuffer.addSample ( 0, sample, recordStem[static_cast<size_t> ( sample ) * 2 + 0] );
                    stereoRecordBuffer.addSample ( 1, sample, recordStem[static_cast<size_t> ( sample ) * 2 + 1] );
                }
            }
        }
    }

    if ( recordingActiveSnapshot )
    {
        std::lock_guard<std::mutex> lock ( recordingMutex );
        if ( recordingRuntime.active && recordingRuntime.writer != nullptr )
        {
            if ( recordingSettingsSnapshot.mode == RecordingSettings::multitrackMogg )
            {
                recordingRuntime.writer->write ( stemRecordBuffer.getArrayOfReadPointers(), numSamples );
            }
            else if ( recordingSettingsSnapshot.mode != RecordingSettings::recordingOff )
            {
                if ( recordingSettingsSnapshot.stereoApplyLimiter )
                {
                    std::vector<float> stereoInterleaved ( static_cast<size_t> ( numSamples ) * 2 );
                    for ( int sample = 0; sample < numSamples; ++sample )
                    {
                        stereoInterleaved[static_cast<size_t> ( sample ) * 2 + 0] = stereoRecordBuffer.getSample ( 0, sample );
                        stereoInterleaved[static_cast<size_t> ( sample ) * 2 + 1] = stereoRecordBuffer.getSample ( 1, sample );
                    }
                    clampSoftLimitInterleaved ( stereoInterleaved );
                    for ( int sample = 0; sample < numSamples; ++sample )
                    {
                        stereoRecordBuffer.setSample ( 0, sample, stereoInterleaved[static_cast<size_t> ( sample ) * 2 + 0] );
                        stereoRecordBuffer.setSample ( 1, sample, stereoInterleaved[static_cast<size_t> ( sample ) * 2 + 1] );
                    }
                }

                recordingRuntime.writer->write ( stereoRecordBuffer.getArrayOfReadPointers(), numSamples );
            }
        }
    }

    // Apply main volume to final output (after all mixing/fallback paths)
    if ( mainVolume != 1.0f )
    {
        for ( size_t i = 0; i < internalOut.size(); ++i )
            internalOut[i] *= mainVolume;
    }

    // Apply final soft clipper on the post-master signal so peaks above 0 dB
    // are rounded off and capped just below full scale.
    if ( limiterEnabled )
    {
        constexpr float limiterCeiling    = 0.9885531f; // -0.1 dBFS
        constexpr float softClipThreshold = 0.92f;
        const float thresholdLinear =
            juce::jlimit ( 0.05f, limiterCeiling, std::pow ( 10.0f, juce::jlimit ( -24.0f, 0.0f, limiterThresholdDb ) / 20.0f ) );
        const float autoMakeupGain = juce::jmax ( 1.0f, limiterCeiling / thresholdLinear );
        const float releaseSamples = juce::jmax ( 1.0f, limiterReleaseMs * 0.001f * static_cast<float> ( hostSampleRate ) );
        const float releaseCoeff   = std::exp ( -1.0f / releaseSamples );

        auto applySoftCeiling = [softClipThreshold, limiterCeiling] ( float sample ) {
            const float sign  = ( sample < 0.0f ) ? -1.0f : 1.0f;
            const float absIn = std::abs ( sample );

            if ( absIn <= softClipThreshold )
                return sample;

            float absOut = limiterCeiling;
            if ( absIn < limiterCeiling )
            {
                const float t = ( absIn - softClipThreshold ) / ( limiterCeiling - softClipThreshold );
                const float shaped = t * t * ( 3.0f - 2.0f * t );
                absOut = softClipThreshold + ( limiterCeiling - softClipThreshold ) * shaped;
            }

            return sign * std::min ( absOut, limiterCeiling );
        };

        if ( limiterEnvelope <= 0.0f || limiterEnvelope > 1.0f )
            limiterEnvelope = 1.0f;

        for ( int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex )
        {
            const size_t base = static_cast<size_t> ( sampleIndex ) * 2;
            const float leftPeak  = std::abs ( internalOut[base + 0] );
            const float rightPeak = std::abs ( internalOut[base + 1] );
            const float peak      = juce::jmax ( leftPeak, rightPeak );
            const float targetGain = peak > thresholdLinear && peak > 0.0f ? ( thresholdLinear / peak ) : 1.0f;

            if ( targetGain < limiterEnvelope )
                limiterEnvelope = targetGain;
            else
                limiterEnvelope = 1.0f - ( 1.0f - limiterEnvelope ) * releaseCoeff;

            for ( int channel = 0; channel < 2; ++channel )
            {
                float sample = internalOut[base + static_cast<size_t> ( channel )] * limiterEnvelope;
                sample = applySoftCeiling ( sample );
                sample *= autoMakeupGain;
                sample = applySoftCeiling ( sample );

                internalOut[base + static_cast<size_t> ( channel )] = sample;
            }
        }
    }

    float outPeakL = 0.0f;
    float outPeakR = 0.0f;
    for ( int i = 0; i < numSamples; ++i )
    {
        const float l = internalOut[static_cast<size_t> ( i ) * 2 + 0];
        const float r = internalOut[static_cast<size_t> ( i ) * 2 + 1];
        if ( std::abs ( l ) > outPeakL )
            outPeakL = std::abs ( l );
        if ( std::abs ( r ) > outPeakR )
            outPeakR = std::abs ( r );
    }

    lastOutputPeakLeft  = outPeakL;
    lastOutputPeakRight = outPeakR;

    // 2. Output (Internal Stereo -> Host Output)
    for ( int i = 0; i < numSamples; ++i )
    {
        if ( numChannels >= 2 )
        {
            // Stereo Output
            if ( outputs[0] )
                outputs[0][i] = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            if ( outputs[1] )
                outputs[1][i] = internalOut[static_cast<size_t> ( i ) * 2 + 1];
        }
        else if ( numChannels == 1 )
        {
            // Mono Output -> Average L+R
            float left  = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            float right = internalOut[static_cast<size_t> ( i ) * 2 + 1];
            if ( outputs[0] )
                outputs[0][i] = ( left + right ) * 0.5f;
        }
    }
}

void JamulusPluginProcessor::captureMidiForLearn ( const juce::MidiBuffer& midiMessages )
{
    std::lock_guard<std::mutex> lock ( midiLearnMutex );

    for ( const auto metadata : midiMessages )
    {
        const auto& msg = metadata.getMessage();
        if ( !msg.isController() )
            continue;

        MidiLearnEvent event;
        event.channel         = msg.getChannel();
        event.controller      = msg.getControllerNumber();
        event.normalizedValue = juce::jlimit ( 0.0f, 1.0f, msg.getControllerValue() / 127.0f );
        pendingMidiLearnEvents.push_back ( event );
    }

    while ( pendingMidiLearnEvents.size() > 64 )
        pendingMidiLearnEvents.pop_front();
}

bool JamulusPluginProcessor::popNextMidiLearnEvent ( MidiLearnEvent& event )
{
    std::lock_guard<std::mutex> lock ( midiLearnMutex );
    if ( pendingMidiLearnEvents.empty() )
        return false;

    event = pendingMidiLearnEvents.front();
    pendingMidiLearnEvents.pop_front();
    return true;
}

void JamulusPluginProcessor::enqueueMidiOutputCc ( int channel, int controller, int value )
{
    std::lock_guard<std::mutex> lock ( midiOutputMutex );
    pendingMidiOutputMessages.push_back ( juce::MidiMessage::controllerEvent ( juce::jlimit ( 1, 16, channel ),
                                                                                juce::jlimit ( 0, 127, controller ),
                                                                                juce::jlimit ( 0, 127, value ) ) );
    while ( pendingMidiOutputMessages.size() > 256 )
        pendingMidiOutputMessages.pop_front();
}

void JamulusPluginProcessor::drainMidiOutput ( juce::MidiBuffer& midiMessages )
{
    std::lock_guard<std::mutex> lock ( midiOutputMutex );
    for ( const auto& msg : pendingMidiOutputMessages )
        midiMessages.addEvent ( msg, 0 );
    pendingMidiOutputMessages.clear();
}

void JamulusPluginProcessor::releaseResources()
{
    DebugLogger::instance().log ( "[JamulusPluginProcessor] releaseResources called" );
    // Reset resamplers
    if ( needsResampling )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Resetting resamplers" );
        inputResampler.reset();
        outputResampler.reset();
    }

    delete inputFifo;
    inputFifo = nullptr;
    delete outputFifo;
    outputFifo = nullptr;
    clearAuxBusBuffers ( 0 );
}

void JamulusPluginProcessor::resetAudioState()
{
    DebugLogger::instance().log ( "[JamulusPluginProcessor] resetAudioState called" );

    if ( core )
    {
        if ( jamulus_core_is_connected ( core ) )
        {
            DebugLogger::instance().log ( "[JamulusPluginProcessor] Active core connection detected, disconnecting..." );
            jamulus_core_disconnect ( core );
        }

        if ( jamulus_core_is_running ( core ) )
        {
            DebugLogger::instance().log ( "[JamulusPluginProcessor] Core running but not connected, stopping..." );
            jamulus_core_stop ( core );
        }

        jamulus_core_set_server_addr ( core, nullptr );
    }

    destroyHiddenRouteClients();

    // 2. Clear effects tails
    audioReverb.clear();
    audioDelay.clear();

    // 3. Reset limiter envelope
    limiterEnvelope = 0.0f;

    DebugLogger::instance().log ( "[JamulusPluginProcessor] Global state reset complete" );
}

void JamulusPluginProcessor::setChannelGain ( int channelId, float gain )
{
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelControlStates[channelId].gain = gain;
        if ( !channelReferenceGains.count ( channelId ) )
            channelReferenceGains[channelId] = gain;
    }
    syncMainCoreChannelState ( channelId );
    syncHiddenRouteClient ( channelId );
}

void JamulusPluginProcessor::setChannelPan ( int channelId, float pan )
{
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelControlStates[channelId].pan = pan;
    }
    syncMainCoreChannelState ( channelId );
    syncHiddenRouteClient ( channelId );
}

void JamulusPluginProcessor::setChannelMute ( int channelId, bool muted )
{
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelControlStates[channelId].muted = muted;
    }
    syncMainCoreChannelState ( channelId );
    syncHiddenRouteClient ( channelId );
}

void JamulusPluginProcessor::setChannelSolo ( int channelId, bool solo )
{
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelControlStates[channelId].solo = solo;
    }
    if ( core )
        jamulus_core_set_channel_solo ( core, channelId, solo );
}

void JamulusPluginProcessor::setChannelOutputRoute ( int channelId, int routeCode )
{
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelControlStates[channelId].route = routeCode;
    }

    syncMainCoreChannelState ( channelId );
    syncHiddenRouteClient ( channelId );
}

int JamulusPluginProcessor::getChannelOutputRoute ( int channelId ) const
{
    std::lock_guard<std::mutex> lock ( routeStateMutex );
    const auto it = channelControlStates.find ( channelId );
    return ( it != channelControlStates.end() ) ? it->second.route : 0;
}

void JamulusPluginProcessor::copyAuxBusBuffer ( int busIndex, float** outputs, int numChannels, int numSamples ) const
{
    if ( busIndex < 0 || busIndex >= kAuxStereoBusCount )
        return;

    std::lock_guard<std::mutex> lock ( routeStateMutex );
    const auto& buffer = auxBusBuffers[static_cast<size_t> ( busIndex )];
    if ( buffer.empty() )
        return;

    writeInterleavedToBus ( buffer, outputs, numChannels, numSamples, 2 );
}

int JamulusPluginProcessor::getNumChannels() const
{
    if ( core )
        return jamulus_core_get_num_channels ( core );
    return 0;
}

int JamulusPluginProcessor::getMyChannelId() const
{
    if ( core )
        return jamulus_core_get_my_channel_id ( core );
    return -1;
}

bool JamulusPluginProcessor::getChannelInfo ( int index, jamulus_channel_info_t& info ) const
{
    if ( core )
        return jamulus_core_get_channel_info ( core, index, &info );
    return false;
}

void JamulusPluginProcessor::sendChatMessage ( const std::string& message )
{
    if ( message.empty() )
        return;

    if ( core )
        jamulus_core_send_chat_message ( core, message.c_str() );
}

std::string JamulusPluginProcessor::getProfileName() const
{
    if ( core )
    {
        const char* name = jamulus_core_get_name ( core );
        if ( name )
            return std::string ( name );
    }
    return std::string();
}

void JamulusPluginProcessor::setProfileName ( const std::string& name )
{
    profileName = name;
    if ( core )
        jamulus_core_set_name ( core, name.c_str() );
    syncAllHiddenRouteClients();
}

int JamulusPluginProcessor::getProfileInstrument() const
{
    if ( core )
        return jamulus_core_get_instrument ( core );
    return 0;
}

void JamulusPluginProcessor::setProfileInstrument ( int instrument )
{
    profileInstrument = instrument;
    if ( core )
        jamulus_core_set_instrument ( core, instrument );
    syncAllHiddenRouteClients();
}

int JamulusPluginProcessor::getProfileCountry() const
{
    if ( core )
        return jamulus_core_get_country ( core );
    return 0;
}

void JamulusPluginProcessor::setProfileCountry ( int country )
{
    profileCountry = country;
    if ( core )
        jamulus_core_set_country ( core, country );
    syncAllHiddenRouteClients();
}

std::string JamulusPluginProcessor::getProfileCountryCode() const
{
    if ( core )
    {
        const char* code = jamulus_core_get_country_code ( core );
        if ( code )
            return std::string ( code );
    }
    return std::string();
}

std::string JamulusPluginProcessor::getProfileCity() const
{
    if ( core )
    {
        const char* city = jamulus_core_get_city ( core );
        if ( city )
            return std::string ( city );
    }
    return std::string();
}

void JamulusPluginProcessor::setProfileCity ( const std::string& city )
{
    profileCity = city;
    if ( core )
        jamulus_core_set_city ( core, city.c_str() );
    syncAllHiddenRouteClients();
}

int JamulusPluginProcessor::getProfileSkill() const
{
    if ( core )
        return jamulus_core_get_skill ( core );
    return 0;
}

void JamulusPluginProcessor::setProfileSkill ( int skill )
{
    profileSkill = skill;
    if ( core )
        jamulus_core_set_skill ( core, skill );
    syncAllHiddenRouteClients();
}

int JamulusPluginProcessor::getNumServers() const
{
    if ( core )
        return jamulus_core_get_num_servers ( core );
    return 0;
}

bool JamulusPluginProcessor::getServerInfoAt ( int index, jamulus_server_info_t& info ) const
{
    if ( core )
        return jamulus_core_get_server_info ( core, index, &info );
    return false;
}

void JamulusPluginProcessor::clearServerList()
{
    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating non-Qt core in clearServerList" );
        core = jamulus_core_create ( 0, nullptr, "JUCE Jamulus Core", true );
        if ( core )
            jamulus_core_start ( core );
    }
    if ( core )
        jamulus_core_clear_server_list ( core );
}

void JamulusPluginProcessor::requestServerList ( int directoryType, const std::string& customAddress )
{
    DebugLogger::instance().log ( "[JamulusPluginProcessor] requestServerList called. dirType=" + std::to_string ( directoryType ) +
                                  ", customAddress=" + customAddress );

    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating non-Qt core in requestServerList" );
        core = jamulus_core_create ( 0, nullptr, "JUCE Jamulus Core", true );
        if ( core )
            jamulus_core_start ( core );
    }
    if ( core )
        jamulus_core_request_server_list ( core, directoryType, customAddress.c_str() );
}

void JamulusPluginProcessor::requestServerClientList ( const std::string& address )
{
    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating non-Qt core in requestServerClientList" );
        core = jamulus_core_create ( 0, nullptr, "JUCE Jamulus Core", true );
        if ( core )
            jamulus_core_start ( core );
    }
    if ( core )
        jamulus_core_request_server_client_list ( core, address.c_str() );
}

void JamulusPluginProcessor::pingServerList()
{
    if ( !core )
    {
        DebugLogger::instance().log ( "[JamulusPluginProcessor] Creating non-Qt core in pingServerList" );
        core = jamulus_core_create ( 0, nullptr, "JUCE Jamulus Core", true );
        if ( core )
            jamulus_core_start ( core );
    }
    if ( core )
        jamulus_core_ping_server_list ( core );
}

bool JamulusPluginProcessor::getAutoJitter() const
{
    if ( core )
        return jamulus_core_get_auto_jitter ( core );
    return autoJitterEnabled;
}

void JamulusPluginProcessor::setAutoJitter ( bool enabled )
{
    autoJitterEnabled = enabled;
    if ( !core )
    {
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( core )
        {
            syncSettingsToCore ( core );
            jamulus_core_start ( core );
        }
    }
    if ( core )
        jamulus_core_set_auto_jitter ( core, enabled );
    syncAllHiddenRouteClients();
}

int JamulusPluginProcessor::getJitterBuffer() const
{
    if ( core )
        return jamulus_core_get_jitter_buffer ( core );
    return jitterBufferSize;
}

void JamulusPluginProcessor::setJitterBuffer ( int size )
{
    jitterBufferSize = size;
    if ( !core )
    {
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( core )
        {
            syncSettingsToCore ( core );
            jamulus_core_start ( core );
        }
    }
    if ( core )
        jamulus_core_set_jitter_buffer ( core, size );
    syncAllHiddenRouteClients();
}


int JamulusPluginProcessor::getServerJitterBuffer() const
{
    if ( core )
        return jamulus_core_get_server_jitter_buffer ( core );
    return serverJitterBufferSize;
}

void JamulusPluginProcessor::setServerJitterBuffer ( int size )
{
    serverJitterBufferSize = size;
    if ( !core )
    {
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( core )
        {
            syncSettingsToCore ( core );
            jamulus_core_start ( core );
        }
    }
    if ( core )
        jamulus_core_set_server_jitter_buffer ( core, size );
    syncAllHiddenRouteClients();
}
bool JamulusPluginProcessor::getSmallBuffers() const
{
    if ( core )
        return jamulus_core_get_small_buffers ( core );
    return smallBuffersEnabled;
}

void JamulusPluginProcessor::setSmallBuffers ( bool enabled )
{
    smallBuffersEnabled = enabled;
    if ( !core )
    {
        core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( core )
        {
            syncSettingsToCore ( core );
            jamulus_core_start ( core );
        }
    }
    if ( core )
        jamulus_core_set_small_buffers ( core, enabled );
    syncAllHiddenRouteClients();
}

void JamulusPluginProcessor::setInputFader ( int value )
{
    if ( value < 0 )
        value = 0;
    if ( value > 100 )
        value = 100;

    inputFaderValue = value;

    float gain = 1.0f;
    if ( value <= 50 )
        gain = value / 50.0f;
    else
        gain = 1.0f + ( ( value - 50 ) / 50.0f );

    if ( gain < 0.0f )
        gain = 0.0f;

    inputFaderGain = gain;
}

void JamulusPluginProcessor::sendPing()
{
    if ( core )
        jamulus_core_send_ping ( core );
}

bool JamulusPluginProcessor::getNetworkStats ( jamulus_network_stats_t& stats ) const
{
    if ( core )
        return jamulus_core_get_network_stats ( core, &stats );
    return false;
}

bool JamulusPluginProcessor::getNextChatMessage ( std::string& message )
{
    message.clear();
    if ( !core )
        return false;
    const char* msg = jamulus_core_get_chat_message ( core );
    if ( !msg || !*msg )
        return false;
    message = msg;
    return true;
}

void JamulusPluginProcessor::setAutoTranslateEnabled ( bool enabled )
{
    autoTranslateEnabled = enabled;
}

void JamulusPluginProcessor::setTranslateTargetLang ( const juce::String& langCode )
{
    auto normalised = langCode.trim().toLowerCase();
    if ( normalised.isEmpty() )
        normalised = "system";

    translateTargetLang = normalised;
}

void JamulusPluginProcessor::destroyHiddenRouteClients()
{
    std::lock_guard<std::mutex> lock ( routeStateMutex );
    for ( auto& [channelId, hiddenClient] : hiddenRouteClients )
    {
        if ( hiddenClient.core != nullptr )
        {
            jamulus_core_disconnect ( hiddenClient.core );
            jamulus_core_stop ( hiddenClient.core );
            jamulus_core_destroy ( hiddenClient.core );
            hiddenClient.core = nullptr;
        }
    }
    hiddenRouteClients.clear();
}

void JamulusPluginProcessor::syncMainCoreChannelState ( int channelId )
{
    if ( core == nullptr )
        return;

    ChannelControlState state;
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        state = channelControlStates[channelId];
    }

    const auto route = decodeOutputRoute ( state.route );
    const float mainGain = ( route.isMasterStereo && !state.muted ) ? state.gain : 0.0f;

    jamulus_core_set_channel_gain ( core, channelId, mainGain );
    jamulus_core_set_channel_pan ( core, channelId, state.pan );
    jamulus_core_set_channel_mute ( core, channelId, !route.isMasterStereo || state.muted );
}

void JamulusPluginProcessor::syncHiddenRouteClient ( int channelId )
{
    const ChannelControlState state = [this, channelId]() {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        return channelControlStates[channelId];
    }();

    const auto route = decodeOutputRoute ( state.route );
    if ( !shouldMaintainHiddenClientForChannel ( channelId ) || currentServerAddress.empty() )
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        auto it = hiddenRouteClients.find ( channelId );
        if ( it != hiddenRouteClients.end() )
        {
            if ( it->second.core != nullptr )
            {
                jamulus_core_disconnect ( it->second.core );
                jamulus_core_stop ( it->second.core );
                jamulus_core_destroy ( it->second.core );
            }
            hiddenRouteClients.erase ( it );
        }
        return;
    }

    std::lock_guard<std::mutex> lock ( routeStateMutex );
    auto& hiddenClient = hiddenRouteClients[channelId];
    hiddenClient.routeCode = state.route;

    if ( hiddenClient.core == nullptr )
    {
        hiddenClient.core = jamulus_core_create ( 0, nullptr, profileName.c_str(), true );
        if ( hiddenClient.core == nullptr )
            return;

        syncSettingsToCore ( hiddenClient.core );
        jamulus_core_set_server_addr ( hiddenClient.core, currentServerAddress.c_str() );
        jamulus_core_start ( hiddenClient.core );
        hiddenClient.needsResampling = needsResampling;
        if ( hiddenClient.needsResampling )
        {
            hiddenClient.inputResampler.prepare ( hostSampleRate, JAMULUS_SAMPLE_RATE, 2 );
            hiddenClient.outputResampler.prepare ( JAMULUS_SAMPLE_RATE, hostSampleRate, 2 );
        }
    }

    const int numChannels = jamulus_core_get_num_channels ( hiddenClient.core );
    for ( int index = 0; index < numChannels; ++index )
    {
        jamulus_channel_info_t info{};
        if ( !jamulus_core_get_channel_info ( hiddenClient.core, index, &info ) )
            continue;

        const bool isTarget = ( info.id == channelId );
        const float gain = ( isTarget && !state.muted ) ? 1.0f : 0.0f;
        jamulus_core_set_channel_gain ( hiddenClient.core, info.id, gain );
        jamulus_core_set_channel_mute ( hiddenClient.core, info.id, !isTarget || state.muted );
        if ( isTarget )
            jamulus_core_set_channel_pan ( hiddenClient.core, info.id, state.pan );
    }
}

void JamulusPluginProcessor::syncAllHiddenRouteClients()
{
    std::vector<int> channelIds;
    {
        std::lock_guard<std::mutex> lock ( routeStateMutex );
        channelIds.reserve ( channelControlStates.size() );
        for ( const auto& [channelId, state] : channelControlStates )
            channelIds.push_back ( channelId );
    }

    for ( const int channelId : channelIds )
        syncHiddenRouteClient ( channelId );
}

void JamulusPluginProcessor::syncSettingsToCore ( jamulus_core_t targetCore ) const
{
    if ( targetCore == nullptr )
        return;

    jamulus_core_set_name ( targetCore, profileName.c_str() );
    jamulus_core_set_instrument ( targetCore, profileInstrument );
    jamulus_core_set_country ( targetCore, profileCountry );
    jamulus_core_set_city ( targetCore, profileCity.c_str() );
    jamulus_core_set_skill ( targetCore, profileSkill );
    jamulus_core_set_auto_jitter ( targetCore, autoJitterEnabled );
    jamulus_core_set_jitter_buffer ( targetCore, jitterBufferSize );
    jamulus_core_set_server_jitter_buffer ( targetCore, serverJitterBufferSize );
    jamulus_core_set_small_buffers ( targetCore, smallBuffersEnabled );
    jamulus_core_set_audio_channels ( targetCore, audioChannelsMode );
}

void JamulusPluginProcessor::clearAuxBusBuffers ( int numSamples )
{
    std::lock_guard<std::mutex> lock ( routeStateMutex );
    const size_t samples = static_cast<size_t> ( std::max ( 0, numSamples ) ) * 2u;
    for ( auto& buffer : auxBusBuffers )
        buffer.assign ( samples, 0.0f );
}

bool JamulusPluginProcessor::shouldMaintainHiddenClientForChannel ( int channelId ) const
{
    const auto stateIt = channelControlStates.find ( channelId );
    if ( stateIt != channelControlStates.end() )
    {
        const auto route = decodeOutputRoute ( stateIt->second.route );
        if ( !route.isMasterStereo )
            return true;
    }

    std::lock_guard<std::mutex> lock ( recordingMutex );
    if ( !recordingRuntime.active )
        return false;

    return std::find ( recordingRuntime.trackedChannelIds.begin(),
                       recordingRuntime.trackedChannelIds.end(),
                       channelId ) != recordingRuntime.trackedChannelIds.end();
}
