/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace
{
constexpr auto verticalGap { 5.f };
}

#include "ButtonGroupComponent.h"

ButtonGroupComponent::ButtonGroupComponent (juce::AudioProcessorValueTreeState& processorState,
                                            const juce::String& theParameterID,
                                            std::unique_ptr<Selection> theSelection,
                                            juce::StringRef mainButtonName,
                                            juce::Array<juce::StringRef> selectionButtonNames,
                                            bool allowEmpty) :
    state (processorState),
    parameterID (theParameterID),
    mainButton (mainButtonName, juce::DrawableButton::ImageAboveTextLabel),
    selection (std::move (theSelection)),
    allowEmptySelection (allowEmpty)
{
    mainButton.addListener (this);

    auto nonSelectedDrawable = Helpers::getDrawable (BinaryData::blackSquare_svg, BinaryData::blackSquare_svgSize);
    auto selectedDrawable = Helpers::getDrawable (BinaryData::redSquare_svg, BinaryData::redSquare_svgSize);
    mainButton.setImages (nonSelectedDrawable.get(), nonSelectedDrawable.get(), selectedDrawable.get());

    addAndMakeVisible (mainButton);

    for (auto names : selectionButtonNames)
    {
        auto button = new juce::ToggleButton (names);
        button->setRadioGroupId (Constants::oscShapeRadioGroupId);
        button->addListener (this);
        selectionButtons.add (button);
        addAndMakeVisible (button);
    }

    //adding this class as a parameter listener so we get callbacks in parameterChanged()
    //when the associated parameterID changes.
    state.addParameterListener (parameterID, this);
}

void ButtonGroupComponent::resized()
{
    auto bounds { getLocalBounds().toFloat()};
    const auto ogHeight { bounds.getHeight() };

    //position main button
    auto mainButtonBounds = bounds.removeFromLeft (bounds.getWidth() / 3.f);
    mainButtonBounds.reduce (0, ogHeight / 5.f);
    mainButton.setBounds (mainButtonBounds.toNearestInt());

    //position selection buttons
    bounds.reduce (0.f, verticalGap);
    const auto buttonH { (ogHeight - 2 * verticalGap) / selectionButtons.size () };
    for (auto button : selectionButtons)
        button->setBounds (bounds.removeFromTop (buttonH).toNearestInt ());
}

void ButtonGroupComponent::buttonClicked (juce::Button* button)
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

/** A selectionIndex of 0 means no selection. A selection of 1 is the first one
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

    selectionButtons[selectedButtonId]->setToggleState (true, juce::dontSendNotification);
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

void ButtonGroupComponent::parameterChanged (const juce::String& theParameterID, float newValue)
{
    jassert (parameterID == theParameterID);

    juce::Component::SafePointer safePtr { this };
    juce::MessageManager::callAsync ([safePtr, newSelection = static_cast<int> (newValue)] ()
    {
        if (! safePtr)
            return;

        //@TODO instead of looping here, we could simply cache whichever button was selected and toggle it off
        if (! safePtr->selection->isNullSelectionAllowed ())
        {
            safePtr->selectionButtons[newSelection]->setToggleState (true, juce::dontSendNotification);
            return;
        }

        if (newSelection == 0)
            for (auto button : safePtr->selectionButtons)
                button->setToggleState (false, juce::dontSendNotification);
        else
            safePtr->selectionButtons[newSelection - 1]->setToggleState (true, juce::dontSendNotification);
    });
}
