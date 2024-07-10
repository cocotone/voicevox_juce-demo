#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

class SpinLockedPositionInfo
{
public:
    // Wait-free, but setting new info may fail if the main thread is currently
    // calling `get`. This is unlikely to matter in practice because
    // we'll be calling `set` much more frequently than `get`.
    void set (const juce::AudioPlayHead::PositionInfo& newInfo)
    {
        const juce::SpinLock::ScopedTryLockType lock (mutex);

        if (lock.isLocked())
            info = newInfo;
    }

    juce::AudioPlayHead::PositionInfo get() const noexcept
    {
        const juce::SpinLock::ScopedLockType lock (mutex);
        return info;
    }

private:
    juce::SpinLock mutex;
    juce::AudioPlayHead::PositionInfo info;

    JUCE_LEAK_DETECTOR(SpinLockedPositionInfo)
};
