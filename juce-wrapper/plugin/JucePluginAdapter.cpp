#include "JucePluginAdapter.h"
#include "PluginEditor.h"

JamulusAudioProcessor::JamulusAudioProcessor() = default;
JamulusAudioProcessor::~JamulusAudioProcessor() = default;

void JamulusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    impl.prepareToPlay(sampleRate, samplesPerBlock);
}

void JamulusAudioProcessor::releaseResources()
{
    impl.releaseResources();
}

void JamulusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Build arrays of pointers expected by JamulusPluginProcessor
    std::vector<const float*> inputs;
    std::vector<float*> outputs;
    inputs.reserve(numChannels);
    outputs.reserve(numChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        inputs.push_back(buffer.getReadPointer(ch));
        outputs.push_back(buffer.getWritePointer(ch));
    }

    impl.processBlock(inputs.data(), outputs.data(), numChannels, numSamples);
}

juce::AudioProcessorEditor* JamulusAudioProcessor::createEditor()
{
    return new JamulusPluginEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JamulusAudioProcessor();
}
