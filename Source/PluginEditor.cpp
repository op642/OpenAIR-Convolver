/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>

// IR File Selection Options
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
void OpenAIRConvolverAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    // background images
    background = juce::ImageCache::getFromMemory(BinaryData::OpenAir_logo_png, BinaryData::OpenAir_logo_pngSize);
    g.drawImageWithin(background, 25, 10, 0.4 * getWidth(), 0.2 * getHeight(), juce::RectanglePlacement::fillDestination, false);
    audiolab = juce::ImageCache::getFromMemory(BinaryData::AudioLab_png, BinaryData::AudioLab_pngSize);
    g.drawImageWithin(audiolab, 7.86258 * 25, 2.36349 * 10, 0.166207 * getWidth(), 0.0864271 * getHeight(), juce::RectanglePlacement::fillDestination, false);
    UOY = juce::ImageCache::getFromMemory(BinaryData::UOYLogo_png, BinaryData::UOYLogo_pngSize);
    g.drawImageWithin(UOY, 1.17293 * 250, 0.070561 * 200, 0.2 * getWidth(), 0.2 * getHeight(), juce::RectanglePlacement::fillDestination, false);

// plot area
    g.setColour(juce::Colours::darkgrey);
    auto plotArea = getLocalBounds().removeFromBottom(getHeight() / 2).reduced(10);
    plotArea = plotArea.translated(0, -20);
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(plotArea);

    // IR waveform plot
    g.setColour(juce::Colours::white);
    if (!irData.empty())
    {
        auto numSamples = irData.size();
        auto sampleRate = audioProcessor.getSampleRate();
        auto durationInSeconds = static_cast<int>(std::ceil(static_cast<float>(numSamples) / sampleRate));

        juce::Path waveformPath;
        waveformPath.startNewSubPath(plotArea.getX(), plotArea.getCentreY());

        for (size_t i = 0; i < numSamples; ++i)
        {
            auto x = juce::jmap<float>(i, 0, numSamples, plotArea.getX(), plotArea.getRight());
            auto y = juce::jmap<float>(irData[i], -1.0f, 1.0f, plotArea.getBottom(), plotArea.getY());
            waveformPath.lineTo(x, y);
        }

        g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

        // Draw time axis
        g.setColour(juce::Colours::lightgrey);
        g.drawLine(plotArea.getX(), plotArea.getBottom(), plotArea.getRight(), plotArea.getBottom());

        for (int i = 0; i <= durationInSeconds; ++i)
        {
            int x = juce::jmap<float>(i, 0.0f, static_cast<float>(durationInSeconds), plotArea.getX(), plotArea.getRight());

            // Draw tick
            g.drawLine(x, plotArea.getBottom(), x, plotArea.getBottom() + 5);

            // Draw label
            g.drawText(juce::String(i) + "s", x - 15, plotArea.getBottom() + 5, 30, 15, juce::Justification::centred);
        }
    }
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
                loadTempIRFile(file);
            }
        }
        else
        {
            DBG("Invalid or unhandled selection");
        }
    };

    setSize(400, 300);
}

void OpenAIRConvolverAudioProcessorEditor::loadTempIRFile(const juce::File& tempFile)
{
    // Run the file loading in a background thread
    juce::Thread::launch([this, tempFile]() {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));
        if (reader != nullptr)
        {
            juce::AudioBuffer<float> tempBuffer;
            tempBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

            // Store the first channel's data for rendering
            std::vector<float> newIRData(tempBuffer.getNumSamples());
            std::copy(tempBuffer.getReadPointer(0),
                      tempBuffer.getReadPointer(0) + tempBuffer.getNumSamples(),
                      newIRData.begin());

            // Update the IR data on the message thread
            juce::MessageManager::callAsync([this, newIRData = std::move(newIRData)]() mutable {
                irData = std::move(newIRData);
                repaint();
            });
        }
    });
}

void OpenAIRConvolverAudioProcessorEditor::resized()
{
    const auto btnX = getWidth() * 0.21;
    const auto btnY = getHeight() * 0.27;
    const auto btnWidth = getWidth() * 0.59;
    const auto btnHeight = getHeight() * 0.1;

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
