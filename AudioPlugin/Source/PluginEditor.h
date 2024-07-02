#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    std::unique_ptr<juce::TreeView> jsonTreeView;
    std::unique_ptr<juce::TreeViewItem> jsonTreeViewItemRoot;
    std::unique_ptr<juce::TextButton> buttonUpdateVoicevoxMetas;

    std::unique_ptr<juce::TextPropertyComponent> speakerIdEditor;
    juce::Value valueSpeakerId;

    std::unique_ptr<juce::TextEditor> textEditor;
    std::unique_ptr<juce::TextButton> buttonInvokeTTS;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
