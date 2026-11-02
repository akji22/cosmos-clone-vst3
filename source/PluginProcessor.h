#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>

struct SampleStar
{
    juce::File file;
    float brightness = 0.5f; // X-axis (0.0 to 1.0)
    float durationNorm = 0.5f; // Y-axis (0.0 to 1.0)
    float rmsEnergy = 0.5f; // Dot radius & color intensity
    juce::Point<float> screenPos;
};

class PluginProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CosmosClone"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // Cosmos Clone API
    void scanFolder (const juce::File& folder);
    void auditionSample (const juce::File& file);

    std::vector<SampleStar> getStarsSnapshot()
    {
        const juce::ScopedLock sl (starLock);
        return stars;
    }

    std::atomic<bool> isScanning { false };
    std::atomic<int> scannedCount { 0 };

private:
    void run() override; // Background scanner thread
    void analyzeFile (const juce::File& file);

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    juce::File targetFolder;
    juce::CriticalSection starLock;
    std::vector<SampleStar> stars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};