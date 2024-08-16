#pragma once

#include <voicevox_juce_extra/voicevox_juce_extra.h>

#include "PluginProcessor.h"

#include "View/MusicView.h"
#include "View/PlayerController.h"
#include "View/ProgressPanel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , private juce::ValueTree::Listener
    , private juce::Timer
    , private cctn::song::IPositionInfoProvider
    , private cctn::song::IAudioThumbnailProvider
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
    virtual std::optional<juce::AudioPlayHead::PositionInfo> getPositionInfo() override;

    //==============================================================================
    virtual std::optional<juce::AudioThumbnail*> getAudioThumbnail() override;

    //==============================================================================
    void updateView(bool isInitial);
    void updateTimecodeDisplay (const juce::AudioPlayHead::PositionInfo& positionInfo);

    //==============================================================================
    void exchangeSongDocument();

    //==============================================================================
    AudioPluginAudioProcessor& processorRef;

    // SongEditor
    std::unique_ptr<cctn::song::SongEditor> songEditor;
    std::unique_ptr<juce::TextButton> buttonTransportMenu;
    std::unique_ptr<juce::PopupMenu> transportMenu;

    std::unique_ptr<juce::ComboBox> comboboxHummingSpeakerChoice;
    std::unique_ptr<juce::TextButton> buttonInvokeHumming;

    std::unique_ptr<MusicView> musicView;
    std::unique_ptr<PlayerController> playerController;
    std::unique_ptr<juce::Label> labelTimecodeDisplay;

    std::unique_ptr<ProgressPanel> progressPanel;
    juce::CachedValue<bool> valueIsVoicevoxEngineTaskRunning;
    juce::CachedValue<bool> valueIsVoicevoxEngineHasSpeakerListUpdated;

    std::unique_ptr<juce::TextButton> buttonInvokeSongDocumentExchange;
    std::unique_ptr<juce::ThreadWithProgressWindow> processTaskThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
