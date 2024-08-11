#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "View/JsonTreeItem.h"
#include "Exchange/RPC/TaskThread.h"

#include <cocotone_song_editor_formats/cocotone_song_editor_formats.h>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

#if JUCE_WINDOWS
    juce::String typeFaceName = "Meiryo UI";
    juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#elif JUCE_MAC
    juce::String typeFaceName = "Arial Unicode MS";
    juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
// #elif JUCE_LINUX
//   juce::String typeFaceName = "IPAGothic";
//   juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#endif

    songEditor = std::make_unique<cctn::song::SongEditor>();
    addAndMakeVisible(songEditor.get());

    songEditor->registerPositionInfoProvider(this);
    songEditor->registerSongDocumentEditor(processorRef.getSongDocumentEditor());

    comboboxHummingSpeakerChoice = std::make_unique<juce::ComboBox>();
    comboboxHummingSpeakerChoice->onChange =
        [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto selected_speaker_identifier = safe_this->comboboxHummingSpeakerChoice->getItemText(safe_this->comboboxHummingSpeakerChoice->getSelectedItemIndex());
        safe_this->processorRef.getEditorState().setProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier", selected_speaker_identifier, nullptr);
        };
    addAndMakeVisible(comboboxHummingSpeakerChoice.get());

    buttonInvokeHumming = std::make_unique<juce::TextButton>();
    buttonInvokeHumming->setButtonText(
        juce::CharPointer_UTF8("\xe3\x81\x86\xe3\x81\x9f\xe3\x81\x86\xe3\x82\x88")
    );
    buttonInvokeHumming->onClick = [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        safe_this->processorRef.requestSongWithSongEditorDocument(0);
        };
    addAndMakeVisible(buttonInvokeHumming.get());

    musicView = std::make_unique<MusicView>(processorRef, processorRef.getApplicationState(), processorRef.getAudioTransportSource(), processorRef.getAudioThumbnail());
    addAndMakeVisible(musicView.get());

    playerController = std::make_unique<PlayerController>(processorRef.getApplicationState());
    addAndMakeVisible(playerController.get());

    labelTimecodeDisplay = std::make_unique<juce::Label>();
    labelTimecodeDisplay->setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::plain));
    addAndMakeVisible(labelTimecodeDisplay.get());

    buttonTransportMenu = std::make_unique<juce::TextButton>();
    buttonTransportMenu->setButtonText("Transport Menu");
    buttonTransportMenu->onClick = 
        [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto options = juce::PopupMenu::Options()
            .withDeletionCheck(*safe_this.getComponent())
            .withTargetComponent(safe_this->buttonTransportMenu.get())
            ;

        safe_this->transportMenu = std::move(safe_this->processorRef.getTransportEmulator().createMenu());
        safe_this->transportMenu->showMenuAsync(options);
        };
    addAndMakeVisible(buttonTransportMenu.get());

    buttonInvokeSongDocumentExchange = std::make_unique<juce::TextButton>();
    buttonInvokeSongDocumentExchange->setButtonText("Exchange");
    buttonInvokeSongDocumentExchange->onClick =
        [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        safe_this->exchangeSongDocument();
        };
    addAndMakeVisible(buttonInvokeSongDocumentExchange.get());

    progressPanel = std::make_unique<ProgressPanel>();
    addChildComponent(progressPanel.get());

    processorRef.getEditorState().addListener(this);

    valueIsVoicevoxEngineTaskRunning.referTo(processorRef.getEditorState(), "VoicevoxEngine_IsTaskRunning", nullptr);
    valueIsVoicevoxEngineTaskRunning.forceUpdateOfCachedValue();

    valueIsVoicevoxEngineHasSpeakerListUpdated.referTo(processorRef.getEditorState(), "VoicevoxEngine_HasSpeakerListUpdated", nullptr);
    valueIsVoicevoxEngineHasSpeakerListUpdated.forceUpdateOfCachedValue();

    // Initial update
    updateView(true);

    setResizable(true, true);
    setSize (1280, 960);

    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    songEditor->unregisterPositionInfoProvider(this);
    songEditor->unregisterSongDocumentEditor(processorRef.getSongDocumentEditor());
    songEditor.reset();

    stopTimer();

    processorRef.getEditorState().removeListener(this);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto rect_area = getLocalBounds();

    {
        auto bottom_pane = rect_area.removeFromBottom(240);
        auto property_pane = rect_area;
        auto progress_area = property_pane;

        auto action_select_pane = property_pane.removeFromBottom(60);
        {
            auto action_humming_pane = action_select_pane;
            {
                const auto comp_width = action_humming_pane.getWidth() * 0.5f;
                comboboxHummingSpeakerChoice->setBounds(action_humming_pane.removeFromLeft(comp_width).reduced(8));
                buttonInvokeHumming->setBounds(action_humming_pane.removeFromRight(comp_width).reduced(8));
            }
        }
        songEditor->setBounds(property_pane.reduced(8));

        // Transport 
        {
            auto rect_transport = bottom_pane.removeFromBottom(60);

            // Exchange
            buttonInvokeSongDocumentExchange->setBounds(rect_transport.removeFromRight(120).reduced(8));

            buttonTransportMenu->setBounds(rect_transport.removeFromLeft(120).reduced(8));
            labelTimecodeDisplay->setBounds(rect_transport.reduced(8));
        }

        playerController->setBounds(bottom_pane.removeFromLeft(160).reduced(8));
        musicView->setBounds(bottom_pane.reduced(8));

        progressPanel->setBounds(progress_area);
    }
    repaint();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId)
{
    bool should_update_view = false;

    if (treeWhosePropertyHasChanged == processorRef.getEditorState())
    {
        if (propertyId == valueIsVoicevoxEngineTaskRunning.getPropertyID())
        {
            valueIsVoicevoxEngineTaskRunning.forceUpdateOfCachedValue();
            should_update_view = true;
        }

        if (propertyId == valueIsVoicevoxEngineHasSpeakerListUpdated.getPropertyID())
        {
            valueIsVoicevoxEngineHasSpeakerListUpdated.forceUpdateOfCachedValue();
            valueIsVoicevoxEngineHasSpeakerListUpdated.setValue(false, nullptr);

            {
                auto speaker_list = processorRef.getVoicevoxHummingSpeakerList();
                const auto last_combo_text = processorRef.getEditorState().getProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier").toString();
                comboboxHummingSpeakerChoice->clear(juce::dontSendNotification);
                comboboxHummingSpeakerChoice->addItemList(speaker_list, 1);
                comboboxHummingSpeakerChoice->setText(last_combo_text);
            }

            should_update_view = true;
        }
    }

    if (should_update_view)
    {
        updateView(false);
        repaint();
    }
}

