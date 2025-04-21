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
#include <thread>
#include <condition_variable>
#include <functional>

class IRLoader
{
public:
    IRLoader(size_t threadPoolSize);
    ~IRLoader();

    void loadMultichannelIRFile(const juce::File& irFile, double sampleRate, int numChannels);
    
    void processPendingBuffers(std::vector<std::unique_ptr<juce::dsp::Convolution>>& convolutions,
                               double sampleRate);
    bool isBufferReady() const;
    
    void decodeBFormatTo5Point1(const juce::AudioBuffer<float>& bFormatBuffer,
                                juce::AudioBuffer<float>& outputBuffer);

private:
    void workerThread();

    std::vector<std::thread> threadPool;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;

    std::queue<std::vector<juce::AudioBuffer<float>>> bufferQueue;
    std::mutex bufferMutex;
    std::atomic<bool> bufferReady{false};
};
