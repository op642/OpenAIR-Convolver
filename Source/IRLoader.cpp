#include "IRLoader.h"

IRLoader::IRLoader(size_t threadPoolSize) // thread pool initialised in plugin processor constructor
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

void IRLoader::loadBformatIRFile(const juce::File& irFile, double sampleRate, int numChannels)
{
    auto task = [this, irFile, sampleRate]()
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
        if (reader != nullptr)
        {
            DBG("IR File Channels: " << int(reader->numChannels));
            DBG("IR File Length: " << reader->lengthInSamples);

            if (reader->numChannels == 4) // Ensure it's B-Format
            {
                juce::AudioBuffer<float> bFormatBuffer;
                bFormatBuffer.setSize(4, (int)reader->lengthInSamples);

                reader->read(&bFormatBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

                juce::AudioBuffer<float> surroundBuffer;
                surroundBuffer.setSize(6, bFormatBuffer.getNumSamples());
                decodeBFormatTo5Point1(bFormatBuffer, surroundBuffer);

                DBG("Decoded Buffer Channels: " << surroundBuffer.getNumChannels());
                DBG("Decoded Buffer Samples: " << surroundBuffer.getNumSamples());

                // Split the surround buffer into individual channel buffers
                std::vector<juce::AudioBuffer<float>> channelBuffers;
                for (int ch = 0; ch < surroundBuffer.getNumChannels(); ++ch)
                {
                    juce::AudioBuffer<float> channelBuffer;
                    channelBuffer.setSize(1, surroundBuffer.getNumSamples());
                    channelBuffer.copyFrom(0, 0, surroundBuffer, ch, 0, surroundBuffer.getNumSamples());
                    channelBuffers.push_back(std::move(channelBuffer));
                }

                {
                    std::lock_guard<std::mutex> lock(bufferMutex);
                    bufferQueue.push(std::move(channelBuffers));
                }
                bufferReady.store(true);
            }
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
    DBG("Processing pending buffers...");
    if (bufferReady.load())
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        while (!bufferQueue.empty())
        {
            auto buffers = std::move(bufferQueue.front());
            bufferQueue.pop();
            DBG("conv size: " << convolutions.size());

            for (size_t i = 0; i < convolutions.size(); ++i)
            {
                if (i < buffers.size())
                {
                    convolutions[i]->loadImpulseResponse(std::move(buffers[i]),
                                                         sampleRate,
                                                         juce::dsp::Convolution::Stereo::no,
                                                         juce::dsp::Convolution::Trim::yes,
                                                         juce::dsp::Convolution::Normalise::yes);
                    DBG("Loaded convolution buffer for channel " << i);
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


// *****************************************************************************
// ******************************* BFormat *************************************
// *****************************************************************************

void IRLoader::decodeBFormatTo5Point1(const juce::AudioBuffer<float>& bFormatBuffer, juce::AudioBuffer<float>& outputBuffer)
{
    // Ensure the B-Format buffer has 4 channels (W, X, Y, Z)
    jassert(bFormatBuffer.getNumChannels() == 4);

    // Ensure the output buffer has 6 channels (5.1 format: L, R, C, LFE, Ls, Rs)
    jassert(outputBuffer.getNumChannels() == 6);

    auto numSamples = bFormatBuffer.getNumSamples();

    // Get pointers to the B-Format channels
    auto* W = bFormatBuffer.getReadPointer(0);
    auto* X = bFormatBuffer.getReadPointer(1);
    auto* Y = bFormatBuffer.getReadPointer(2);
    auto* Z = bFormatBuffer.getReadPointer(3);

    // Get pointers to the output channels
    auto* L = outputBuffer.getWritePointer(0);
    auto* R = outputBuffer.getWritePointer(1);
    auto* C = outputBuffer.getWritePointer(2);
    auto* LFE = outputBuffer.getWritePointer(3);
    auto* Ls = outputBuffer.getWritePointer(4);
    auto* Rs = outputBuffer.getWritePointer(5);

    for (int i = 0; i < numSamples; ++i)
    {
        // Bruce Wiggins pHD bformat Decoder
        L[i] = 0.3623f * W[i] + 0.3208f*Y[i] + 0.4340f*X[i]; //0.707f * (W[i] + X[i]);
        R[i] = 0.3623f * W[i] - 0.3208f*Y[i] + 0.4340f*X[i];
        C[i] = 0*W[i];
        LFE[i] = 0*Z[i];
        Ls[i] = 0.5548f*W[i] + 0.3359f*Y[i] - 0.2415f*X[i]; //* (W[i] + Y[i]);
        Rs[i] = 0.5548f*W[i] - 0.3359f*Y[i] - 0.2415f*X[i];

//        if (i < 10) // Log the first 10 samples
//        {
//            DBG("Sample " << i << ": W=" << W[i] << ", X=" << X[i] << ", Y=" << Y[i] << ", Z=" << Z[i]);
//            DBG("L=" << L[i] << ", R=" << R[i] << ", C=" << C[i] << ", LFE=" << LFE[i] << ", Ls=" << Ls[i] << ", Rs=" << Rs[i]);
//        }
    }
}
