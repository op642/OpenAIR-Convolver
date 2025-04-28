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
    irLoader(6) // 6 threads for 6 channels (5.1)
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
    if (!irFile.existsAsFile())
    {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "File Loading Error",
            "The selected IR file could not be found or loaded. Please check the file and try again."
        );
        return;
    }

    irLoader.loadBformatIRFile(irFile, getSampleRate(), getTotalNumOutputChannels());
}


//==============================================================================
// Prepare to Play
void OpenAIRConvolverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Ensure the number of channels matches the 5.1 surround format
    spec.numChannels = 6; // Explicitly set to 6 for 5.1 surround
    convolutions.resize(spec.numChannels);

    for (auto& conv : convolutions)
    {
        conv = std::make_unique<juce::dsp::Convolution>(NUP);
    }

    NUP.headSizeInSamples = 4096 * 4; // 16k seems to work well for 5.1

    const int expectedBlockSize = 512;
    if (samplesPerBlock != expectedBlockSize)
    {
        DBG("Warning: Block size changed to " << samplesPerBlock << ". Enforcing block size to " << expectedBlockSize);
        samplesPerBlock = expectedBlockSize; // Enforce the block size
    }

    // Prepare each convolution processor
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;

    for (auto& conv : convolutions)
    {
        conv->prepare(spec);
    }
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

    // Process each channel's convolution
    for (size_t i = 0; i < convolutions.size(); ++i)
    {
        if (i < totalNumOutputChannels && convolutions[i]->getCurrentIRSize() > 0)
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
