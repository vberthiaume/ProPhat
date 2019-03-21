/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   sBMP4 IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "ButtonGroupComponent.h"

ButtonGroupComponent::ButtonGroupComponent (AudioProcessorValueTreeState& processorState, const String& theParameterID, std::unique_ptr<Selection> theSelection,
                                            StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty) :
    state (processorState),
    parameterID (theParameterID),
    mainButton (mainButtonName, DrawableButton::ImageAboveTextLabel),
    selection (std::move (theSelection)),
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
        button->setRadioGroupId (Constants::oscShapeRadioGroupId);
        button->setLookAndFeel (&lf);
        button->addListener (this);
        selectionButtons.add (button);
        addAndMakeVisible (button);
    }

    state.addParameterListener (parameterID, this);
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
    auto incr = selection->isNullSelectionAllowed() ? 1 : 0;

    if (button == &mainButton)
        selectNext();
    else if (! button->getToggleState())
        return;
    else
        for (int i = 0; i < selectionButtons.size(); ++i)
            if (selectionButtons[i] == button)
                setSelection (i + incr);
}
/**
    A selectionIndex of 0 means no selection. A selection of 1 is the first one
*/
void ButtonGroupComponent::setSelection (int selectionIndex)
{
    if (auto* p = state.getParameter (parameterID))
    {
        const float newValue = state.getParameterRange (parameterID).convertTo0to1 ((float) selectionIndex);

        if (p->getValue() != newValue)
            p->setValueNotifyingHost (newValue);
    }
}

void ButtonGroupComponent::setSelectedButton (int selectedButtonId)
{
    if (selection->isNullSelectionAllowed() && --selectedButtonId < 0)
        return; //valid null selection, so simply return

    if (selectedButtonId < 0 || selectedButtonId > selection->getLastSelectionIndex())
    {
        jassertfalse;
        selectedButtonId = 0;
    }

    selectionButtons[selectedButtonId]->setToggleState (true, dontSendNotification);
}

void ButtonGroupComponent::selectNext()
{
    auto curSelection = 0;
    auto incr = selection->isNullSelectionAllowed() ? 1 : 0;
    for (auto i = 0; i < selectionButtons.size(); ++i)
    {
        if (selectionButtons[i]->getToggleState())
        {
            curSelection = i + incr;
            break;
        }
    }

    if (curSelection == selection->getLastSelectionIndex())
        curSelection = 0;
    else
        ++curSelection;

    setSelection (curSelection);
}

void ButtonGroupComponent::parameterChanged (const String& theParameterID, float newValue)
{
    if (parameterID != theParameterID)
        return;

    auto newSelection = (int) newValue;

    //@TODO instead of looping here, we could simply cache whichever button was selected and toggle it off
    if (selection->isNullSelectionAllowed())
    {
        if (newSelection == 0)
            for (auto button : selectionButtons)
                button->setToggleState (false, dontSendNotification);
        else
            selectionButtons[newSelection - 1]->setToggleState (true, dontSendNotification);
    }
    else
    {
        selectionButtons[newSelection]->setToggleState (true, dontSendNotification);
    }
}
