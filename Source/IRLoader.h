/*
  ==============================================================================

    IRLoader.h
    Created: 15 Apr 2025 10:38:10pm
    Author:  Oli Partridge

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <queue>
#include <mutex>

class IRLoader
{
public:
    void loadMultichannelIRFile(const juce::File& irFile, double sampleRate, int numChannels);
    void processPendingBuffers(std::vector<std::unique_ptr<juce::dsp::Convolution>>& convolutions, double sampleRate);
    bool isBufferReady() const;

private:
    std::queue<std::vector<juce::AudioBuffer<float>>> bufferQueue;
    std::mutex mutex;
    std::atomic<bool> bufferReady{false};
};
