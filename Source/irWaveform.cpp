//// SpectrumAnalyzer.cpp
#include "irWaveform.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(fftOrder), fftData(fftSize * 2, 0.0f), scopeData(fftSize / 2, 0.0f)
{
    startTimerHz(30); // Update at 30 FPS
}

void SpectrumAnalyzer::setAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() < fftSize)
    {
        // Zero-pad the buffer if it's too small
        juce::AudioBuffer<float> paddedBuffer(buffer.getNumChannels(), fftSize);
        paddedBuffer.clear();
        paddedBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
        audioBuffer.makeCopyOf(paddedBuffer);
    }
    else
    {
        audioBuffer.makeCopyOf(buffer);
    }
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    auto width = getWidth();
    auto height = getHeight();

    // Draw logarithmic frequency grid
    g.setColour(juce::Colours::darkgrey);
    const std::vector<float> frequencies = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    for (auto freq : frequencies)
    {
        // Correctly map the frequency to the x position
        float x = std::log10(freq / 20.0f) / std::log10(20000.0f / 20.0f) * (float)width;
        g.drawLine(x, 0, x, height);

        // Draw frequency labels
        g.setColour(juce::Colours::white);
        g.setFont(10.0f); // Set smaller font size
        juce::String label = (freq >= 1000.0f)
            ? juce::String(freq / 1000.0f, 1) + "k"
            : juce::String((int)freq); // Remove "Hz" label
        g.drawText(label, x + 2, height - 15, 50, 15, juce::Justification::left);
    }

    // Draw amplitude grid
    g.setColour(juce::Colours::darkgrey);
    const std::vector<float> amplitudes = {+6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f};
    for (auto amp : amplitudes)
    {
        float y = juce::jmap(amp, -24.0f, +6.0f, (float)height, 0.0f);
        g.drawLine(0, y, width, y);

        // Draw amplitude labels
        g.setColour(juce::Colours::white);
        g.setFont(10.0f); // Set smaller font size
        g.drawText(juce::String((int)amp) + " dB", 2, y - 10, 50, 15, juce::Justification::left);
    }

    // Draw spectrum
    g.setColour(juce::Colours::green);
    for (size_t i = 1; i < scopeData.size(); ++i)
    {
        auto x1 = juce::jmap<float>(i - 1, 0, (int)scopeData.size(), 0, width);
        auto y1 = juce::jmap<float>(scopeData[i - 1], 0.0f, 1.0f, height, 0);
        auto x2 = juce::jmap<float>(i, 0, (int)scopeData.size(), 0, width);
        auto y2 = juce::jmap<float>(scopeData[i], 0.0f, 1.0f, height, 0);

        g.drawLine(x1, y1, x2, y2);
    }
}

void SpectrumAnalyzer::resized()
{
    // Handle resizing if needed
}

void SpectrumAnalyzer::timerCallback()
{
    performFFT();
    repaint();
}

void SpectrumAnalyzer::performFFT()
{
    if (audioBuffer.getNumSamples() < fftSize)
        return;

    auto* channelData = audioBuffer.getReadPointer(0);
    std::copy(channelData, channelData + fftSize, fftData.begin());

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    for (size_t i = 0; i < scopeData.size(); ++i)
    {
        scopeData[i] = juce::jmap(fftData[i], 0.0f, 1.0f);
    }
}
