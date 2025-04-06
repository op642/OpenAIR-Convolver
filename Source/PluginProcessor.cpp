#include "PluginProcessor.h"
#include "PluginEditor.h"

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
                       ),
    convolution(NUP)
#endif
{
    NUP.headSizeInSamples = 256; // Default head size for non-uniform convolution for long IRs
    DBG("NonUniform head size set to: " << NUP.headSizeInSamples);
}

OpenAIRConvolverAudioProcessor::~OpenAIRConvolverAudioProcessor() {}

//==============================================================================
// Prepare to Play
void OpenAIRConvolverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    convolution.prepare(spec);
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
        convolution.process(context);
    }
}

//==============================================================================
// Buses Layout Supported
bool OpenAIRConvolverAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return true;
}

//==============================================================================
// Plugin Information
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

//==============================================================================
// Programs
int OpenAIRConvolverAudioProcessor::getNumPrograms()
{
    return 1;
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

//==============================================================================
// State Information
void OpenAIRConvolverAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void OpenAIRConvolverAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

//==============================================================================
// Editor
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

//==============================================================================
// Getters and Setters

// Getter and Setter for root
juce::File OpenAIRConvolverAudioProcessor::getRoot() const {
    return root;
}

void OpenAIRConvolverAudioProcessor::setRoot(const juce::File& newRoot) {
    root = newRoot;
}

// Getter and Setter for savedIRFile
juce::File OpenAIRConvolverAudioProcessor::getSavedIRFile() const {
    return savedIRFile;
}

void OpenAIRConvolverAudioProcessor::setSavedIRFile(const juce::File& newSavedIRFile) {
    savedIRFile = newSavedIRFile;
}

// Getter and Setter for convolution
juce::dsp::Convolution& OpenAIRConvolverAudioProcessor::getConvolution() {
    return convolution;
}

void OpenAIRConvolverAudioProcessor::setConvolution(const juce::dsp::Convolution& newConvolution) {
    convolution.loadImpulseResponse(getSavedIRFile(), juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, 0);
}
