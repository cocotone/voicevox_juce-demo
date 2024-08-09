#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <voicevox_juce_extra/voicevox_juce_extra.h>
#include <cocotone_song_editor_basics/cocotone_song_editor_basics.h>
#include "SpinLockedPositionInfo.h"

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
    void loadVoicevoxEngineAudioBufferInfo(const cctn::AudioBufferInfo& audioBufferInfo);
    void clearAudioFileHandle();

    // Should call on juce::MessageThread
    void resetAudioThumbnail();
    void updatePlayerState();

    //==============================================================================
    void requestSynthesis(juce::int64 speakerId, const juce::String& text);
    void requestTextToSpeech(juce::int64 speakerId, const juce::String& text);
    void requestHumming(juce::int64 speakerId, const juce::String& text);
    juce::String getMetaJsonStringify();

    //==============================================================================
    juce::AudioFormatManager& getAudioFormatManager() const { return *audioFormatManager.get(); }
    juce::AudioTransportSource& getAudioTransportSource() const { return *audioTransportSource.get(); }
    juce::AudioThumbnail& getAudioThumbnail() const { return *audioThumbnail.get(); }
    juce::ValueTree& getApplicationState() { return applicationState; }
    juce::ValueTree& getEditorState() { return editorState; }

    //==============================================================================
    const juce::StringArray& getVoicevoxTalkSpeakerList() const { return voicevoxTalkSpeakerIdentifierList; };
    const juce::StringArray& getVoicevoxHummingSpeakerList() const { return voicevoxHummingSpeakerIdentifierList; };
    const std::map<juce::String, juce::uint32>& getVoicevoxSpeakerMap() const { return voicevoxMapSpeakerIdentifierToSpeakerId; };

    //==============================================================================
    cctn::song::TransportEmulator& getTransportEmulator() const { return *songTransportEmulator.get(); }

    //==============================================================================
    const juce::AudioPlayHead::PositionInfo getLastPositionInfo() const { return spinLockedLastPositionInfo.get(); }
    double getHostSyncAudioSourceLengthInSeconds() const;

private:
    //==============================================================================
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // juce::ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId) override;

    //==============================================================================
    void updateCurrentTimeInfoFromHost(const juce::AudioPlayHead::PositionInfo& newPositionInfo);

    //==============================================================================
    // Audio
    std::unique_ptr<juce::AudioFormatManager> audioFormatManager;
    std::unique_ptr<juce::TimeSliceThread> audioBufferingThread;
    std::unique_ptr<juce::AudioFormatReaderSource> audioFormatReaderSource;
    std::unique_ptr<juce::MemoryAudioSource> memoryAudioSource;
    std::unique_ptr<juce::AudioTransportSource> audioTransportSource;
    
    // Host sync audio source player
    std::unique_ptr<cctn::HostSyncAudioSourcePlayer> hostSyncAudioSourcePlayer;

    // For audio buffer thumbnail
    juce::AudioThumbnailCache audioThumbnailCache{ 5 };
    std::unique_ptr<juce::AudioThumbnail> audioThumbnail;
    struct AudioDataForAudioThumbnail
    {
        juce::AudioBuffer<float> audioBuffer{};
        double sampleRate{ 0.0 };

        JUCE_LEAK_DETECTOR(AudioDataForAudioThumbnail)
    };
    std::unique_ptr<AudioDataForAudioThumbnail> audioDataForAudioThumbnail;

    // Position info
    SpinLockedPositionInfo spinLockedLastPositionInfo;
    juce::AudioPlayHead::PositionInfo playTriggeredPositionInfo;

    // Voicevox Engine
    std::unique_ptr<cctn::VoicevoxEngine> voicevoxEngine;

    // SongEditor for Voicevox
    std::unique_ptr<cctn::song::TransportEmulator> songTransportEmulator;

    // State
    juce::ValueTree applicationState;
    juce::ValueTree editorState;

    juce::StringArray voicevoxTalkSpeakerIdentifierList;
    juce::StringArray voicevoxHummingSpeakerIdentifierList;
    std::map<juce::String, juce::uint32> voicevoxMapSpeakerIdentifierToSpeakerId;

    std::atomic<bool> isSyncToHostTransport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
