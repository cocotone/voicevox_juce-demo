#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// PlayerController
//==============================================================================
class PlayerController final
    : public juce::Component
    , private juce::ValueTree::Listener
{
public:
    //==============================================================================
    explicit PlayerController(juce::ValueTree& appState);
    ~PlayerController() override;

private:
    //==============================================================================
    // juce::Component
    void paint (juce::Graphics& g) override;
    void resized() override;

    // juce::ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId);

    void updateView();

    //==============================================================================
    std::unique_ptr<juce::GroupComponent> groupPlayerController;

    std::unique_ptr<juce::TextButton> playButton;
    std::unique_ptr<juce::TextButton> loopButton;
    std::unique_ptr<juce::ToggleButton> syncToHostButton;

    juce::ValueTree applicationState;
    juce::CachedValue<bool> valueCanPlay;
    juce::CachedValue<bool> valueIsPlaying;
    juce::CachedValue<bool> valueIsLooping;
    juce::CachedValue<bool> valueIsSyncToHost;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerController)
};
