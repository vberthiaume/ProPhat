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

    bool allowEmptySelection = false;

    sBMP4LookAndFeel lf;

    FilledDrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;
};

class ButtonOscGroupComponent : public ButtonGroupComponent
{
public:
    ButtonOscGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false) :
        ButtonGroupComponent (mainButtonName, selectionButtonNames, allowEmpty)
    {
        addItem (sBMP4AudioProcessorChoices::oscShape0, (int) OscShape::saw);
        addItem (sBMP4AudioProcessorChoices::oscShape1, (int) OscShape::sawTri);
        addItem (sBMP4AudioProcessorChoices::oscShape2, (int) OscShape::triangle);
        addItem (sBMP4AudioProcessorChoices::oscShape3, (int) OscShape::pulse);

        selectNextToggleButton();
    }

    void setShape (OscShape shape)
    {
        switch (shape)
        {
            case OscShape::saw:
            case OscShape::sawTri:
            case OscShape::triangle:
            case OscShape::pulse:
                selectionButtons[(int) shape - 1]->setToggleState (true, sendNotification);
                setSelectedId ((int) shape);
                break;

            case OscShape::none:
                for (auto button : selectionButtons)
                    button->setToggleState (false, sendNotification);

                setSelectedId ((int) shape);
                break;
            case OscShape::total:
                jassertfalse;
                break;
            default:
                break;
        }
    }

protected:
    virtual void selectNextToggleButton() override;
};

class ButtonLfoGroupComponent : public ButtonGroupComponent
{
public:
    ButtonLfoGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty = false) :
        ButtonGroupComponent (mainButtonName, selectionButtonNames, allowEmpty)
    {
        addItem (sBMP4AudioProcessorChoices::lfoShape0, (int) LfoShape::triangle);
        addItem (sBMP4AudioProcessorChoices::lfoShape1, (int) LfoShape::saw);
        addItem (sBMP4AudioProcessorChoices::lfoShape2, (int) LfoShape::revSaw);
        addItem (sBMP4AudioProcessorChoices::lfoShape3, (int) LfoShape::square);
        addItem (sBMP4AudioProcessorChoices::lfoShape3, (int) LfoShape::random);

        selectNextToggleButton();
    }

    void setShape (LfoShape shape)
    {
        switch (shape)
        {
            case LfoShape::triangle:
            case LfoShape::saw:
            case LfoShape::revSaw:
            case LfoShape::square:
            case LfoShape::random:
                selectionButtons[(int) shape - 1]->setToggleState (true, sendNotification);
                setSelectedId ((int) shape);
                break;

            case LfoShape::none:
                for (auto button : selectionButtons)
                    button->setToggleState (false, sendNotification);

                setSelectedId ((int) shape);
                break;
            case LfoShape::total:
                jassertfalse;
                break;
            default:
                break;
        }

    }

protected:
    virtual void selectNextToggleButton() override;
};