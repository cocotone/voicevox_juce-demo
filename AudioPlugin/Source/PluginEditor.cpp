#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{

//==============================================================================
class JsonTreeItem
    : public juce::TreeViewItem
{
public:
    JsonTreeItem(juce::Identifier i, juce::var value)
        : identifier(i),
        json(value)
    {}

    juce::String getUniqueName() const override
    {
        return identifier.toString() + "_id";
    }

    bool mightContainSubItems() override
    {
        if (auto* obj = json.getDynamicObject())
            return obj->getProperties().size() > 0;

        return json.isArray();
    }

    void paintItem(juce::Graphics& g, int width, int height) override
    {
        // if this item is selected, fill it with a background colour..
        if (isSelected())
            g.fillAll(juce::Colours::blue.withAlpha(0.3f));

        g.setColour(juce::Colours::black);
        g.setFont((float)height * 0.7f);

        // draw the element's tag name..
        g.drawText(getText(),
            4, 0, width - 4, height,
            juce::Justification::centredLeft, true);
    }

    void itemOpennessChanged(bool isNowOpen) override
    {
        if (isNowOpen)
        {
            // if we've not already done so, we'll now add the tree's sub-items. You could
            // also choose to delete the existing ones and refresh them if that's more suitable
            // in your app.
            if (getNumSubItems() == 0)
            {
                // create and add sub-items to this node of the tree, corresponding to
                // the type of object this var represents

                if (json.isArray())
                {
                    for (int i = 0; i < json.size(); ++i)
                    {
                        auto& child = json[i];
                        jassert(!child.isVoid());
                        addSubItem(new JsonTreeItem({}, child));
                    }
                }
                else if (auto* obj = json.getDynamicObject())
                {
                    auto& props = obj->getProperties();

                    for (int i = 0; i < props.size(); ++i)
                    {
                        auto id = props.getName(i);

                        auto child = props[id];
                        jassert(!child.isVoid());

                        addSubItem(new JsonTreeItem(id, child));
                    }
                }
            }
        }
        else
        {
            // in this case, we'll leave any sub-items in the tree when the node gets closed,
            // though you could choose to delete them if that's more appropriate for
            // your application.
        }
    }

private:
    juce::Identifier identifier;
    juce::var json;

    /** Returns the text to display in the tree.
        This is a little more complex for JSON than XML as nodes can be strings, objects or arrays.
        */
    juce::String getText() const
    {
        juce::String text;

        if (identifier.isValid())
            text << identifier.toString();

        if (!json.isVoid())
        {
            if (text.isNotEmpty() && (!json.isArray()))
                text << ": ";

            if (json.isObject() && (!identifier.isValid()))
                text << "[Array]";
            else if (!json.isArray())
                text << json.toString();
        }

        return text;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonTreeItem)
};

/** Parses the editor's contents as JSON. */
juce::TreeViewItem* rebuildJson(const juce::String& text, juce::String& errorMessage)
{
    juce::var parsedJson;
    auto result = juce::JSON::parse(text, parsedJson);

    if (!result.wasOk())
    {
        errorMessage = juce::String("Error parsing JSON: ") + result.getErrorMessage();
        return nullptr;
    }

    return new JsonTreeItem(juce::Identifier(), parsedJson);
}

}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

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
    textEditor->setFont(textEditor->getFont().withPointHeight(12));
    addAndMakeVisible(textEditor.get());

    buttonInvokeSynthesis = std::make_unique<juce::TextButton>();
    buttonInvokeSynthesis->setButtonText(
        juce::CharPointer_UTF8 ("\xe3\x81\x86\xe3\x81\x9f\xe3\x81\x86\xe3\x82\x88")
    );
    buttonInvokeSynthesis->onClick = [safe_this = juce::Component::SafePointer(this)] {
        if(safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto speaker_id = (int)safe_this->valueSpeakerId.getValue();
        const auto text = safe_this->textEditor->getText();
        safe_this->processorRef.requestSynthesis(speaker_id, text);
        };
    addAndMakeVisible(buttonInvokeSynthesis.get());

    buttonInvokeTTS = std::make_unique<juce::TextButton>();
    buttonInvokeTTS->setButtonText(
        juce::CharPointer_UTF8 ("\xe3\x81\x97\xe3\x82\x83\xe3\x81\xb9\xe3\x82\x8b\xe3\x82\x88")
    );
    buttonInvokeTTS->onClick = [safe_this = juce::Component::SafePointer(this)] {
        if(safe_this.getComponent() == nullptr)
        {
            return;
        }

        const auto speaker_id = (int)safe_this->valueSpeakerId.getValue();
        const auto text = safe_this->textEditor->getText();
        safe_this->processorRef.requestTextToSpeech(speaker_id, text);
        };
    addAndMakeVisible(buttonInvokeTTS.get());

    musicView = std::make_unique<MusicView>(processorRef.getAudioTransportSource(), processorRef.getAudioThumbnail());
    addAndMakeVisible(musicView.get());

    playerController = std::make_unique<PlayerController>(processorRef.getApplicationState());
    addAndMakeVisible(playerController.get());

    setSize (800, 640);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
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
        auto left_pane = rect_area.removeFromLeft(320);
        auto right_pane = rect_area;

        buttonUpdateVoicevoxMetas->setBounds(left_pane.removeFromBottom(80).reduced(8));
        jsonTreeView->setBounds(left_pane.reduced(8));

        buttonInvokeTTS->setBounds(right_pane.removeFromBottom(80).reduced(8));
        //buttonInvokeSynthesis->setBounds(right_pane.removeFromBottom(80).reduced(8));
        speakerIdEditor->setBounds(right_pane.removeFromBottom(80).reduced(8));
        textEditor->setBounds(right_pane.reduced(8));

        playerController->setBounds(bottom_pane.removeFromLeft(160).reduced(8));
        musicView->setBounds(bottom_pane.reduced(8));
    }

}
