/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Helper function to convert B-format IR to stereo
void convertBFormatToStereo(const juce::AudioBuffer<float>& bFormatBuffer, juce::AudioBuffer<float>& stereoBuffer)
{
    jassert(bFormatBuffer.getNumChannels() >= 3); // Ensure there are at least W, X, and Y channels

    int numSamples = bFormatBuffer.getNumSamples();
    stereoBuffer.setSize(2, numSamples); // Set stereo buffer to 2 channels

    auto* wChannel = bFormatBuffer.getReadPointer(0);
    auto* xChannel = bFormatBuffer.getReadPointer(1);
    auto* yChannel = bFormatBuffer.getReadPointer(2);

    auto* leftChannel = stereoBuffer.getWritePointer(0);
    auto* rightChannel = stereoBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = 0.707f * (wChannel[i] + xChannel[i] - yChannel[i]);
        rightChannel[i] = 0.707f * (wChannel[i] - xChannel[i] + yChannel[i]);
    }
}

bool OpenAIRConvolverAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Add your implementation here
    return true;
}

const juce::String OpenAIRConvolverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OpenAIRConvolverAudioProcessor::acceptsMidi() const
{
    return JucePlugin_WantsMidiInput;
}

bool OpenAIRConvolverAudioProcessor::producesMidi() const
{
    return JucePlugin_ProducesMidiOutput;
}

bool OpenAIRConvolverAudioProcessor::isMidiEffect() const
{
    return JucePlugin_IsMidiEffect;
}

double OpenAIRConvolverAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OpenAIRConvolverAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int OpenAIRConvolverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OpenAIRConvolverAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String OpenAIRConvolverAudioProcessor::getProgramName(int index)
{
    return {};
}

void OpenAIRConvolverAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void OpenAIRConvolverAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Add your implementation here
}

void OpenAIRConvolverAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Add your implementation here
}


//==============================================================================
// Constructor and Destructor
OpenAIRConvolverAudioProcessor::OpenAIRConvolverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

OpenAIRConvolverAudioProcessor::~OpenAIRConvolverAudioProcessor()
{
}

//==============================================================================
// Prepare to Play
void OpenAIRConvolverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();

    convolution.reset();
    convolution.prepare(processSpec);
}

// Release Resources
void OpenAIRConvolverAudioProcessor::releaseResources()
{
    convolution.reset();
}

//==============================================================================
// Process Block
void OpenAIRConvolverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    
    if (convolution.getCurrentIRSize() > 0){
        juce::dsp::ProcessContextReplacing<float> context(block);
    }
//    convolution.process(context);
}

bool OpenAIRConvolverAudioProcessor::hasEditor() const
{
    return true;
}
juce::AudioProcessorEditor* OpenAIRConvolverAudioProcessor::createEditor()
{
    return new OpenAIRConvolverAudioProcessorEditor (*this);
}

//==============================================================================
// Create Plugin Filter
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenAIRConvolverAudioProcessor();
}
