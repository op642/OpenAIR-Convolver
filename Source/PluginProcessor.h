/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "IRLoader.h"


//==============================================================================
/**
*/

class OpenAIRConvolverAudioProcessor  : public juce::AudioProcessor
{
public:
    juce::File getRoot() const;
    void setRoot(const juce::File& newRoot);

    juce::File getSavedIRFile() const;
    void setSavedIRFile(const juce::File& newSavedIRFile);

    juce::dsp::Convolution& getConvolution();
    void setConvolution(const juce::dsp::Convolution& newConvolution);
    
    std::vector<juce::AudioBuffer<float>> loadMultichannelIRFile(const juce::File& irFile);
    
    const std::vector<std::unique_ptr<juce::dsp::Convolution>>& getConvolutions() const;
    
    void loadIRFile(const juce::File& irFile);
    
    const std::vector<float>& getFirstChannelIR() const;
    
private:
    juce::dsp::ProcessSpec spec;
    std::vector<std::unique_ptr<juce::dsp::Convolution>> convolutions;
    
    IRLoader irLoader;
    juce::dsp::Convolution::Convolution::NonUniform NUP;
    std::vector<float> firstChannelIR;
    
    
    
    
    /*==============================================================================
    ============================= JUCE Functions ===================================
    ==============================================================================*/
    
public:
    //==============================================================================
    OpenAIRConvolverAudioProcessor();
    ~OpenAIRConvolverAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenAIRConvolverAudioProcessor)
};
