#include "VoicevoxEngine.h"

namespace cctn
{

//==============================================================================
namespace
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
            auto memory_wav = clientPtr.lock()->humming(request.speakerId, request.text);
            if (memory_wav.has_value())
            {
                artefact.wavBinary = std::move(memory_wav.value());
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

//==============================================================================
VoicevoxEngine::VoicevoxEngine()
{
    voicevoxClient = std::make_shared<voicevox::VoicevoxClient>();

    taskObserver = std::make_shared<VoicevoxEngineTaskObserver>();

    const auto options = 
        juce::ThreadPool::Options()
        .withThreadName("VoicevoxEngineTaskThread");
    taskThread = std::make_unique<juce::ThreadPool>(options);
}

VoicevoxEngine::~VoicevoxEngine()
{
    taskObserver.reset();
    taskThread->removeAllJobs(true, 1000);
}

//==============================================================================
void VoicevoxEngine::initialize()
{
}

void VoicevoxEngine::shutdown()
{
}

void VoicevoxEngine::start()
{
    voicevoxClient->connect();
}

void VoicevoxEngine::stop()
{
    voicevoxClient->disconnect();
}

juce::var VoicevoxEngine::getMetaJson() const
{
    return voicevoxClient->getMetasJson();
}

//==============================================================================
std::map<juce::String, juce::uint32> VoicevoxEngine::getSpeakerIdentifierToSpeakerIdMap() const
{
    std::map<juce::String, juce::uint32> result;

    const auto metas_json = voicevoxClient->getMetasJson();

    if (metas_json.isArray())
    {
        for (const auto& element : *metas_json.getArray())
        {
            const juce::String model_name = element["name"];

            for (const auto& styles : *element["styles"].getArray())
            {
                const juce::String style_name = styles["name"];
                const juce::int64 style_id = styles["id"];

                juce::String item_text = model_name + " - " + style_name;

                if (style_id >= 3000)
                {
                    item_text = item_text + " - " + "Humming";
                }
                else
                {
                    item_text = item_text + " - " + "Talk";
                }

                // NOTE: Only support talk model.
                //if (style_id < 3000)
                {
                    result[item_text] = style_id;
                }
            }
        }
    }

    return result;
}

juce::StringArray VoicevoxEngine::getSpeakerIdentifierList() const
{
    juce::StringArray result;

    const auto metas_json = voicevoxClient->getMetasJson();
    
    if (metas_json.isArray())
    {
        for (const auto& element : *metas_json.getArray())
        {
            const juce::String model_name = element["name"];

            for(const auto& styles : *element["styles"].getArray())
            {
                const juce::String style_name = styles["name"];
                const juce::int64 style_id = styles["id"];

                juce::String item_text = model_name + " - " + style_name;

                if (style_id >= 3000)
                {
                    item_text = item_text + " - " + "Humming";
                }
                else
                {
                    item_text = item_text + " - " + "Talk";
                }

                // NOTE: Only support talk model.
                //if (style_id < 3000)
                {
                    result.add(item_text);
                }
            }
        }
    }

    return result;
}

//==============================================================================
VoicevoxEngineArtefact VoicevoxEngine::requestTextToSpeech(const VoicevoxEngineRequest& request)
{
    VoicevoxEngineArtefact artefact;

    artefact.requestId = request.requestId;

    const auto result = voicevoxClient->loadModel(request.speakerId);
    if (result.failed())
    {
        juce::Logger::outputDebugString(result.getErrorMessage());
    }

    auto memory_wav = voicevoxClient->tts(request.speakerId, request.text);
    if (memory_wav.has_value())
    {
        artefact.wavBinary = std::move(memory_wav.value());
    }

    return artefact;
}

void VoicevoxEngine::requestTextToSpeechAsync(const VoicevoxEngineRequest& request, std::function<void(const VoicevoxEngineArtefact&)> callback)
{
    VoicevoxEngineTask* engine_task = new VoicevoxEngineTask(voicevoxClient, request, callback);
    taskThread->addJob(engine_task, true);
}

}
