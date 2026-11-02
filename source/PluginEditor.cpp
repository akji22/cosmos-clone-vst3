#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (scanButton);
    addAndMakeVisible (statusLabel);

    statusLabel.setText ("Select a sample folder to plot your audio galaxy.", juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    scanButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Choose a sample directory...");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      auto dir = fc.getResult();
                                      if (dir.isDirectory())
                                          processorRef.scanFolder (dir);
                                  });
    };

    startTimerHz (15);
    setSize (850, 560);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::timerCallback()
{
    displayStars = processorRef.getStarsSnapshot();

    const float pad = 45.0f;
    const float w = (float) getWidth() - pad * 2.0f;
    const float h = (float) getHeight() - 95.0f;

    for (auto& s : displayStars)
    {
        s.screenPos.x = pad + s.brightness * w;
        s.screenPos.y = 65.0f + (1.0f - s.durationNorm) * h;
    }

    if (processorRef.isScanning)
        statusLabel.setText ("Scanning folder... Found " + juce::String (processorRef.scannedCount.load()) + " samples", juce::dontSendNotification);

    repaint();
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0e16));

    // Top bar background
    g.setColour (juce::Colour (0xff161a28));
    g.fillRect (0, 0, getWidth(), 50);

    // Draw subtle grid lines
    g.setColour (juce::Colour (0xff1e2436));
    g.drawRect (30, 60, getWidth() - 60, getHeight() - 80, 1);

    // Draw Stars
    for (int i = 0; i < (int) displayStars.size(); ++i)
    {
        const auto& s = displayStars[(size_t) i];
        float radius = 4.0f + s.rmsEnergy * 6.0f;

        if (i == hoveredIndex || i == selectedIndex)
            radius += 3.5f;

        juce::Colour dotCol = juce::Colour::fromHSV (0.52f + s.brightness * 0.35f, 0.8f, 0.95f, 0.85f);
        g.setColour (dotCol);
        g.fillEllipse (s.screenPos.x - radius, s.screenPos.y - radius, radius * 2.0f, radius * 2.0f);
    }

    // Hover Tooltip
    if (hoveredIndex >= 0 && hoveredIndex < (int) displayStars.size())
    {
        const auto& s = displayStars[(size_t) hoveredIndex];
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawFittedText (s.file.getFileName() + " (Drag to DAW)",
                          (int) s.screenPos.x + 12, (int) s.screenPos.y - 10, 260, 24,
                          juce::Justification::centredLeft, 1);
    }
}

void PluginEditor::resized()
{
    scanButton.setBounds (15, 10, 170, 30);
    statusLabel.setBounds (200, 10, getWidth() - 215, 30);
}

int PluginEditor::findStarAt (juce::Point<float> pos) const
{
    for (int i = 0; i < (int) displayStars.size(); ++i)
        if (displayStars[(size_t) i].screenPos.getDistanceFrom (pos) < 10.0f)
            return i;
    return -1;
}

void PluginEditor::mouseMove (const juce::MouseEvent& e)
{
    hoveredIndex = findStarAt (e.position);
    repaint();
}

void PluginEditor::mouseDown (const juce::MouseEvent& e)
{
    isDraggingFile = false;
    selectedIndex = findStarAt (e.position);
    if (selectedIndex >= 0)
    {
        const auto& star = displayStars[(size_t) selectedIndex];
        processorRef.auditionSample (star.file);
        statusLabel.setText ("Playing: " + star.file.getFileName() + " — Drag dot into your DAW track!", juce::dontSendNotification);
        repaint();
    }
}

void PluginEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDraggingFile && selectedIndex >= 0 && e.getDistanceFromDragStart() > 6)
    {
        isDraggingFile = true;
        juce::StringArray filesToDrag;
        filesToDrag.add (displayStars[(size_t) selectedIndex].file.getFullPathName());
        performExternalDragDropOfFiles (filesToDrag, false);
    }
}