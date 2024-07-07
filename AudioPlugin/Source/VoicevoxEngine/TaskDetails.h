#pragma once

#include <juce_core/juce_core.h>
#include "VoicevoxEngine.h"

namespace cctn
{

class VoicevoxEngineTask
    : public juce::ThreadPoolJob
{
public:
    explicit VoicevoxEngineTask(std::shared_ptr<voicevox::VoicevoxClient> client_,
        VoicevoxEngineRequest request_,
        std::function<void(const VoicevoxEngineArtefact&)> callback_)
        : juce::ThreadPoolJob(request_.requestId.toString())
        , clientPtr(client_)
        , request(request_)
        , callbackIfComplete(callback_)
    {
    }

    virtual juce::ThreadPoolJob::JobStatus runJob() override
    {
        VoicevoxEngineArtefact artefact;

        artefact.requestId = request.requestId;

        if (clientPtr.expired())
        {
            return juce::ThreadPoolJob::JobStatus::jobHasFinished;
        }

        if (!clientPtr.lock()->isModelLoaded(request.speakerId))
        {
            const auto result = clientPtr.lock()->loadModel(request.speakerId);
            if (result.failed())
            {
                juce::Logger::outputDebugString(result.getErrorMessage());
            }
        }

        if (request.processType == VoicevoxEngineProcessType::kTalk)
        {
            auto memory_wav = clientPtr.lock()->tts(request.speakerId, request.text);
            if (memory_wav.has_value())
            {
                artefact.wavBinary = std::move(memory_wav.value());
            }
        }
        else if (request.processType == VoicevoxEngineProcessType::kHumming)
        {
            auto output_blocks = clientPtr.lock()->humming(request.speakerId, request.text);
            if (output_blocks.has_value())
            {
                const auto& float_array_array = output_blocks.value();

                juce::Array<float> buffer_one_channel;
                buffer_one_channel.clear();
                for (const auto& float_array : float_array_array)
                {
                    buffer_one_channel.addArray(float_array);
                }

                AudioBufferInfo audio_buffer_info;
                audio_buffer_info.audioBuffer.clear();
                int num_channels = 1;
                int num_samples = buffer_one_channel.size();
                audio_buffer_info.audioBuffer.setSize(num_channels, num_samples);
                audio_buffer_info.audioBuffer.copyFrom(0, 0, buffer_one_channel.getRawDataPointer(), buffer_one_channel.size());

                audio_buffer_info.sampleRate = request.sampleRate;

                artefact.audioBufferInfo = std::move(audio_buffer_info);
            }
        }

        if (callbackIfComplete != nullptr)
        {
            callbackIfComplete(artefact);
        }

        return juce::ThreadPoolJob::JobStatus::jobHasFinished;
    }

private:
    std::weak_ptr<voicevox::VoicevoxClient> clientPtr;
    VoicevoxEngineRequest request;
    std::function<void(const VoicevoxEngineArtefact&)> callbackIfComplete;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicevoxEngineTask)
};

}
