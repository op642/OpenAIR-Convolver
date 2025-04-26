///*
//  ==============================================================================
//
//    irWaveform.cpp
//    Created: 26 Apr 2025 2:18:32am
//    Author:  Oli Partridge
//
//  ==============================================================================
//*/
//#include "irWaveform.h"
//
//void IRWaveform::setIRData(const juce::AudioBuffer<float>& buffer)
//{
//    irBuffer = buffer;
//    repaint();
//}
//
//void IRWaveform::paint(juce::Graphics& g)
//{
//    g.fillAll(juce::Colours::black);
//    g.setColour(juce::Colours::white);
//
//    if (irBuffer.getNumSamples() > 0)
//    {
//        auto width = getWidth();
//        auto height = getHeight();
//        auto numSamples = irBuffer.getNumSamples();
//        auto* data = irBuffer.getReadPointer(0); // Assuming mono for simplicity
//
//        juce::Path waveformPath;
//        waveformPath.startNewSubPath(0, height / 2);
//
//        for (int i = 0; i < width; ++i)
//        {
//            auto sampleIndex = juce::jmap(i, 0, width, 0, numSamples);
//            auto amplitude = juce::jmap(data[sampleIndex], -1.0f, 1.0f, (float)height, 0.0f);
//            waveformPath.lineTo((float)i, amplitude);
//        }
//
//        g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
//    }
//    else
//    {
//        g.drawFittedText("No IR Loaded", getLocalBounds(), juce::Justification::centred, 1);
//    }
//}
