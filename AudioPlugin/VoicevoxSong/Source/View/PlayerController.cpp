#include "PlayerController.h"

//==============================================================================
// PlayerController
//==============================================================================
PlayerController::PlayerController(juce::ValueTree& appState)
    : applicationState(appState)
{
    groupPlayerController = std::make_unique<juce::GroupComponent>();
    groupPlayerController->setText("Player Controller");
    groupPlayerController->setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(groupPlayerController.get());

    playButton = std::make_unique<juce::TextButton>();
    playButton->setButtonText("Play");
    playButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour(0xFF2F9E44));
    playButton->onClick =
        [safe_this = juce::Component::SafePointer(this)]() {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        safe_this->valueIsPlaying = !safe_this->valueIsPlaying.get();
        };
    addAndMakeVisible(playButton.get());

    loopButton = std::make_unique<juce::TextButton>();
    loopButton->setButtonText("Loop");
    loopButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour(0xFFFF7733));
    loopButton->onClick =
        [safe_this = juce::Component::SafePointer(this)]() {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        safe_this->valueIsLooping = !safe_this->valueIsLooping.get();
        };
    addAndMakeVisible(loopButton.get());

    syncToHostButton = std::make_unique<juce::ToggleButton>();
    syncToHostButton->setButtonText("Sync To Host");
    syncToHostButton->onClick =
        [safe_this = juce::Component::SafePointer(this)]() {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        safe_this->valueIsSyncToHost = !safe_this->valueIsSyncToHost.get();
        };
    addAndMakeVisible(syncToHostButton.get());

    applicationState.addListener(this);

    valueCanPlay.referTo(applicationState, "Player_CanPlay", nullptr);
    valueCanPlay.forceUpdateOfCachedValue();

    valueIsPlaying.referTo(applicationState, "Player_IsPlaying", nullptr);
    valueIsPlaying.forceUpdateOfCachedValue();

    valueIsLooping.referTo(applicationState, "Player_IsLooping", nullptr);
    valueIsLooping.forceUpdateOfCachedValue();

    valueIsSyncToHost.referTo(applicationState, "Player_IsSyncToHostTransport", nullptr);
    valueIsSyncToHost.forceUpdateOfCachedValue();

    // Initial update
    updateView();
}

PlayerController::~PlayerController()
{
    applicationState.removeListener(this);
}

//==============================================================================
void PlayerController::paint (juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void PlayerController::resized()
{
    auto area = getLocalBounds();

    groupPlayerController->setBounds(area.reduced(8));

    {
        auto rect_player_controller = groupPlayerController->getBounds().withTrimmedTop(8).reduced(8);
        const auto width = rect_player_controller.getWidth();
        const auto height = rect_player_controller.getHeight() / 3;
        const auto width_button = width;
        const auto height_button = height;
        playButton->setBounds(rect_player_controller.removeFromTop(height).withSizeKeepingCentre(width_button, height_button).reduced(8));
        loopButton->setBounds(rect_player_controller.removeFromTop(height).withSizeKeepingCentre(width_button, height_button).reduced(8));
        syncToHostButton->setBounds(rect_player_controller.removeFromTop(height).withSizeKeepingCentre(width_button, height_button).reduced(8));
    }
}

//==============================================================================
void PlayerController::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId)
{
    if (treeWhosePropertyHasChanged == applicationState)
    {
        if (propertyId.toString() == "Player_CanPlay")
        {
            valueCanPlay.forceUpdateOfCachedValue();
            playButton->setEnabled(valueCanPlay.get());
        }
        else if (propertyId.toString() == "Player_IsPlaying")
        {
            valueIsPlaying.forceUpdateOfCachedValue();
            playButton->setToggleState(valueIsPlaying.get(), juce::dontSendNotification);
        }
        else if (propertyId.toString() == "Player_IsLooping")
        {
            valueIsLooping.forceUpdateOfCachedValue();
            loopButton->setToggleState(valueIsLooping.get(), juce::dontSendNotification);
        }
        else if (propertyId.toString() == "Player_IsSyncToHostTransport")
        {
            valueIsSyncToHost.forceUpdateOfCachedValue();
            syncToHostButton->setToggleState(valueIsSyncToHost.get(), juce::dontSendNotification);
        }

        updateView();
    }
}

//==============================================================================
void PlayerController::updateView()
{
    playButton->setEnabled(valueCanPlay.get());
    loopButton->setEnabled(valueCanPlay.get());

    playButton->setToggleState(valueIsPlaying.get(), juce::dontSendNotification);
    loopButton->setToggleState(valueIsLooping.get(), juce::dontSendNotification);
    syncToHostButton->setToggleState(valueIsSyncToHost.get(), juce::dontSendNotification);

    if (valueIsSyncToHost.get())
    {
        playButton->setEnabled(false);
        loopButton->setEnabled(false);
    }
}
