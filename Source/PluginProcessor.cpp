#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IRLoader.h"

/* OpenAIR Convolver
 Oliver Partridge
 21/04/2025
 
This Program produces a 5.1 surround sound convolver plugin using JUCE.
IRLoader is a custom class that loads and decodes bformat IR files to 5.1 surround.

Non-uniform Partitioned Convolution is implemented through the JUCE DSP module.
*/


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
    irLoader(6) // potentially change to match output channels 6ch = 5.1
{
    //Empty constructor
}

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
    
    return mainInput == mainInput && mainOutput == mainOutput;

//    return mainInput == juce::AudioChannelSet::create5point1()
//        && mainOutput == juce::AudioChannelSet::create5point1();
}

void OpenAIRConvolverAudioProcessor::loadIRFile(const juce::File& irFile)
{
    irLoader.loadBformatIRFile(irFile, getSampleRate(), getTotalNumOutputChannels());
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
    NUP.headSizeInSamples = 4096*4; // 16k seems to work well for 5.1

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

//     Debugging: Measure processing time
//     auto startTime = juce::Time::getMillisecondCounter();

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

//     auto endTime = juce::Time::getMillisecondCounter();
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
