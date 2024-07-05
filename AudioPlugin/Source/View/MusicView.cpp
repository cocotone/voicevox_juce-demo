#include "MusicView.h"

//==============================================================================
MusicView::MusicView(juce::AudioTransportSource& transportSource, juce::AudioThumbnail& audioThumbnail)
    : transportSourceRef(transportSource)
    , audioThumbnailRef(audioThumbnail)
{
    audioThumbnail.addChangeListener(this);

    currentPositionMarker.setFill(juce::Colours::yellow.withAlpha(0.85f));
    addAndMakeVisible(currentPositionMarker);

    startTimerHz(30);
}

MusicView::~MusicView()
{
    audioThumbnailRef.removeChangeListener(this);
}

//==============================================================================
void MusicView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::lightblue);

    if (audioThumbnailRef.getTotalLength() > 0.0)
    {
        auto thumbArea = getLocalBounds();

        audioThumbnailRef.drawChannels(g, 
            thumbArea.reduced(2), 
            0.0, 
            audioThumbnailRef.getTotalLength() * audioThumbnailRef.getProportionComplete(), 
            1.0f);
    }
}

void MusicView::resized()
{
}

//==============================================================================
void MusicView::mouseDown(const juce::MouseEvent& e)
{
    mouseDrag(e);
}

void MusicView::mouseDrag(const juce::MouseEvent& e)
{
    transportSourceRef.setPosition(juce::jmax(0.0, xToTime((float)e.x)));
}

void MusicView::mouseUp(const juce::MouseEvent&)
{
    transportSourceRef.start();
}

void MusicView::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
}

//==============================================================================
void MusicView::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    
    // this method is called by the thumbnail when it has changed, so we should repaint it..
    repaint();
}

//==============================================================================
void MusicView::timerCallback()
{
    updateCursorPosition();
}

//==============================================================================
float MusicView::timeToX(const double pointTime, const double totalTime) const
{
    return (float)getWidth() * (float)pointTime / (float)totalTime;
}

double MusicView::xToTime(const float x) const
{
    return (x / (float)getWidth());
}

bool MusicView::canMoveTransport() const noexcept
{
    return false;
}

//==============================================================================
void MusicView::updateCursorPosition()
{
    if (transportSourceRef.getLengthInSeconds() > 0.0)
    {
        currentPositionMarker.setVisible(true);

        const auto rect_marker = juce::Rectangle<float>(timeToX(transportSourceRef.getCurrentPosition(), audioThumbnailRef.getTotalLength()) - 0.75f, 0, 1.5f, (float)(getHeight()));
        currentPositionMarker.setRectangle(rect_marker);
    }
    else
    {
        currentPositionMarker.setVisible(false);
    }
}