#include "PluginProcessor.h"
#include "PluginEditor.h"

std::vector<juce::dsp::Convolution> convolutions;

//==============================================================================
// Constructor and Destructor
OpenAIRConvolverAudioProcessor::OpenAIRConvolverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::ambisonic(1), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::create5point1(), true)
                     #endif
                       ),
    convolution(NUP)
//    convolutions(5)
#endif
{
    NUP.headSizeInSamples = 256; // Default head size for non-uniform convolution for long IRs
    DBG("NonUniform head size set to: " << NUP.headSizeInSamples);
    
    // Load a default file
    juce::File defaultFile("/Users/olipartridge/Programming/OpenAIR-Convolver/Assets/Audio/IR/Raw_IR/York_Minster_bformat_48k.wav");
    if (defaultFile.existsAsFile())
    {
        loadAndDecodeBFormatFile(defaultFile);
    }
    else
    {
        DBG("Default file does not exist: " << defaultFile.getFullPathName());
    }
    
//    for (auto& conv : convolutions)
//    {
//        conv.loadImpulseResponse(getSavedIRFile(), juce::dsp::Convolution::Stereo::no,
//                                 juce::dsp::Convolution::Trim::yes, 0);
//    }
}
/*
 Xcode is probably shitting itself because of the file is 4 channels and convolution only supports 2 channels,
 the channels need to be split up and processed independantly and then recombined.
 */

OpenAIRConvolverAudioProcessor::~OpenAIRConvolverAudioProcessor() {}

//==============================================================================
// Prepare to Play
void OpenAIRConvolverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    for (auto& conv : convolutions)
    {
        convolution.prepare(spec);
    }
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

    // Process each channel independently as mono
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        juce::dsp::AudioBlock<float> channelBlock = block.getSingleChannelBlock(channel);
        juce::dsp::ProcessContextReplacing<float> context(channelBlock);
        convolutions[channel].process(context);
    }
}


//==============================================================================
// Buses Layout Supported
bool OpenAIRConvolverAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainInput == juce::AudioChannelSet::ambisonic(1)
        && mainOutput == juce::AudioChannelSet::create5point1();
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

//==============================================================================
void OpenAIRConvolverAudioProcessor::decodeBFormatTo5Point1(const juce::File& bFormatFile, juce::AudioBuffer<float>& outputBuffer)
{
    juce::AudioBuffer<float> bFormatBuffer;
    loadBFormatFile(bFormatFile, bFormatBuffer);

    // Ensure the B-Format buffer has 4 channels (W, X, Y, Z)
    jassert(bFormatBuffer.getNumChannels() == 4);

    // Ensure the output buffer has 6 channels (5.1 format: L, R, C, LFE, Ls, Rs)
    jassert(outputBuffer.getNumChannels() == 6);

    auto numSamples = bFormatBuffer.getNumSamples();

    // Get pointers to the B-Format channels
    auto* W = bFormatBuffer.getReadPointer(0);
    auto* X = bFormatBuffer.getReadPointer(1);
    auto* Y = bFormatBuffer.getReadPointer(2);
    auto* Z = bFormatBuffer.getReadPointer(3);

    // Get pointers to the output channels
    auto* L = outputBuffer.getWritePointer(0);
    auto* R = outputBuffer.getWritePointer(1);
    auto* C = outputBuffer.getWritePointer(2);
    auto* LFE = outputBuffer.getWritePointer(3);
    auto* Ls = outputBuffer.getWritePointer(4);
    auto* Rs = outputBuffer.getWritePointer(5);

    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = 0.707f * (W[i] + X[i]);
        R[i] = 0.707f * (W[i] - X[i]);
        C[i] = W[i];
        LFE[i] = Z[i];
        Ls[i] = 0.707f * (W[i] + Y[i]);
        Rs[i] = 0.707f * (W[i] - Y[i]);
    }
}

void OpenAIRConvolverAudioProcessor::loadBFormatFile(const juce::File& bFormatFile, juce::AudioBuffer<float>& bFormatBuffer)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(bFormatFile));
    if (reader != nullptr)
    {
        bFormatBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&bFormatBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
    }
}

void OpenAIRConvolverAudioProcessor::loadAndDecodeBFormatFile(const juce::File& bFormatFile)
{
    if (!bFormatFile.existsAsFile())
    {
        DBG("File does not exist: " << bFormatFile.getFullPathName());
        return;
    }

    juce::AudioBuffer<float> outputBuffer;
    outputBuffer.setSize(6, 0); // 5.1 format has 6 channels

    decodeBFormatTo5Point1(bFormatFile, outputBuffer);

    // Process the decoded buffer as needed
    // For example, you might want to store it or use it in further processing

    // Set the saved IR file and root directory
    setSavedIRFile(bFormatFile);
    setRoot(bFormatFile.getParentDirectory());

    // Reset and load the convolution processor
    getConvolution().reset();
    getConvolution().loadImpulseResponse(bFormatFile, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
}
