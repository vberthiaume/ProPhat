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

class ButtonGroupComponent : public ComboBox, public Button::Listener
{
public:

    ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false);

    void buttonClicked (Button* button) override;

    void paint (Graphics& /*g*/) override {}

    void resized() override;

protected:
    virtual void selectNextToggleButton() = 0;
    virtual void selectToggleButton (int buttonIndex) = 0;

    bool allowEmptySelection = false;

    sBMP4LookAndFeel lf;

    FilledDrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;
};

class ButtonOscGroupComponent : public ButtonGroupComponent
{
public:
    ButtonOscGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false);

    void setShape (OscShape shape);

protected:
    virtual void selectNextToggleButton() override;
    virtual void selectToggleButton (int buttonIndex) override;
};

class ButtonLfoGroupComponent : public ButtonGroupComponent
{
public:
    ButtonLfoGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false);
    void setShape (LfoShape shape);

protected:
    virtual void selectNextToggleButton() override;
    virtual void selectToggleButton (int buttonIndex) override;
};