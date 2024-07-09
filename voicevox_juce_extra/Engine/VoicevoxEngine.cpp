namespace cctn
{

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
                const juce::String style_type = styles["type"];

                juce::String item_text = model_name + " - " + style_name;

                if (style_type == "frame_decode")
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

juce::StringArray VoicevoxEngine::getTalkSpeakerIdentifierList() const
{
    juce::StringArray result;

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
                const juce::String style_type = styles["type"];

                juce::String item_text = model_name + " - " + style_name;

                if (style_type == "frame_decode")
                {
                    item_text = item_text + " - " + "Humming";
                }
                else
                {
                    item_text = item_text + " - " + "Talk";
                }

                // NOTE: Only support talk model.
                if (style_id < 3000)
                {
                    result.add(item_text);
                }
            }
        }
    }

    return result;
}

juce::StringArray VoicevoxEngine::getHummingSpeakerIdentifierList() const
{
    juce::StringArray result;

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
                const juce::String style_type = styles["type"];

                juce::String item_text = model_name + " - " + style_name;

                if (style_type == "frame_decode")
                {
                    item_text = item_text + " - " + "Humming";
                }
                else
                {
                    item_text = item_text + " - " + "Talk";
                }

                // NOTE: Only support humming model.
                if (style_id >= 3000)
                {
                    result.add(item_text);
                }
            }
        }
    }

    return result;
}


//==============================================================================
VoicevoxEngineArtefact VoicevoxEngine::requestSync(const VoicevoxEngineRequest& request)
{
    VoicevoxEngineArtefact artefact;

    artefact.requestId = request.requestId;

    const auto result = voicevoxClient->loadModel(request.speakerId);
    if (result.failed())
    {
        juce::Logger::outputDebugString(result.getErrorMessage());
    }

    auto wav_format_byte_array_optional = voicevoxClient->tts(request.speakerId, request.text);
    if (wav_format_byte_array_optional.has_value())
    {
        const auto& wav_format_byte_array = wav_format_byte_array_optional.value();
        artefact.wavBinary = std::move(juce::MemoryBlock(wav_format_byte_array.data(), wav_format_byte_array.size()));
    }

    return artefact;
}

void VoicevoxEngine::requestAsync(const VoicevoxEngineRequest& request, std::function<void(const VoicevoxEngineArtefact&)> callback)
{
    VoicevoxEngineTask* engine_task = new VoicevoxEngineTask(voicevoxClient, request, callback);
    taskThread->addJob(engine_task, true);
}

}
