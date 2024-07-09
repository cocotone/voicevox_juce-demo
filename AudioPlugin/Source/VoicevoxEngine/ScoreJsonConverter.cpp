#include "ScoreJsonConverter.h"
#include "StaticPhonemes.h"

namespace cctn
{

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

std::vector<int64_t> calculatePhonemeLengthVector(const std::vector<int64_t>& consonant_lengths_vector, const std::vector<int64_t>& note_lengths_vector)
{
    std::vector<int64_t> phoneme_durations;

    for (size_t cons_idx = 0; cons_idx < consonant_lengths_vector.size(); cons_idx++)
    {
        if (cons_idx < consonant_lengths_vector.size() - 1)
        {
            if (cons_idx == 0 && consonant_lengths_vector[cons_idx] != 0)
            {
                return std::vector<int64_t>();
            }

            int64_t next_consonant_length = consonant_lengths_vector[cons_idx + 1];
            int64_t note_duration = note_lengths_vector[cons_idx];

            if (next_consonant_length < 0)
            {
                next_consonant_length = note_duration / 2;
                const_cast<std::vector<int64_t>&>(consonant_lengths_vector)[cons_idx + 1] = next_consonant_length;
            }

            int64_t vowel_length = note_duration - next_consonant_length;

            if (vowel_length < 0)
            {
                next_consonant_length = note_duration / 2;
                const_cast<std::vector<int64_t>&>(consonant_lengths_vector)[cons_idx + 1] = next_consonant_length;
                vowel_length = note_duration - next_consonant_length;
            }

            phoneme_durations.push_back(vowel_length);
            
            if (next_consonant_length > 0)
            {
                phoneme_durations.push_back(next_consonant_length);
            }
        }
        else 
        {
            int64_t vowel_length = note_lengths_vector[cons_idx];
            phoneme_durations.push_back(vowel_length);
        }
    }

    return phoneme_durations;
}

}

cctn::VoicevoxEngineLowLevelScore ScoreJsonConverter::convertToLowLevelScore(voicevox::VoicevoxClient& voicevoxClient, const juce::String& scoreJson)
{
    SharedStaticPhonemes static_phonemes;

    cctn::VoicevoxEngineLowLevelScore result;

    juce::var score_json = juce::JSON::parse(scoreJson);

    if (score_json.isObject() && score_json["notes"].isArray())
    {
        std::vector<int64_t> note_lengths;
        std::vector<int64_t> note_consonants;
        std::vector<int64_t> note_vowels;
        std::vector<int64_t> phonemes;
        std::vector<int64_t> phoneme_keys;

        const auto* notes_array = score_json["notes"].getArray();
        for (const auto& note : *notes_array)
        {
            const int key = (int)note["key"];
            const int frame_length = (int)note["frame_length"];
            const juce::String lyric = note["lyric"].toString();

            if (lyric.isEmpty())
            {
                if (key != -1) 
                {
                }
                note_lengths.push_back(frame_length);
                note_consonants.push_back(-1);
                note_vowels.push_back(0);  // pau
                phonemes.push_back(0);  // pau
                phoneme_keys.push_back(-1);
            }
            else 
            {
                if (key == -1)
                {
                }

                // Fixed values as per requirement
                const std::string consonant = "r";
                const std::string vowel = "a";

                const int consonant_id = (consonant.empty()) ? -1 : static_phonemes->getPhonemeIndex(consonant);
                const int vowel_id = static_phonemes->getPhonemeIndex(vowel);

                note_lengths.push_back(frame_length);
                note_consonants.push_back(consonant_id);
                note_vowels.push_back(vowel_id);

                if (consonant_id != -1)
                {
                    phonemes.push_back(consonant_id);
                    phoneme_keys.push_back(key);
                }

                phonemes.push_back(vowel_id);
                phoneme_keys.push_back(key);
            }
        }

        // Assign to C++ object
        result._note_consonant_vector = note_consonants;
        result._note_vowel_vector = note_vowels;
        result._note_length_vector = note_lengths;
        result._phonemes = phonemes;
        result._phoneme_key_vector = phoneme_keys;
    }

    if(score_json.isObject() && score_json["notes"].isArray())
    {
        const auto predict_speaker_id = voicevoxClient.getSongTeacherSpeakerId();

        auto consonant_length_vector = voicevoxClient.predictSingConsonantLength(predict_speaker_id, result._note_consonant_vector, result._note_vowel_vector, result._note_length_vector).value();

        result._phoneme_length_vector = calculatePhonemeLengthVector(consonant_length_vector, result._note_length_vector);

        const auto repeated_phonemes = repeat(result._phonemes, result._phoneme_length_vector);
        const auto repeated_keys = repeat(result._phoneme_key_vector, result._phoneme_length_vector);

        result.f0Vector = voicevoxClient.predictSingF0(predict_speaker_id, repeated_phonemes, repeated_keys).value();

        result.volumeVector = voicevoxClient.predictSingVolume(predict_speaker_id, repeated_phonemes, repeated_keys, result.f0Vector).value();

        result.phonemeVector = repeat(result._phonemes, result._phoneme_length_vector);
    }

    return result;
}

voicevox::VoicevoxSfDecodeSource ScoreJsonConverter::convertToSfDecodeSource(const cctn::VoicevoxEngineLowLevelScore& lowLevelScore)
{
    voicevox::VoicevoxSfDecodeSource result;
    result.f0Vector.clear();
    result.phonemeVector.clear();
    result.volumeVector.clear();

    result.f0Vector = lowLevelScore.f0Vector;
    result.phonemeVector = lowLevelScore.phonemeVector;
    result.volumeVector = lowLevelScore.volumeVector;

    return result;
}

}