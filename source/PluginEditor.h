#pragma once
#include "PluginProcessor.h"

class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    int findStarAt (juce::Point<float> pos) const;

    PluginProcessor& processorRef;
    juce::TextButton scanButton { "Select Sample Folder" };
    juce::Label statusLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<SampleStar> displayStars;
    int hoveredIndex = -1;
    int selectedIndex = -1;
    bool isDraggingFile = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};