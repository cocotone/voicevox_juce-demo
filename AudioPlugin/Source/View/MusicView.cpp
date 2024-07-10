#include "MusicView.h"
#include "../PluginProcessor.h"

//==============================================================================
MusicView::MusicView(AudioPluginAudioProcessor& processor, juce::ValueTree& appState, juce::AudioTransportSource& transportSource, juce::AudioThumbnail& audioThumbnail)
    : processorRef(processor)
    , applicationState(appState)
    , transportSourceRef(transportSource)
    , audioThumbnailRef(audioThumbnail)
{
    audioThumbnail.addChangeListener(this);

    currentPositionMarker.setFill(juce::Colours::yellow.withAlpha(0.85f));
    addAndMakeVisible(currentPositionMarker);

    applicationState.addListener(this);

    valueIsSyncToHost.referTo(applicationState, "Player_IsSyncToHostTransport", nullptr);
    valueIsSyncToHost.forceUpdateOfCachedValue();

    startTimerHz(30);
}

MusicView::~MusicView()
{
    applicationState.removeListener(this);

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
    repaint();
}

//==============================================================================
void MusicView::mouseDown(const juce::MouseEvent& e)
{
    mouseDrag(e);
}

void MusicView::mouseDrag(const juce::MouseEvent& e)
{
    transportSourceRef.setPosition(juce::jmax(0.0, xToTime((double)e.x, transportSourceRef.getLengthInSeconds())));
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

void MusicView::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId)
{
    if (treeWhosePropertyHasChanged == applicationState)
    {
        if (propertyId.toString() == "Player_IsSyncToHostTransport")
        {
            valueIsSyncToHost.forceUpdateOfCachedValue();
        }
    }
}

//==============================================================================
void MusicView::timerCallback()
{
    updateCursorPosition();

    repaint();
}

//==============================================================================
float MusicView::timeToX(const double pointTime, const double totalTime) const
{
    return (float)getWidth() * (float)pointTime / (float)totalTime;
}

double MusicView::xToTime(const double x, const double totalTime) const
{
    return (x / (double)getWidth()) * totalTime;
}

bool MusicView::canMoveTransport() const noexcept
{
    return false;
}

//==============================================================================
void MusicView::updateCursorPosition()
{
    if (valueIsSyncToHost.get())
    {
        currentPositionMarker.setVisible(true);

        const double host_position_seconds = processorRef.getLastPositionInfo().getTimeInSeconds().orFallback(0.0);
        const double host_sync_audio_source_length_seconds = processorRef.getHostSyncAudioSourceLengthInSeconds();
        if (host_position_seconds > host_sync_audio_source_length_seconds)
        {
            return;
        }

        const auto cursor_x = timeToX(host_position_seconds, host_sync_audio_source_length_seconds);
        const auto cursor_width = 1.5f;
        const auto cursor_height = getHeight();
        const auto rect_marker = juce::Rectangle<float>(cursor_x - cursor_width / 2.0f, 0.0f, cursor_width, cursor_height);
        currentPositionMarker.setRectangle(rect_marker);

        return;
    }

    if (transportSourceRef.getLengthInSeconds() > 0.0)
    {
        currentPositionMarker.setVisible(true);

        const auto cursor_x = timeToX(transportSourceRef.getCurrentPosition(), transportSourceRef.getLengthInSeconds());
        const auto cursor_width = 1.5f;
        const auto cursor_height = getHeight();
        const auto rect_marker = juce::Rectangle<float>(cursor_x - cursor_width / 2.0f, 0.0f, cursor_width, cursor_height);
        currentPositionMarker.setRectangle(rect_marker);
    }
    else
    {
        currentPositionMarker.setVisible(false);
    }
}