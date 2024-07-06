#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
class ProgressPanel
    : public juce::Component
{
public:
    //==============================================================================
    ProgressPanel();
    ~ProgressPanel() override;

private:
    //==============================================================================
    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    std::unique_ptr<juce::ProgressBar> progressBar;
    double valueProgress;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgressPanel)
};
