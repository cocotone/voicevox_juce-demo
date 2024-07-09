#pragma once

#include <juce_core/juce_core.h>
#include "VoicevoxEngine.h"
#include "Format/AudioQueryConverter.h"
#include "Format/ScoreJsonConverter.h"

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
            auto wav_format_byte_array_optional = clientPtr.lock()->tts(request.speakerId, request.text);
            if (wav_format_byte_array_optional.has_value())
            {
                const auto& wav_format_byte_array = wav_format_byte_array_optional.value();
                artefact.wavBinary = std::move(juce::MemoryBlock(wav_format_byte_array.data(), wav_format_byte_array.size()));
            }
        }
        else if (request.processType == VoicevoxEngineProcessType::kHumming)
        {
            double sample_rate_request = 0;
            voicevox::VoicevoxSfDecodeSource sf_decode_source;

            if(request.scoreJson.isNotEmpty())
            {
                const auto predict_speaker_id = clientPtr.lock()->getSongTeacherSpeakerId();
                if (!clientPtr.lock()->isModelLoaded(predict_speaker_id))
                {
                    const auto result = clientPtr.lock()->loadModel(predict_speaker_id);
                    if (result.failed())
                    {
                        juce::Logger::outputDebugString(result.getErrorMessage());
                    }
                }

                const auto low_level_score = cctn::ScoreJsonConverter::convertToLowLevelScore(*clientPtr.lock().get(), request.scoreJson);
                sf_decode_source = cctn::ScoreJsonConverter::convertToSfDecodeSource(low_level_score.value());

                sample_rate_request = clientPtr.lock()->getSampleRate();
            }
            else if (request.audioQuery.isNotEmpty())
            {
                sf_decode_source = cctn::AudioQueryConverter::convertToSfDecodeSource(request.audioQuery, sample_rate_request);
            }

            const auto output_single_channel = clientPtr.lock()->singBySfDecode(request.speakerId, sf_decode_source);
            if (output_single_channel.has_value())
            {
                AudioBufferInfo audio_buffer_info;
                {
                    int num_channels = 1;
                    int num_samples = output_single_channel.value().size();
                    const float* read_ptr = output_single_channel.value().data();
                    audio_buffer_info.audioBuffer.setSize(num_channels, num_samples);
                    audio_buffer_info.audioBuffer.clear();
                    audio_buffer_info.audioBuffer.copyFrom(0, 0, read_ptr, num_samples);
                    audio_buffer_info.sampleRate = sample_rate_request;
                }
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
