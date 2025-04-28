/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>

const std::map<int, std::pair<const char*, size_t>> irFileMap = {
    {1, {BinaryData::York_Minster_bformat_48k_wav, BinaryData::York_Minster_bformat_48k_wavSize}},
    {2, {BinaryData::Usina_bformat_48_wav, BinaryData::Usina_bformat_48_wavSize}},
    {3, {nullptr, 0}} // Placeholder for other cases
};

//==============================================================================

OpenAIRConvolverAudioProcessorEditor::~OpenAIRConvolverAudioProcessorEditor()
{
}

//==============================================================================
void OpenAIRConvolverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    background = juce::ImageCache::getFromMemory(BinaryData::OpenAir_logo_png, BinaryData::OpenAir_logo_pngSize);
    g.drawImageWithin(background, 15, 30, 0.95*getWidth(), 0.2*getHeight(), juce::RectanglePlacement::fillDestination, false);
    audiolab = juce::ImageCache::getFromMemory(BinaryData::AudioLab_png,
                                               BinaryData::AudioLab_pngSize);
    g.drawImageWithin(audiolab, 0.759979*(15), 1.19*200, 0.114773*getWidth(), 0.2*getHeight(), juce::RectanglePlacement::fillDestination, false);
    UOY = juce::ImageCache::getFromMemory(BinaryData::UOYLogo_png,
                                               BinaryData::UOYLogo_pngSize);
    g.drawImageWithin(UOY, 1.17293*250,1.23372*200, 0.2*getWidth(), 0.2*getHeight(), juce::RectanglePlacement::fillDestination, false);
}

OpenAIRConvolverAudioProcessorEditor::OpenAIRConvolverAudioProcessorEditor(OpenAIRConvolverAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    addAndMakeVisible(irSelectionBox);

    irSelectionBox.addItem("York Minster", 1);
    irSelectionBox.addItem("Usina del arte", 2);
    irSelectionBox.addItem("Another", 3);
    irSelectionBox.setTextWhenNothingSelected("Select An Impulse Response");

    irSelectionBox.onChange = [this] {
        auto selectedId = irSelectionBox.getSelectedId();
        auto it = irFileMap.find(selectedId);

        if (it != irFileMap.end() && it->second.first != nullptr)
        {
            auto file = juce::File::createTempFile("TempIR.wav");
            file.replaceWithData(it->second.first, it->second.second);
            if (file.existsAsFile())
            {
                audioProcessor.loadIRFile(file);
            }
        }
        else
        {
            DBG("Invalid or unhandled selection");
        }
    };

    setSize(400, 300);
}

void OpenAIRConvolverAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    const auto btnX = getWidth() * (0.21);
    const auto btnY = getHeight() * (0.45);
    const auto btnWidth = getWidth() * (0.59);
    const auto btnHeight = (0.1) * getHeight(); //JUCE_LIVE_CONSTANT
    
    irSelectionBox.setBounds(btnX, btnY, btnWidth, btnHeight);
    //loadIRButton.setBounds(btnX, btnY, btnWidth, btnHeight);
}


//OpenAIRConvolverAudioProcessorEditor::OpenAIRConvolverAudioProcessorEditor(OpenAIRConvolverAudioProcessor& p)
//    : AudioProcessorEditor(&p), audioProcessor(p)
//{
// THIS CODE CAN BE USED TO ADD A FILE CHOOSER FOR USERS TO LOAD PERSONAL IR'S

//    addAndMakeVisible(loadIRButton);
//    loadIRButton.setButtonText("Load B-Format IR");
//    loadIRButton.onClick = [this] {
//        fileChooser = std::make_unique<juce::FileChooser>("Select a B-Format IR file", audioProcessor.getRoot(), "*");
//
//        const auto fileChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
//
//        fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser)
//        {
//            juce::File file = chooser.getResult();
//            if (file.existsAsFile())
//            {
//                audioProcessor.loadIRFile(file);
//            }
//        }
//        );
//    };
//    setSize (400, 300);
//
//}
