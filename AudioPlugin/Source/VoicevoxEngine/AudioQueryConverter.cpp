#include "AudioQueryConverter.h"

namespace cctn
{

voicevox::VoicevoxDecodeSource AudioQueryConverter::convertToDecodeSource(const juce::String& audioQuery)
{
    voicevox::VoicevoxDecodeSource result;
    result.f0Vector.clear();
    result.phonemeVector.clear();

    juce::var audio_query_json = juce::JSON::parse(audioQuery);

    if (audio_query_json.isObject())
    {
        if (audio_query_json["f0"].isArray())
        {
            const auto f0_array = *audio_query_json["f0"].getArray();
            for (const auto f0_element : f0_array)
            {
                result.f0Vector.push_back((float)f0_element);
            }
        }

        //  frame_phonemes = np.repeat(phonemes_array, phoneme_lengths_array)
        /*
        *   {
                "phoneme": "e",
                "frame_length": 2
            },
        */
        if (audio_query_json["phonemes"].isArray())
        {
            const auto phonemes_array = *audio_query_json["phonemes"].getArray();
            for (const auto phoneme_frame : phonemes_array)
            {
                for (int idx = 0; idx < (int)phoneme_frame["frame_length"]; idx++)
                {
                    result.phonemeVector.push_back((float)phoneme_frame["phoneme"]);
                }
            }
        }

        result.sampleRate = audio_query_json["outputSamplingRate"];
    }

    return result;
}

}
