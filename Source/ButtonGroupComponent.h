/*
  ==============================================================================

    ButtonGroupComponent.h
    Created: 23 Jan 2019 4:41:28pm
    Author:  Haake

  ==============================================================================
*/
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Helpers.h"
#include "sBMP4LookAndFeel.h"
#include <memory>

class FilledDrawableButton : public DrawableButton
{
public:
    FilledDrawableButton (const String& buttonName, ButtonStyle buttonStyle) :
        DrawableButton (buttonName, buttonStyle)
    {
    }

    Rectangle<float> getImageBounds() const override
    {
        auto bounds = getLocalBounds().toFloat();
        auto half = bounds.getHeight() / 4;
        auto reduced = bounds.reduced (0, half);
        return reduced;
    }
};

class ButtonGroupComponent : public Component, public Button::Listener, public AudioProcessorValueTreeState::Listener
{
public:

    ButtonGroupComponent (AudioProcessorValueTreeState& state, const String& parameterID, std::unique_ptr<Selection> theSelection,
                          StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false);

    void buttonClicked (Button* button) override;

    void paint (Graphics& /*g*/) override {}

    void setSelection (int selectionIndex);

    void setSelectedButton (int selectedButtonId);

    void resized() override;

    void parameterChanged (const String& parameterID, float newValue) override;

protected:
    void selectNext();

    AudioProcessorValueTreeState& state;
    String parameterID;

    bool allowEmptySelection = false;

    sBMP4LookAndFeel lf;

    FilledDrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;

    std::unique_ptr<Selection> selection;
};
