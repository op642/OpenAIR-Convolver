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
class OpenAIR_ConvolverAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OpenAIR_ConvolverAudioProcessorEditor (OpenAIR_ConvolverAudioProcessor&);
    ~OpenAIR_ConvolverAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OpenAIR_ConvolverAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenAIR_ConvolverAudioProcessorEditor)
};
