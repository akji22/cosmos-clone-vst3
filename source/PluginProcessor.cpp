#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("CosmosScannerThread")
{
    formatManager.registerBasicFormats();
}

PluginProcessor::~PluginProcessor()
{
    stopThread (2000);
    transportSource.setSource (nullptr);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transportSource.prepareToPlay (samplesPerBlock, sampleRate);
}

void PluginProcessor::releaseResources()
{
    transportSource.releaseResources();
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    juce::AudioSourceChannelInfo channelInfo (&buffer, 0, buffer.getNumSamples());
    transportSource.getNextAudioBlock (channelInfo);
}

void PluginProcessor::auditionSample (const juce::File& file)
{
    if (auto* reader = formatManager.createReaderFor (file))
    {
        transportSource.stop();
        transportSource.setSource (nullptr);
        readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
        transportSource.setSource (readerSource.get(), 0, nullptr, reader->sampleRate);
        transportSource.setPosition (0.0);
        transportSource.start();
    }
}

void PluginProcessor::scanFolder (const juce::File& folder)
{
    stopThread (2000);
    targetFolder = folder;
    {
        const juce::ScopedLock sl (starLock);
        stars.clear();
    }
    scannedCount = 0;
    isScanning = true;
    startThread();
}

void PluginProcessor::run()
{
    auto flags = juce::File::findFiles;
    juce::RangedDirectoryIterator iter (targetFolder, true, "*.wav;*.aif;*.aiff;*.flac;*.mp3", flags);

    for (const auto& entry : iter)
    {
        if (threadShouldExit())
            break;

        analyzeFile (entry.getFile());
        if (++scannedCount >= 500) // Cap at 500 samples for fast initial responsiveness
            break;
    }
    isScanning = false;
}

void PluginProcessor::analyzeFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return;

    const int numSamplesToRead = (int) juce::jmin<juce::int64> (reader->lengthInSamples, 44100 * 4);
    juce::AudioBuffer<float> tempBuffer (1, numSamplesToRead);
    reader->read (&tempBuffer, 0, numSamplesToRead, 0, true, false);

    const float* data = tempBuffer.getReadPointer (0);
    double sumSq = 0.0;
    int zeroCrossings = 0;

    for (int i = 1; i < numSamplesToRead; ++i)
    {
        sumSq += data[i] * data[i];
        if ((data[i] >= 0.0f && data[i - 1] < 0.0f) || (data[i] < 0.0f && data[i - 1] >= 0.0f))
            ++zeroCrossings;
    }

    float rms = std::sqrt ((float) (sumSq / numSamplesToRead));
    float zcr = (float) zeroCrossings / (float) numSamplesToRead;
    float durationSec = (float) reader->lengthInSamples / (float) reader->sampleRate;

    SampleStar star;
    star.file = file;
    star.brightness = juce::jlimit (0.0f, 1.0f, zcr * 4.0f);
    star.durationNorm = juce::jlimit (0.0f, 1.0f, durationSec / 8.0f);
    star.rmsEnergy = juce::jlimit (0.1f, 1.0f, rms * 3.5f);

    const juce::ScopedLock sl (starLock);
    stars.push_back (star);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}