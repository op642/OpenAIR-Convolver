/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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
    juce::Image background;
    // void comboBoxChanged(juce::ComboBox* irSelectionBox) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OpenAIRConvolverAudioProcessor& audioProcessor;
    
    juce::TextButton loadIRButton;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
//    juce::ComboBox irSelectionBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenAIRConvolverAudioProcessorEditor)
};
