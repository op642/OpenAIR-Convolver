/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================


OpenAIRConvolverAudioProcessorEditor::~OpenAIRConvolverAudioProcessorEditor()
{
}

//==============================================================================
void OpenAIRConvolverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (30.0f));
    g.drawFittedText ("OpenAIR Convolver", getLocalBounds(), juce::Justification::centred, 1);
}

OpenAIRConvolverAudioProcessorEditor::OpenAIRConvolverAudioProcessorEditor (OpenAIRConvolverAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible(loadIRButton);
    loadIRButton.setButtonText("Load IR");
    loadIRButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Select an IR file", audioProcessor.getRoot(), "*");

        const auto fileChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser)
        {
            juce::File file = chooser.getResult();
            if (file.existsAsFile())
            {
                audioProcessor.loadIRFile(file);
//                audioProcessor.preloadIRFile(file); // Preload the IR file
            }
        });
    };
    setSize (400, 300);

}




void OpenAIRConvolverAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    const auto btnX = getWidth() * (0.21);
    const auto btnY = getHeight() * (0.6);
    const auto btnWidth = getWidth() * (0.59);
    const auto btnHeight = getHeight() * (0.1); //JUCE_LIVE_CONSTANT
    
    loadIRButton.setBounds(btnX, btnY, btnWidth, btnHeight);
}
