#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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