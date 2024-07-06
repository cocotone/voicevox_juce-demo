#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <voicevox_juce/voicevox_juce.h>

namespace cctn
{

//==============================================================================
struct VoicevoxEngineArtefact
{
    juce::Uuid requestId;
    std::optional<juce::MemoryBlock> wavBinary;

    JUCE_LEAK_DETECTOR(VoicevoxEngineArtefact)
};

//==============================================================================
struct VoicevoxEngineRequest
{
    juce::Uuid requestId;
    juce::int64 speakerId { 0 };
    juce::String text{ "" };

    JUCE_LEAK_DETECTOR(VoicevoxEngineRequest)
};

//==============================================================================
class VoicevoxEngineTaskObserver
{
public:
    VoicevoxEngineTaskObserver() = default;
    virtual ~VoicevoxEngineTaskObserver() = default;

    virtual void onTaskCompleted()
    {
        juce::Logger::outputDebugString("VoicevoxEngineTaskObserver::onTaskCompleted");
    };

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicevoxEngineTaskObserver)
};

//==============================================================================
class VoicevoxEngine final
{
public:
    //==============================================================================
    VoicevoxEngine();
    ~VoicevoxEngine();

    //==============================================================================
    void initialize();
    void shutdown();

    void start();
    void stop();

    //==============================================================================
    juce::String getMetaJsonStringify();
    VoicevoxEngineArtefact requestTextToSpeech(const VoicevoxEngineRequest& request);
    void requestTextToSpeechAsync(const VoicevoxEngineRequest& request, std::function<void(const VoicevoxEngineArtefact&)> callback);

private:
    //==============================================================================
    std::shared_ptr<voicevox::VoicevoxClient> voicevoxClient;
    std::unique_ptr<juce::ThreadPool> taskThread;

    std::shared_ptr<VoicevoxEngineTaskObserver> taskObserver;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicevoxEngine)
};

}
