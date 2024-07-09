namespace cctn
{

//==============================================================================
namespace
{
// Function to repeat elements in a vector
template<typename T>
std::vector<T> repeat(const std::vector<T>& input, const std::vector<T>& repeats) 
{
    std::vector<T> result;
    for (size_t i = 0; i < input.size(); ++i) 
    {
        result.insert(result.end(), repeats[i], input[i]);
    }
    return result;
}
}

//==============================================================================
voicevox::VoicevoxSfDecodeSource AudioQueryConverter::convertToSfDecodeSource(const juce::String& audioQuery, double& sampleRate)
{
    SharedStaticPhonemes static_phonemes;

    voicevox::VoicevoxSfDecodeSource result;
    result.f0Vector.clear();
    result.phonemeVector.clear();
    result.volumeVector.clear();

    juce::var audio_query_json = juce::JSON::parse(audioQuery);

    if (audio_query_json.isObject())
    {
        std::vector<float> f0s;
        if (audio_query_json["f0"].isArray())
        {
            const auto f0_array = *audio_query_json["f0"].getArray();
            for (const auto f0_element : f0_array)
            {
                f0s.push_back((float)f0_element);
            }
        }

        std::vector<float> volumes;
        if (audio_query_json["volume"].isArray())
        {
            const auto volume_array = *audio_query_json["volume"].getArray();
            for (const auto volume_element : volume_array)
            {
                volumes.push_back((float)volume_element);
            }
        }

        std::vector<std::int64_t> phonemes;
        std::vector<std::int64_t> phoneme_lengths;
        if (audio_query_json["phonemes"].isArray())
        {
            const auto phonemes_array = *audio_query_json["phonemes"].getArray();
            for (const auto phoneme_frame : phonemes_array)
            {
                try
                {
                    
                    const auto str_phoneme = phoneme_frame["phoneme"].toString().toStdString();
                    const std::int64_t phoneme_index = static_phonemes->getPhonemeIndex(str_phoneme);
                    const std::int64_t frame_length = phoneme_frame["frame_length"];
                    phonemes.push_back(phoneme_index);
                    phoneme_lengths.push_back(frame_length);
                }
                catch (std::runtime_error e)
                {
                    juce::Logger::outputDebugString(e.what());
                    return voicevox::VoicevoxSfDecodeSource();
                }
            }
        }

        result.f0Vector = f0s;
        result.volumeVector = volumes;
        result.phonemeVector = repeat(phonemes, phoneme_lengths);

        // Update sample rate
        sampleRate = audio_query_json["outputSamplingRate"];
    }

    return result;
}

}