//==============================================================================
void AudioPluginAudioProcessorEditor::timerCallback()
{
    updateTimecodeDisplay(processorRef.getLastPositionInfo());
}

//==============================================================================
std::optional<juce::AudioPlayHead::PositionInfo> AudioPluginAudioProcessorEditor::getPositionInfo()
{
    return processorRef.getLastPositionInfo();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::updateView(bool isInitial)
{
    progressPanel->setVisible(valueIsVoicevoxEngineTaskRunning);

    if (isInitial)
    {
        {
            auto speaker_list = processorRef.getVoicevoxHummingSpeakerList();
            const auto last_combo_text = processorRef.getEditorState().getProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier").toString();
            comboboxHummingSpeakerChoice->clear(juce::dontSendNotification);
            comboboxHummingSpeakerChoice->addItemList(speaker_list, 1);
            comboboxHummingSpeakerChoice->setText(last_combo_text);
        }
    }
}

 //==============================================================================
// quick-and-dirty function to format a timecode string
juce::String timeToTimecodeString (double seconds)
{
    auto millisecs = juce::roundToInt (seconds * 1000.0);
    auto absMillisecs = std::abs (millisecs);

    return juce::String::formatted ("%02d:%02d:%02d.%03d",
                                millisecs / 3600000,
                                (absMillisecs / 60000) % 60,
                                (absMillisecs / 1000)  % 60,
                                absMillisecs % 1000);
}

// quick-and-dirty function to format a bars/beats string
juce::String quarterNotePositionToBarsBeatsString (double quarterNotes, juce::AudioPlayHead::TimeSignature timeSignature)
{
    if (timeSignature.numerator == 0 || timeSignature.denominator == 0)
    {
        return "1|1|000";
    }

    auto quarterNotesPerBar = (timeSignature.numerator * 4 / timeSignature.denominator);
    auto beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * timeSignature.numerator;

    auto bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
    auto beat   = ((int) beats) + 1;
    auto ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

    return juce::String::formatted ("%d|%d|%03d", bar, beat, ticks);
}

// Updates the text in our position label.
void AudioPluginAudioProcessorEditor::updateTimecodeDisplay (const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    juce::MemoryOutputStream displayText;

    const auto timeSignature = positionInfo.getTimeSignature().orFallback (juce::AudioPlayHead::TimeSignature{});

    displayText << juce::String (positionInfo.getBpm().orFallback (120.0), 2) << " bpm, "
                << timeSignature.numerator << '/' << timeSignature.denominator
                << "  -  " << timeToTimecodeString (positionInfo.getTimeInSeconds().orFallback (0.0))
                << "  -  " << quarterNotePositionToBarsBeatsString (positionInfo.getPpqPosition().orFallback (0.0), timeSignature);

    if (positionInfo.getIsRecording())
    {
        displayText << "  (recording)";
    }
    else if (positionInfo.getIsPlaying())
    {
        displayText << "  (playing)";
    }

    labelTimecodeDisplay->setText (displayText.toString(), juce::dontSendNotification);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::exchangeSongDocument()
{
    if (processorRef.getSongDocumentEditor()->getCurrentDocument().has_value())
    {
        cctn::song::UtaFormatixTranspileTarget transpiler;
        const auto text_ufdata = transpiler.transpile(*processorRef.getSongDocumentEditor()->getCurrentDocument().value());
        juce::Logger::outputDebugString(text_ufdata);

        const auto tmp_file = juce::File::createTempFile(".ufdata");
        if (tmp_file.hasWriteAccess())
        {
            const auto os_tmp_file = tmp_file.createOutputStream();
            if(os_tmp_file.get() != nullptr && os_tmp_file->openedOk())
            {
                os_tmp_file->write(text_ufdata.toRawUTF8(), text_ufdata.getNumBytesAsUTF8());
                os_tmp_file->flush();
            }
        }

        processTaskThread = std::make_unique<cctn::song::exchange::ExchangeProcessingTask>(
            [this](const juce::File& inputFile, const juce::File& outputFile) {
                outputFile.startAsProcess();
            },
            tmp_file);
        processTaskThread->launchThread();
    }
}
