#pragma once

#include <juce_core/juce_core.h>

namespace cctn
{

//==============================================================================
class StaticPhonemes final
{
public:
    //==============================================================================
    JUCE_DECLARE_SINGLETON(StaticPhonemes, false);

    std::int64_t getPhonemeIndex(const std::string& phoneme) const;

private:
    //==============================================================================
    StaticPhonemes();
    ~StaticPhonemes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticPhonemes)
};

}
