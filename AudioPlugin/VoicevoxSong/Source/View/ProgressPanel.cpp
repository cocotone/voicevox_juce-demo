#include "ProgressPanel.h"

//==============================================================================
ProgressPanel::ProgressPanel()
{
    valueProgress = -1;

    progressBar = std::make_unique<juce::ProgressBar>(valueProgress, juce::ProgressBar::Style::circular);
    progressBar->setTextToDisplay("Processing...");
    progressBar->setColour(juce::ProgressBar::ColourIds::foregroundColourId, juce::Colours::white);
    addAndMakeVisible(progressBar.get());
}

ProgressPanel::~ProgressPanel()
{
}

//==============================================================================
void ProgressPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.withAlpha(0.6f));
}

void ProgressPanel::resized()
{
    auto rect_area = getLocalBounds();
    const int radius = rect_area.getWidth() * 0.33f;
    progressBar->setBounds(rect_area.withSizeKeepingCentre(radius, radius));
}
