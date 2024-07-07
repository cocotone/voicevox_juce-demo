#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "View/JsonTreeItem.h"

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

    jsonTreeView = std::make_unique<juce::TreeView>();
    jsonTreeView->setColour(juce::TreeView::ColourIds::backgroundColourId, getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    jsonTreeView->setColour(juce::TreeView::ColourIds::backgroundColourId, juce::Colours::whitesmoke);
    jsonTreeView->setDefaultOpenness(true);
    jsonTreeView->setInterceptsMouseClicks(true, true);
    addAndMakeVisible(jsonTreeView.get());

    buttonUpdateVoicevoxMetas = std::make_unique<juce::TextButton>();
    buttonUpdateVoicevoxMetas->setButtonText("Update MetasJson");
    buttonUpdateVoicevoxMetas->onClick = [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        juce::String text_to_parse = safe_this->processorRef.getMetaJsonStringify();
        safe_this->jsonTreeView->setRootItem(nullptr);
        safe_this->jsonTreeViewItemRoot.reset(nullptr);

        juce::String error_message;
        safe_this->jsonTreeViewItemRoot.reset(rebuildJson(text_to_parse, error_message));

        if (error_message.isNotEmpty())
        {
        }
        else
        {
            safe_this->jsonTreeView->setRootItem(safe_this->jsonTreeViewItemRoot.get());
            safe_this->jsonTreeView->repaint();
        }
        };
    addAndMakeVisible(buttonUpdateVoicevoxMetas.get());

    speakerIdEditor = std::make_unique<juce::TextPropertyComponent>(valueSpeakerId, "Speaker ID", 4, false, true);
    addAndMakeVisible(speakerIdEditor.get());

    textEditor = std::make_unique<juce::TextEditor>();
    textEditor->setMultiLine(true);
    textEditor->setFont(textEditor->getFont().withPointHeight(20));
    addAndMakeVisible(textEditor.get());
    comboboxTalkSpeakerChoice = std::make_unique<juce::ComboBox>();
    comboboxTalkSpeakerChoice->onChange =
        [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto selected_speaker_identifier = safe_this->comboboxTalkSpeakerChoice->getItemText(safe_this->comboboxTalkSpeakerChoice->getSelectedItemIndex());
        safe_this->processorRef.getEditorState().setProperty("VoicevoxEngine_SelectedTalkSpeakerIdentifier", selected_speaker_identifier, nullptr);
        };
    addAndMakeVisible(comboboxTalkSpeakerChoice.get());

    buttonInvokeTalk = std::make_unique<juce::TextButton>();
    buttonInvokeTalk->setButtonText(
        juce::CharPointer_UTF8 ("\xe3\x81\x97\xe3\x82\x83\xe3\x81\xb9\xe3\x82\x8b\xe3\x82\x88")
    );
    buttonInvokeTalk->onClick = [safe_this = juce::Component::SafePointer(this)] {
        if(safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto speaker_id = (int)safe_this->valueSpeakerId.getValue();
        const auto text = safe_this->textEditor->getText();
        safe_this->processorRef.requestTextToSpeech(speaker_id, text);
        };
    addAndMakeVisible(buttonInvokeTalk.get());

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

        const auto speaker_id = (int)safe_this->valueSpeakerId.getValue();
        const auto text = safe_this->textEditor->getText();
        safe_this->processorRef.requestHumming(speaker_id, text);
        };
    addAndMakeVisible(buttonInvokeHumming.get());


    musicView = std::make_unique<MusicView>(processorRef.getAudioTransportSource(), processorRef.getAudioThumbnail());
    addAndMakeVisible(musicView.get());

    playerController = std::make_unique<PlayerController>(processorRef.getApplicationState());
    addAndMakeVisible(playerController.get());

    progressPanel = std::make_unique<ProgressPanel>();
    addChildComponent(progressPanel.get());

    processorRef.getEditorState().addListener(this);

    valueIsVoicevoxEngineTaskRunning.referTo(processorRef.getEditorState(), "VoicevoxEngine_IsTaskRunning", nullptr);
    valueIsVoicevoxEngineTaskRunning.forceUpdateOfCachedValue();

    valueIsVoicevoxEngineHasSpeakerListUpdated.referTo(processorRef.getEditorState(), "VoicevoxEngine_HasSpeakerListUpdated", nullptr);
    valueIsVoicevoxEngineHasSpeakerListUpdated.forceUpdateOfCachedValue();

    // Initial update
    updateView(true);

    setSize (800, 640);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    processorRef.getEditorState().removeListener(this);

    jsonTreeView->setRootItem(nullptr);
    jsonTreeViewItemRoot.reset(nullptr);
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
        auto bottom_pane = rect_area.removeFromBottom(200);
        auto property_pane = rect_area;
        auto progress_area = property_pane;

        auto action_select_pane = property_pane.removeFromBottom(160);
        {
            auto action_talk_pane = action_select_pane.removeFromLeft(action_select_pane.getWidth() * 0.5f);
            {
                buttonInvokeTalk->setBounds(action_talk_pane.removeFromBottom(80).reduced(8));
                comboboxTalkSpeakerChoice->setBounds(action_talk_pane.removeFromBottom(80).reduced(8));
            }

            auto action_humming_pane = action_select_pane;
            {
                buttonInvokeHumming->setBounds(action_humming_pane.removeFromBottom(80).reduced(8));
                comboboxHummingSpeakerChoice->setBounds(action_humming_pane.removeFromBottom(80).reduced(8));
            }
        }
        textEditor->setBounds(property_pane.reduced(8));

        playerController->setBounds(bottom_pane.removeFromLeft(160).reduced(8));
        musicView->setBounds(bottom_pane.reduced(8));

        progressPanel->setBounds(progress_area);
    }
    repaint();
}

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
                auto speaker_list = processorRef.getVoicevoxTalkSpeakerList();
                const auto last_combo_text = processorRef.getEditorState().getProperty("VoicevoxEngine_SelectedTalkSpeakerIdentifier").toString();
                comboboxTalkSpeakerChoice->clear(juce::dontSendNotification);
                comboboxTalkSpeakerChoice->addItemList(speaker_list, 1);
                comboboxTalkSpeakerChoice->setText(last_combo_text);
            }

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

void AudioPluginAudioProcessorEditor::updateView(bool isInitial)
{
    progressPanel->setVisible(valueIsVoicevoxEngineTaskRunning);

    if (isInitial)
    {
        {
            auto speaker_list = processorRef.getVoicevoxTalkSpeakerList();
            const auto last_combo_text = processorRef.getEditorState().getProperty("VoicevoxEngine_SelectedTalkSpeakerIdentifier").toString();
            comboboxTalkSpeakerChoice->clear(juce::dontSendNotification);
            comboboxTalkSpeakerChoice->addItemList(speaker_list, 1);
            comboboxTalkSpeakerChoice->setText(last_combo_text);
        }

        {
            auto speaker_list = processorRef.getVoicevoxHummingSpeakerList();
            const auto last_combo_text = processorRef.getEditorState().getProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier").toString();
            comboboxHummingSpeakerChoice->clear(juce::dontSendNotification);
            comboboxHummingSpeakerChoice->addItemList(speaker_list, 1);
            comboboxHummingSpeakerChoice->setText(last_combo_text);
        }
    }
}
