namespace cctn
{

namespace
{
//==============================================================================
// BaseVowel and Vowel are represented as strings in C++
using BaseVowel = std::string;
using Vowel = std::string;
using Consonant = std::string;

// List of phonemes
const std::array<std::string, 10> kPhonemeList1 = {"pau", "A", "E", "I", "N", "O", "U", "a", "b", "by"};
const std::array<std::string, 10> kPhonemeList2 = {"ch", "cl", "d", "dy", "e", "f", "g", "gw", "gy", "h"};
const std::array<std::string, 10> kPhonemeList3 = {"hy", "i", "j", "k", "kw", "ky", "m", "my", "n", "ny"};
const std::array<std::string, 10> kPhonemeList4 = {"o", "p", "py", "r", "ry", "s", "sh", "t", "ts", "ty"};
const std::array<std::string,  5> kPhonemeList5 = {"u", "v", "w", "y", "z"};

const std::vector<std::string> kUnvoicedMoraTailPhonemeList = { "A", "I", "U", "E", "O", "cl", "pau" };
const std::vector<std::string> kMoraTailPhonemeList = { "a", "i", "u", "e", "o", "N", "A", "I", "U", "E", "O", "cl", "pau" };

// Number of elements in the phoneme list
const size_t kNumPhoneme = kPhonemeList1.size() + kPhonemeList2.size() + kPhonemeList3.size() + kPhonemeList3.size() + kPhonemeList5.size();

//==============================================================================
class PhonemeHelper
{
public:
    //==============================================================================
    PhonemeHelper(const std::string& phoneme_, const std::vector<std::string>& phonemeList_)
        : myphoneme(phoneme_)
        , phonemeListRef(phonemeList_)
    {
        // Convert silence to pause
        if (myphoneme.find("sil") != std::string::npos)
        {
            myphoneme = "pau";
        }
        else
        {
            myphoneme = myphoneme;
        }
    }

    ~PhonemeHelper() = default;

    //==============================================================================
    std::int64_t getPhonemeId() const
    {
        auto it = std::find(phonemeListRef.begin(), phonemeListRef.end(), myphoneme);
        if (it == phonemeListRef.end())
        {
            //throw std::runtime_error("Invalid phoneme");
            return -1;
        }
        return std::distance(phonemeListRef.begin(), it);
    }

    std::vector<float> onehot() const
    {
        std::vector<float> vec(kNumPhoneme, 0.0f);
        if (getPhonemeId() >= 0)
        {
            vec[getPhonemeId()] = 1.0f;
            return vec;
        }
        return vec;
    }

    bool isMoraTail() const
    {
        return std::find(kMoraTailPhonemeList.begin(), kMoraTailPhonemeList.end(), myphoneme) != kMoraTailPhonemeList.end();
    }

    bool isUnvoicedMoraTail() const
    {
        return std::find(kUnvoicedMoraTailPhonemeList.begin(), kUnvoicedMoraTailPhonemeList.end(), myphoneme) != kUnvoicedMoraTailPhonemeList.end();
    }

private:
    //==============================================================================
    std::string myphoneme;
    const std::vector<std::string>& phonemeListRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhonemeHelper)
};

} // namespace

//==============================================================================
StaticPhonemes::StaticPhonemes()
{
    initialize();
}

StaticPhonemes::~StaticPhonemes()
{
    phonemeList.clear();
}

//==============================================================================
void StaticPhonemes::initialize()
{
    phonemeList.clear();
    phonemeList.insert(phonemeList.end(), kPhonemeList1.begin(), kPhonemeList1.end());
    phonemeList.insert(phonemeList.end(), kPhonemeList2.begin(), kPhonemeList2.end());
    phonemeList.insert(phonemeList.end(), kPhonemeList3.begin(), kPhonemeList3.end());
    phonemeList.insert(phonemeList.end(), kPhonemeList4.begin(), kPhonemeList4.end());
    phonemeList.insert(phonemeList.end(), kPhonemeList5.begin(), kPhonemeList5.end());
}

//==============================================================================
std::int64_t StaticPhonemes::getPhonemeIndex(const std::string& phoneme) const
{
    return PhonemeHelper(phoneme, phonemeList).getPhonemeId();
}

} // namespace cctn
