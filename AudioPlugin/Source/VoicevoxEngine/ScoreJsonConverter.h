#pragma once

#include <juce_core/juce_core.h>
#include "VoicevoxEngine.h"

namespace cctn
{

class ScoreJsonConverter
{
public:
    static voicevox::VoicevoxSfDecodeSource convertToSfDecodeSource(const voicevox::VoicevoxClient& voicevoxClient, const juce::String& scoreJson);

private:
    ScoreJsonConverter() = default;
    ~ScoreJsonConverter() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScoreJsonConverter)
};

}
