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
    // Find a way to load file
    background = juce::ImageCache::getFromMemory(BinaryData::OpenAir_logo_png, BinaryData::OpenAir_logo_pngSize);
    g.drawImageWithin(background, 15, 30, 0.95*getWidth(), 0.2*getHeight(), juce::RectanglePlacement::fillDestination, false);
    

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (30.0f));
    //g.drawFittedText ("OpenAIR Convolver", getLocalBounds(), juce::Justification::centred, 1);
}

OpenAIRConvolverAudioProcessorEditor::OpenAIRConvolverAudioProcessorEditor (OpenAIRConvolverAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible(loadIRButton);
    loadIRButton.setButtonText("Load B-Format IR");
    loadIRButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Select a B-Format IR file", audioProcessor.getRoot(), "*");

        const auto fileChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser)
        {
            juce::File file = chooser.getResult();
            if (file.existsAsFile())
            {
                audioProcessor.loadIRFile(file);
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
    const auto btnY = getHeight() * (0.45);
    const auto btnWidth = getWidth() * (0.59);
    const auto btnHeight = (0.1) * getHeight(); //JUCE_LIVE_CONSTANT
    
    loadIRButton.setBounds(btnX, btnY, btnWidth, btnHeight);
}
