#pragma once

#include <juce_core/juce_core.h>

namespace cctn
{

//==============================================================================
class StaticPhonemes final
{
public:
    //==============================================================================
    StaticPhonemes();
    ~StaticPhonemes();

    //==============================================================================
    std::int64_t getPhonemeIndex(const std::string& phoneme) const;

private:
    //==============================================================================
    void initialize();

    //==============================================================================
    std::vector<std::string> phonemeList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticPhonemes)
};

using SharedStaticPhonemes = juce::SharedResourcePointer<StaticPhonemes>;

}
