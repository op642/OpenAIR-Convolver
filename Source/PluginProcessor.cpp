#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IRLoader.h"

//==============================================================================
// Constructor and Destructor
OpenAIRConvolverAudioProcessor::OpenAIRConvolverAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::create5point1(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::create5point1(), true)
                     #endif
                       ),
    irLoader(6) // potentially change to match output channels
{
    //Empty constructor
}
/*
 Xcode is probably shitting itself because of the file is 4 channels and convolution only supports 2 channels,
 the channels need to be split up and processed independantly and then recombined.
 
 INPUT SHOULD BE MONO, BUT SHOULD APPLY TO ALL OUTPUT CHANNELS.
 */

OpenAIRConvolverAudioProcessor::~OpenAIRConvolverAudioProcessor()
{
}

//==============================================================================
// Buses Layout Supported
bool OpenAIRConvolverAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Check if the input is mono and the output is 5.1 surround
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    return mainInput == juce::AudioChannelSet::create5point1()
        && mainOutput == juce::AudioChannelSet::create5point1();
}

void OpenAIRConvolverAudioProcessor::loadIRFile(const juce::File& irFile)
{
    irLoader.loadMultichannelIRFile(irFile, getSampleRate(), getTotalNumOutputChannels());
}


//==============================================================================
// Prepare to Play
void OpenAIRConvolverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize the convolutions vector with 6 convolution processors for 5.1 output
    convolutions.resize(spec.numChannels);
    for (auto& conv : convolutions)
    {
        conv = std::make_unique<juce::dsp::Convolution>(NUP); //NUP
    }
    NUP.headSizeInSamples = 4096*4;
    // PLAY AOUND WITH NUP AND HEADSIZE

    // Prepare each convolution processor
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    DBG("block size in samples: " << samplesPerBlock);

    for (auto& conv : convolutions)
    {
        conv->prepare(spec);
    }

    // Initialize monoIRBuffers for each channel
    monoIRBuffers.resize(spec.numChannels);
    for (auto& buffer : monoIRBuffers)
    {
        buffer.setSize(1, samplesPerBlock); // Mono buffer for each channel
    }
    
    // Initialize temporary buffers
    decodedIRBuffer.setSize(spec.numChannels, samplesPerBlock);
}

// Release Resources
void OpenAIRConvolverAudioProcessor::releaseResources()
{
    for (auto& conv : convolutions)
    {
        conv->reset();
    }
}

//==============================================================================
// Process Block
void OpenAIRConvolverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Check if new IRs need to be processed
    if (irLoader.isBufferReady())
    {
        irLoader.processPendingBuffers(convolutions, getSampleRate());
    }

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);

    // Debugging: Measure processing time
    // auto startTime = juce::Time::getMillisecondCounter();

    // Process each channel's convolution
    for (size_t i = 0; i < convolutions.size(); ++i)
    {
        if (i < block.getNumChannels() && convolutions[i]->getCurrentIRSize() > 0)
        {
            auto channelBlock = block.getSingleChannelBlock((int)i);
            juce::dsp::ProcessContextReplacing<float> context(channelBlock);
            convolutions[i]->process(context);
        }
    }

    // auto endTime = juce::Time::getMillisecondCounter();
//    float procTime = endTime-startTime;
//    DBG("Convolution processing time: " << (procTime) << " ms");
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

// Getter for convolutions
const std::vector<std::unique_ptr<juce::dsp::Convolution>>& OpenAIRConvolverAudioProcessor::getConvolutions() const {
    return convolutions;
}


// *************************************************
// ************* MIGHT REPLACE IRLOADER ************
// *************************************************
// might be better to use IRLoader as it is asynchronus
// just need to find what is causing spikes

//void OpenAIRConvolverAudioProcessor::preloadIRFile(const juce::File& irFile)
//{
//    juce::AudioFormatManager formatManager;
//    formatManager.registerBasicFormats();
//
//    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
//    if (reader != nullptr)
//    {
//        std::vector<juce::AudioBuffer<float>> buffers(getTotalNumOutputChannels());
//
//        for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel)
//        {
//            buffers[channel].setSize(1, (int)reader->lengthInSamples);
//            reader->read(&buffers[channel], 0, (int)reader->lengthInSamples, channel, true, true);
//        }
//
//        for (size_t i = 0; i < convolutions.size(); ++i)
//        {
//            if (i < buffers.size())
//            {
//                convolutions[i]->loadImpulseResponse(std::move(buffers[i]),
//                                                     getSampleRate(),
//                                                     juce::dsp::Convolution::Stereo::no,
//                                                     juce::dsp::Convolution::Trim::yes,
//                                                     juce::dsp::Convolution::Normalise::yes);
//            }
//        }
//    }
//    else
//    {
//        DBG("Failed to preload IR file: " << irFile.getFullPathName());
//    }
//}

// ***********************
// MULTICHANNEL IR LOADING
// ***********************

