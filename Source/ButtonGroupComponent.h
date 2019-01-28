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

class ButtonGroupComponent : public Component, public Button::Listener
{
public:

    ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames);

    void buttonClicked (Button* button) override;

    //void paint (Graphics& g) override
    //{
    //    g.fillAll (Colours::red);
    //}

    void resized() override;

private:
    void selectNextToggleButton();

    sBMP4LookAndFeel lf;

    FilledDrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;
};