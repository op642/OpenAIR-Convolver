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

void IRLoader::loadMultichannelIRFile(const juce::File& irFile, double sampleRate, int numChannels)
{
    auto task = [this, irFile, sampleRate]()
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
        if (reader != nullptr && reader->numChannels == 4) // Ensure it's a B-Format file
        {
            juce::AudioBuffer<float> bFormatBuffer;
            bFormatBuffer.setSize(4, (int)reader->lengthInSamples);

            reader->read(&bFormatBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

            // Decode B-Format to 5.1
            juce::AudioBuffer<float> surroundBuffer;
            surroundBuffer.setSize(6, bFormatBuffer.getNumSamples()); // 5.1 has 6 channels

            decodeBFormatTo5Point1(bFormatBuffer, surroundBuffer);

            // Push the decoded buffer to the queue
            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                bufferQueue.push({std::move(surroundBuffer)});
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
        L[i] = 0.707f * (W[i] + X[i]);
        R[i] = 0.707f * (W[i] - X[i]);
        C[i] = W[i];
        LFE[i] = Z[i];
        Ls[i] = 0.707f * (W[i] + Y[i]);
        Rs[i] = 0.707f * (W[i] - Y[i]);
    }
}
//
//void OpenAIRConvolverAudioProcessor::loadBFormatFile(const juce::File& bFormatFile, juce::AudioBuffer<float>& bFormatBuffer)
//{
//    juce::AudioFormatManager formatManager;
//    formatManager.registerBasicFormats();
//
//    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(bFormatFile));
//    if (reader != nullptr)
//    {
//        bFormatBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
//        reader->read(&bFormatBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
//    }
//}
//
//void OpenAIRConvolverAudioProcessor::loadAndDecodeBFormatFile(const juce::File& bFormatFile)
//{
//    if (!bFormatFile.existsAsFile())
//    {
//        DBG("File does not exist: " << bFormatFile.getFullPathName());
//        return;
//    }
//
//    juce::AudioBuffer<float> outputBuffer;
//    outputBuffer.setSize(6, 0); // 5.1 format has 6 channels
//
//    decodeBFormatTo5Point1(bFormatFile, outputBuffer);
//
//    decodedIRBuffer = outputBuffer;
//
//    // Set the saved IR file and root directory
//    setSavedIRFile(bFormatFile);
//    setRoot(bFormatFile.getParentDirectory());
//
//    // Reset and load the convolution processors
//    for (int channel = 0; channel < convolutions.size(); ++channel)
//    {
//        convolutions[channel]->reset();
//        juce::AudioBuffer<float> monoIRBuffer(1, decodedIRBuffer.getNumSamples());
//        monoIRBuffer.copyFrom(0, 0, decodedIRBuffer, channel, 0, decodedIRBuffer.getNumSamples());
//
//        convolutions[channel]->loadImpulseResponse(bFormatFile, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
//    }
//}
