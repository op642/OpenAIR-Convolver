#include "IRLoader.h"

void IRLoader::loadMultichannelIRFile(const juce::File& irFile, double sampleRate, int numChannels)
{
    std::thread([this, irFile, sampleRate, numChannels]()
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
        if (reader != nullptr && reader->numChannels >= numChannels)
        {
            std::vector<juce::AudioBuffer<float>> buffers(numChannels);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                buffers[channel].setSize(1, (int)reader->lengthInSamples);
                reader->read(&buffers[channel], 0, (int)reader->lengthInSamples, channel, true, true);
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                bufferQueue.push(std::move(buffers));
            }
            bufferReady.store(true);
        }
    }).detach();
}

void IRLoader::processPendingBuffers(std::vector<std::unique_ptr<juce::dsp::Convolution>>& convolutions, double sampleRate)
{
    if (bufferReady.load())
    {
        std::lock_guard<std::mutex> lock(mutex);
        while (!bufferQueue.empty())
        {
            auto buffers = std::move(bufferQueue.front());
            bufferQueue.pop();

            for (size_t i = 0; i < convolutions.size(); ++i)
            {
                if (i < buffers.size())
                {
                    convolutions[i]->loadImpulseResponse(std::move(buffers[i]),
                                                         sampleRate,
                                                         juce::dsp::Convolution::Stereo::no,
                                                         juce::dsp::Convolution::Trim::yes,
                                                         juce::dsp::Convolution::Normalise::yes);
                }
            }
        }
        bufferReady.store(false);
    }
}
