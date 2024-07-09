#pragma once

#include <juce_core/juce_core.h>

namespace cctn
{

//==============================================================================
using MoraKana = juce::String;

struct MoraPhoneme
{
    std::optional<juce::String> consonant;
    std::optional<juce::String> vowel;
};

//==============================================================================
class StaticMoraMapping final
{
public:
    //==============================================================================
    StaticMoraMapping();
    ~StaticMoraMapping();

    //==============================================================================
    std::optional<MoraPhoneme> convertMoraKanaToMoraPhonemes(const MoraKana& moraKana) const;

    const std::map<MoraKana, MoraPhoneme>& getMoraMapping() const;

private:
    //==============================================================================
    void initialize();

    //==============================================================================
    std::map<MoraKana, MoraPhoneme> mapMoraKanaToMoraPhoneme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticMoraMapping)
};

using SharedStaticMoraMapping = juce::SharedResourcePointer<StaticMoraMapping>;

}
