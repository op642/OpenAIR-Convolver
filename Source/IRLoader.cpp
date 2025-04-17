#include "IRLoader.h"

IRLoader::IRLoader(size_t threadPoolSize)
{
    for (size_t i = 0; i < threadPoolSize; ++i)
    {
        threadPool.emplace_back(&IRLoader::workerThread, this);
    }
}

IRLoader::~IRLoader()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : threadPool)
    {
        if (worker.joinable())
            worker.join();
    }
}

void IRLoader::loadMultichannelIRFile(const juce::File& irFile, double sampleRate, int numChannels)
{
    auto task = [this, irFile, sampleRate, numChannels]()
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
                std::lock_guard<std::mutex> lock(bufferMutex);
                bufferQueue.push(std::move(buffers));
            }
            bufferReady.store(true);
        }
    };

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(std::move(task));
    }
    condition.notify_one();
}

void IRLoader::processPendingBuffers(std::vector<std::unique_ptr<juce::dsp::Convolution>>& convolutions, double sampleRate)
{
    if (bufferReady.load())
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
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

bool IRLoader::isBufferReady() const
{
    return bufferReady.load();
}

void IRLoader::workerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() { return !taskQueue.empty() || stop; });
            if (stop && taskQueue.empty())
                return;
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }
        task();
    }
}
