#include "JuceAdapter.h"

JamulusAudioProcessor::JamulusAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    impl = std::make_unique<JamulusPluginProcessor>();
}

JamulusAudioProcessor::~JamulusAudioProcessor() = default;

void JamulusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    impl->prepareToPlay(sampleRate, samplesPerBlock);
}

void JamulusAudioProcessor::releaseResources()
{
    impl->releaseResources();
}

void JamulusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Build arrays of pointers for adapter
    std::vector<const float*> inputs(numChannels);
    std::vector<float*> outputs(numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        inputs[ch] = buffer.getReadPointer(ch);
        outputs[ch] = buffer.getWritePointer(ch);
    }

    impl->processBlock(inputs.data(), outputs.data(), numChannels, numSamples);
}
