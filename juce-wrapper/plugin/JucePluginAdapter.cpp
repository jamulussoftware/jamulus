#include "JucePluginAdapter.h"
#include "PluginEditor.h"
#include "DebugLogger.h"

JamulusAudioProcessor::~JamulusAudioProcessor() = default;

void JamulusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    DebugLogger::instance().log("[JamulusAudioProcessor] prepareToPlay called");
    impl.prepareToPlay(sampleRate, samplesPerBlock);
}

void JamulusAudioProcessor::releaseResources()
{
    DebugLogger::instance().log("[JamulusAudioProcessor] releaseResources called");
    impl.releaseResources();
}

void JamulusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    DebugLogger::instance().log("[JamulusAudioProcessor] processBlock called");
    const int numSamples = buffer.getNumSamples();
    
    // Determine which input to use
    juce::AudioBuffer<float>* inputBuffer = &buffer;
    juce::AudioBuffer<float> sidechainBuffer;
    
    // Check if sidechain is enabled and available
    auto* sidechainBus = getBus(true, 1);  // Input bus index 1 = sidechain
    bool hasSidechain = sidechainBus != nullptr && sidechainBus->isEnabled();
    
    if (useSidechain && hasSidechain)
    {
        // Get sidechain audio from the buffer (comes after main input channels)
        int mainInputChannels = getMainBusNumInputChannels();
        int sidechainChannels = sidechainBus->getNumberOfChannels();
        
        if (sidechainChannels > 0 && buffer.getNumChannels() >= mainInputChannels + sidechainChannels)
        {
            // Create a buffer pointing to sidechain channels
            sidechainBuffer.setSize(2, numSamples, false, false, true);
            
            // Copy sidechain channels (handle mono->stereo if needed)
            if (sidechainChannels == 1)
            {
                // Mono sidechain - duplicate to both channels
                sidechainBuffer.copyFrom(0, 0, buffer, mainInputChannels, 0, numSamples);
                sidechainBuffer.copyFrom(1, 0, buffer, mainInputChannels, 0, numSamples);
            }
            else
            {
                // Stereo sidechain
                sidechainBuffer.copyFrom(0, 0, buffer, mainInputChannels, 0, numSamples);
                sidechainBuffer.copyFrom(1, 0, buffer, mainInputChannels + 1, 0, numSamples);
            }
            
            inputBuffer = &sidechainBuffer;
        }
    }
    
    // Build arrays of pointers
    const int numChannels = 2;  // Always process stereo
    std::vector<const float*> inputs;
    std::vector<float*> outputs;
    inputs.reserve(numChannels);
    outputs.reserve(numChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch < inputBuffer->getNumChannels())
            inputs.push_back(inputBuffer->getReadPointer(ch));
        else
            inputs.push_back(nullptr);
        
        if (ch < buffer.getNumChannels())
            outputs.push_back(buffer.getWritePointer(ch));
        else
            outputs.push_back(nullptr);
    }

    impl.processBlock(inputs.data(), outputs.data(), numChannels, numSamples);
}

juce::AudioProcessorEditor* JamulusAudioProcessor::createEditor()
{
    return new JamulusPluginEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    DebugLogger::instance().log("[JucePluginAdapter] createPluginFilter called");
    return new JamulusAudioProcessor();
}
