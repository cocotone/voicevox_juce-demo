#pragma once

#include <juce_core/juce_core.h>
#include "VoicevoxEngine.h"

namespace cctn
{

//==============================================================================
class ScoreJsonConverter
{
public:
    //==============================================================================
    static cctn::VoicevoxEngineLowLevelScore convertToLowLevelScore(voicevox::VoicevoxClient& voicevoxClient, const juce::String& scoreJson);
    static voicevox::VoicevoxSfDecodeSource convertToSfDecodeSource(const cctn::VoicevoxEngineLowLevelScore& lowLevelScore);

private:
    //==============================================================================
    ScoreJsonConverter() = default;
    ~ScoreJsonConverter() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScoreJsonConverter)
};

}
