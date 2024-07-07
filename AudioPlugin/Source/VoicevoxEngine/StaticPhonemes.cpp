#include "StaticPhonemes.h"

#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace cctn
{

namespace
{
// BaseVowel and Vowel are represented as strings in C++
using BaseVowel = std::string;
using Vowel = std::string;
using Consonant = std::string;

// List of phonemes
const std::array<std::string, 10> _P_LIST1 = {"pau", "A", "E", "I", "N", "O", "U", "a", "b", "by"};
const std::array<std::string, 10> _P_LIST2 = {"ch", "cl", "d", "dy", "e", "f", "g", "gw", "gy", "h"};
const std::array<std::string, 10> _P_LIST3 = {"hy", "i", "j", "k", "kw", "ky", "m", "my", "n", "ny"};
const std::array<std::string, 10> _P_LIST4 = {"o", "p", "py", "r", "ry", "s", "sh", "t", "ts", "ty"};
const std::array<std::string, 5> _P_LIST5 = {"u", "v", "w", "y", "z"};

const std::vector<std::string> UNVOICED_MORA_TAIL_PHONEMES = { "A", "I", "U", "E", "O", "cl", "pau" };
const std::vector<std::string> MORA_TAIL_PHONEMES = { "a", "i", "u", "e", "o", "N", "A", "I", "U", "E", "O", "cl", "pau" };

// Number of elements in the phoneme list
const size_t _NUM_PHONEME = _P_LIST1.size() + _P_LIST2.size() + _P_LIST3.size() + _P_LIST4.size() + _P_LIST5.size();

class PhonemeHelper
{
public:
    PhonemeHelper(const std::string& phoneme_, const std::vector<std::string>& phonemeList_)
        : phoneme(phoneme_)
        , phonemeList(phonemeList_)
    {
        // Convert silence to pause
        if (phoneme.find("sil") != std::string::npos) {
            phoneme = "pau";
        } else {
            phoneme = phoneme;
        }
    }

    ~PhonemeHelper() = default;

    std::int64_t id() const
    {
        auto it = std::find(phonemeList.begin(), phonemeList.end(), phoneme);
        if (it == phonemeList.end()) {
            throw std::runtime_error("Invalid phoneme");
        }
        return std::distance(phonemeList.begin(), it);
    }

    std::vector<float> onehot() const
    {
        std::vector<float> vec(_NUM_PHONEME, 0.0f);
        vec[id()] = 1.0f;
        return vec;
    }

    bool is_mora_tail() const
    {
        return std::find(MORA_TAIL_PHONEMES.begin(), MORA_TAIL_PHONEMES.end(), phoneme) != MORA_TAIL_PHONEMES.end();
    }

    bool is_unvoiced_mora_tail() const
    {
        return std::find(UNVOICED_MORA_TAIL_PHONEMES.begin(), UNVOICED_MORA_TAIL_PHONEMES.end(), phoneme) != UNVOICED_MORA_TAIL_PHONEMES.end();
    }

private:
    std::string phoneme;
    const std::vector<std::string>& phonemeList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhonemeHelper)
};

}

//==============================================================================
StaticPhonemes::StaticPhonemes()
{
    initializePhonemeList();
}

StaticPhonemes::~StaticPhonemes()
{
    phonemeList.clear();
}

void StaticPhonemes::initializePhonemeList()
{
    phonemeList.clear();
    phonemeList.insert(phonemeList.end(), _P_LIST1.begin(), _P_LIST1.end());
    phonemeList.insert(phonemeList.end(), _P_LIST2.begin(), _P_LIST2.end());
    phonemeList.insert(phonemeList.end(), _P_LIST3.begin(), _P_LIST3.end());
    phonemeList.insert(phonemeList.end(), _P_LIST4.begin(), _P_LIST4.end());
    phonemeList.insert(phonemeList.end(), _P_LIST5.begin(), _P_LIST5.end());
}

//==============================================================================
std::int64_t StaticPhonemes::getPhonemeIndex(const std::string& phoneme) const
{
    return (std::int64_t)PhonemeHelper(phoneme, phonemeList).id();
}

}