//std::vector<juce::AudioBuffer<float>> OpenAIRConvolverAudioProcessor::loadMultichannelIRFile(const juce::File& irFile)
//{
//    if (!irFile.existsAsFile())
//    {
//        DBG("File does not exist: " << irFile.getFullPathName());
//        return monoIRBuffers;
//    }
//
//    juce::AudioFormatManager formatManager;
//    formatManager.registerBasicFormats();
//
//    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
//    if (reader == nullptr)
//    {
//        DBG("Failed to create reader for file: " << irFile.getFullPathName());
//        return monoIRBuffers;
//    }
//
//    // Ensure the file has the expected number of channels
//    if (reader->numChannels < 1)
//    {
//        DBG("The IR file must have at least one channel.");
//        return monoIRBuffers;
//    }
//
//    // Load the IR file into a buffer
//    juce::AudioBuffer<float> irBuffer;
//    // sound be 6 channels, but set dynamically here
//    irBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
//    reader->read(&irBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
//
//    // Process each channel individually
//    for (int channel = 0; channel < reader->numChannels; ++channel)
//    {
//        juce::AudioBuffer<float> monoIRBuffer;
//        monoIRBuffer.setSize(1, irBuffer.getNumSamples());
//        monoIRBuffer.copyFrom(0, 0, irBuffer, channel, 0, irBuffer.getNumSamples());
//        monoIRBuffers.push_back(std::move(monoIRBuffer));
//    }
//
//    DBG("Loaded multichannel IR file: " << irFile.getFullPathName());
//    return monoIRBuffers;
//}



// *****************************************************************************
// ******************************* BFormat *************************************
// *****************************************************************************

//void OpenAIRConvolverAudioProcessor::decodeBFormatTo5Point1(const juce::File& bFormatFile, juce::AudioBuffer<float>& outputBuffer)
//{
//    juce::AudioBuffer<float> bFormatBuffer;
//    loadBFormatFile(bFormatFile, bFormatBuffer);
//
//    // Ensure the B-Format buffer has 4 channels (W, X, Y, Z)
//    jassert(bFormatBuffer.getNumChannels() == 4);
//
//    // Ensure the output buffer has 6 channels (5.1 format: L, R, C, LFE, Ls, Rs)
//    jassert(outputBuffer.getNumChannels() == 6);
//
//    auto numSamples = bFormatBuffer.getNumSamples();
//
//    // Get pointers to the B-Format channels
//    auto* W = bFormatBuffer.getReadPointer(0);
//    auto* X = bFormatBuffer.getReadPointer(1);
//    auto* Y = bFormatBuffer.getReadPointer(2);
//    auto* Z = bFormatBuffer.getReadPointer(3);
//
//    // Get pointers to the output channels
//    auto* L = outputBuffer.getWritePointer(0);
//    auto* R = outputBuffer.getWritePointer(1);
//    auto* C = outputBuffer.getWritePointer(2);
//    auto* LFE = outputBuffer.getWritePointer(3);
//    auto* Ls = outputBuffer.getWritePointer(4);
//    auto* Rs = outputBuffer.getWritePointer(5);
//
//    for (int i = 0; i < numSamples; ++i)
//    {
//        L[i] = 0.707f * (W[i] + X[i]);
//        R[i] = 0.707f * (W[i] - X[i]);
//        C[i] = W[i];
//        LFE[i] = Z[i];
//        Ls[i] = 0.707f * (W[i] + Y[i]);
//        Rs[i] = 0.707f * (W[i] - Y[i]);
//    }
//}
//
//void OpenAIRConvolverAudioProcessor::loadBFormatFile(const juce::File& bFormatFile, juce::AudioBuffer<float>& bFormatBuffer)
//{
//    juce::AudioFormatManager formatManager;
//    formatManager.registerBasicFormats();
//
//    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(bFormatFile));
//    if (reader != nullptr)
//    {
//        bFormatBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
//        reader->read(&bFormatBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
//    }
//}
//
//void OpenAIRConvolverAudioProcessor::loadAndDecodeBFormatFile(const juce::File& bFormatFile)
//{
//    if (!bFormatFile.existsAsFile())
//    {
//        DBG("File does not exist: " << bFormatFile.getFullPathName());
//        return;
//    }
//
//    juce::AudioBuffer<float> outputBuffer;
//    outputBuffer.setSize(6, 0); // 5.1 format has 6 channels
//
//    decodeBFormatTo5Point1(bFormatFile, outputBuffer);
//
//    decodedIRBuffer = outputBuffer;
//
//    // Set the saved IR file and root directory
//    setSavedIRFile(bFormatFile);
//    setRoot(bFormatFile.getParentDirectory());
//
//    // Reset and load the convolution processors
//    for (int channel = 0; channel < convolutions.size(); ++channel)
//    {
//        convolutions[channel]->reset();
//        juce::AudioBuffer<float> monoIRBuffer(1, decodedIRBuffer.getNumSamples());
//        monoIRBuffer.copyFrom(0, 0, decodedIRBuffer, channel, 0, decodedIRBuffer.getNumSamples());
//
//        convolutions[channel]->loadImpulseResponse(bFormatFile, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
//    }
//}



// *****************************************************************************
// ************************* JUCE SETTINGS *************************************
// *****************************************************************************
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
