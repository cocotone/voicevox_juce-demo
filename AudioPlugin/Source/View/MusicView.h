#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// MusicView
//==============================================================================
class MusicView final
    : public juce::Component
    , private juce::ChangeListener
    , private juce::Timer
{
public:
    //==============================================================================
    MusicView(juce::AudioTransportSource& transportSource, juce::AudioThumbnail& audioThumbnail);
    ~MusicView() override;

private:
    //==============================================================================
    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::Component
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // juce::Timer
    void timerCallback() override;

    //==============================================================================
    float timeToX(const double pointTime, const double totalTime) const;
    double xToTime(const double x, const double totalTime) const;
    bool canMoveTransport() const noexcept;
    void updateCursorPosition();

    //==============================================================================
    juce::AudioTransportSource& transportSourceRef;
    juce::AudioThumbnail& audioThumbnailRef;

    juce::DrawableRectangle currentPositionMarker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicView)
};

