#pragma once

#include <juce_core/juce_core.h>
#include "VoicevoxEngine.h"

namespace cctn
{

class AudioQueryConverter
{
public:
    static voicevox::VoicevoxDecodeSource convertToDecodeSource(const juce::String& audioQuery);

private:
    AudioQueryConverter() = default;
    ~AudioQueryConverter() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioQueryConverter)
};

}
