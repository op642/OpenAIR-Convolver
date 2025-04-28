// SpectrumAnalyzer.h
#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer();
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void performFFT();

    static constexpr int fftOrder = 11; // FFT size = 2^11 = 2048
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft;
    juce::AudioBuffer<float> audioBuffer;
    std::vector<float> fftData;
    std::vector<float> scopeData;
};
//};
