#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "VoicevoxEngine/VoicevoxEngine.h"

//==============================================================================
class AudioPluginAudioProcessor final
    : public juce::AudioProcessor
    , private juce::ValueTree::Listener
    , private juce::ChangeListener
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    void loadAudioFile(const juce::File& fileToLoad);
    void loadAudioFileStream(std::unique_ptr<juce::InputStream> audioFileStream);

    //==============================================================================
    void requestSynthesis(juce::int64 speakerId, const juce::String& text);
    void requestTextToSpeech(juce::int64 speakerId, const juce::String& text);
    juce::String getMetaJsonStringify();

    //==============================================================================
    juce::AudioFormatManager& getAudioFormatManager() const { return *audioFormatManager.get(); }
    juce::AudioTransportSource& getAudioTransportSource() const { return *audioTransportSource.get(); }
    juce::AudioThumbnail& getAudioThumbnail() const { return *audioThumbnail.get(); }
    juce::ValueTree& getApplicationState() { return applicationState; }

private:
    //==============================================================================
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // juce::ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId) override;

    //==============================================================================
    // Audio
    std::unique_ptr<juce::AudioFormatManager> audioFormatManager;
    std::unique_ptr<juce::TimeSliceThread> audioBufferingThread;
    std::unique_ptr<juce::AudioFormatReaderSource> audioFormatReaderSource;
    std::unique_ptr<juce::AudioTransportSource> audioTransportSource;

    juce::AudioThumbnailCache audioThumbnailCache{ 5 };
    std::unique_ptr<juce::AudioThumbnail> audioThumbnail;
    juce::AudioBuffer<float> audioBufferForThumbnail;

    std::unique_ptr<cctn::VoicevoxEngine> voicevoxEngine;

    // State
    juce::ValueTree applicationState{ "ApplicationState", {} };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
