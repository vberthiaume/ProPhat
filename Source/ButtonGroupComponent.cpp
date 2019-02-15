/*
  ==============================================================================

    ButtonGroupComponent.cpp
    Created: 23 Jan 2019 4:41:28pm
    Author:  Haake

  ==============================================================================
*/

#include "ButtonGroupComponent.h"

ButtonGroupComponent::ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty) :
    mainButton (mainButtonName, DrawableButton::ImageAboveTextLabel),
    allowEmptySelection (allowEmpty)
{
    mainButton.addListener (this);

    ScopedPointer<Drawable> nonSelectedDrawable = Helpers::getDrawable (BinaryData::blackLight_svg, BinaryData::blackLight_svgSize);
    ScopedPointer<Drawable> selectedDrawable = Helpers::getDrawable (BinaryData::redLight_svg, BinaryData::redLight_svgSize);
    mainButton.setImages (nonSelectedDrawable, nonSelectedDrawable, selectedDrawable);

    addAndMakeVisible (mainButton);

    for (auto names : selectionButtonNames)
    {
        auto button = new ToggleButton (names);
        button->setRadioGroupId (oscShapeRadioGroupId);
        button->setLookAndFeel (&lf);
        button->addListener (this);
        selectionButtons.add (button);
        addAndMakeVisible (button);
    }
}

void ButtonGroupComponent::resized()
{
    auto bounds = getLocalBounds();
    auto ogHeight = bounds.getHeight();

    auto mainButtonBounds = bounds.removeFromLeft (bounds.getWidth() / 3);
    mainButtonBounds.reduce (0, ogHeight / 5);
    mainButton.setBounds (mainButtonBounds);

    for (auto button : selectionButtons)
        button->setBounds (bounds.removeFromTop (ogHeight / selectionButtons.size()));
}

void ButtonGroupComponent::buttonClicked (Button* button)
{
    if (button == &mainButton)
        selectNextToggleButton();
    else if (! button->getToggleState())
        return;
    else
        for (int i = 0; i < selectionButtons.size(); ++i)
            if (selectionButtons[i] == button)
                selectToggleButton (i);
}

//=========================================================================

ButtonOscGroupComponent::ButtonOscGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty) :
    ButtonGroupComponent (mainButtonName, selectionButtonNames, allowEmpty)
{
    addItem (sBMP4AudioProcessorChoices::oscShape0, (int) OscShape::saw);
    addItem (sBMP4AudioProcessorChoices::oscShape1, (int) OscShape::sawTri);
    addItem (sBMP4AudioProcessorChoices::oscShape2, (int) OscShape::triangle);
    addItem (sBMP4AudioProcessorChoices::oscShape3, (int) OscShape::pulse);

    setShape (OscShape::saw);
}

void ButtonOscGroupComponent::selectToggleButton (int buttonIndex)
{
    setShape (OscShape (buttonIndex + 1));
}

void ButtonOscGroupComponent::setShape (OscShape shape)
{
    //select the right button and shape
    switch (shape)
    {
        case OscShape::saw:
        case OscShape::sawTri:
        case OscShape::triangle:
        case OscShape::pulse:
            selectionButtons[(int) shape - 1]->setToggleState (true, dontSendNotification);
            setSelectedId ((int) shape);
            break;

        case OscShape::none:
        case OscShape::total:
        default:
            jassertfalse;
            break;
    }
}

//@TODO at the end of this cycle, we should get to a 0 option which deactivates the oscillator
void ButtonOscGroupComponent::selectNextToggleButton()
{
    auto selected = OscShape::none;
    for (auto i = 0; i < selectionButtons.size(); ++i)
    {
        if (selectionButtons[i]->getToggleState())
        {
            selected = OscShape (i + 1);
            break;
        }
    }

    if (selected == OscShape::none || selected == OscShape::pulse)
        selected = OscShape::saw;
    else
        selected = OscShape ((int) selected + 1);

    selectionButtons[(int) selected - 1]->setToggleState (true, dontSendNotification);
    setSelectedId ((int) selected);
}

//=========================================================================

ButtonLfoGroupComponent::ButtonLfoGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty) :
    ButtonGroupComponent (mainButtonName, selectionButtonNames, allowEmpty)
{
    addItem (sBMP4AudioProcessorChoices::lfoShape0, (int) LfoShape::triangle);
    addItem (sBMP4AudioProcessorChoices::lfoShape1, (int) LfoShape::saw);
    addItem (sBMP4AudioProcessorChoices::lfoShape2, (int) LfoShape::revSaw);
    addItem (sBMP4AudioProcessorChoices::lfoShape3, (int) LfoShape::square);
    addItem (sBMP4AudioProcessorChoices::lfoShape3, (int) LfoShape::random);

    setShape (LfoShape::triangle);
}

void ButtonLfoGroupComponent::selectToggleButton (int buttonIndex)
{
    setShape (LfoShape (buttonIndex + 1));
}

void ButtonLfoGroupComponent::setShape (LfoShape shape)
{
    switch (shape)
    {
        case LfoShape::triangle:
        case LfoShape::saw:
        case LfoShape::revSaw:
        case LfoShape::square:
        case LfoShape::random:
            selectionButtons[(int) shape - 1]->setToggleState (true, dontSendNotification);
            setSelectedId ((int) shape);
            break;

        case LfoShape::none:
        case LfoShape::total:
        default:
            jassertfalse;
            break;
    }
}

void ButtonLfoGroupComponent::selectNextToggleButton()
{
    auto selected = LfoShape::none;
    for (auto i = 0; i < selectionButtons.size(); ++i)
    {
        if (selectionButtons[i]->getToggleState())
        {
            selected = LfoShape (i + 1);
            break;
        }
    }

    if (selected == LfoShape::none || selected == LfoShape::random)
        selected = LfoShape::triangle;
    else
        selected = LfoShape ((int) selected + 1);

    selectionButtons[(int) selected - 1]->setToggleState (true, dontSendNotification);
    setSelectedId ((int) selected);
}
