/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
//#include "IRWaveform.h"

//==============================================================================
/**
*/
class OpenAIRConvolverAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OpenAIRConvolverAudioProcessorEditor (OpenAIRConvolverAudioProcessor&);
    ~OpenAIRConvolverAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::ComboBox irSelectionBox;
    juce::Image background;
    juce::Image audiolab;
    juce::Image UOY;
//    IRWaveform irWaveform;
    OpenAIRConvolverAudioProcessor& audioProcessor;
    
    //juce::TextButton loadIRButton;
    
    //std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenAIRConvolverAudioProcessorEditor)
};
