#pragma once

#include "PluginProcessor.h"

#include "View/MusicView.h"
#include "View/PlayerController.h"
#include "View/ProgressPanel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , private juce::ValueTree::Listener
    , private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    virtual void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    //==============================================================================
    virtual void timerCallback() override;

    //==============================================================================
    void updateView(bool isInitial);
    void updateTimecodeDisplay (const juce::AudioPlayHead::PositionInfo& positionInfo);

    //==============================================================================
    AudioPluginAudioProcessor& processorRef;

    // Deprecated
    std::unique_ptr<juce::TreeView> jsonTreeView;
    std::unique_ptr<juce::TreeViewItem> jsonTreeViewItemRoot;
    std::unique_ptr<juce::TextButton> buttonUpdateVoicevoxMetas;

    std::unique_ptr<juce::TextPropertyComponent> speakerIdEditor;
    juce::Value valueSpeakerId;

    std::unique_ptr<juce::TextEditor> textEditor;
    std::unique_ptr<juce::TextButton> buttonInvokeTalk;
    std::unique_ptr<juce::TextButton> buttonInvokeHumming;

    std::unique_ptr<juce::ComboBox> comboboxTalkSpeakerChoice;
    std::unique_ptr<juce::ComboBox> comboboxHummingSpeakerChoice;

    std::unique_ptr<MusicView> musicView;
    std::unique_ptr<PlayerController> playerController;
    std::unique_ptr<juce::Label> labelTimecodeDisplay;

    std::unique_ptr<ProgressPanel> progressPanel;
    juce::CachedValue<bool> valueIsVoicevoxEngineTaskRunning;
    juce::CachedValue<bool> valueIsVoicevoxEngineHasSpeakerListUpdated;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
