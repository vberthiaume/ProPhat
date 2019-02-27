/*
  ==============================================================================

    ButtonGroupComponent.cpp
    Created: 23 Jan 2019 4:41:28pm
    Author:  Haake

  ==============================================================================
*/

#include "ButtonGroupComponent.h"

ButtonGroupComponent::ButtonGroupComponent (AudioProcessorValueTreeState& processorState, const String& theParameterID, std::unique_ptr<Selection> theSelection,
                                            StringRef mainButtonName, Array<StringRef> selectionButtonNames, bool allowEmpty) :
    state (processorState),
    parameterID (theParameterID),
    selection (std::move (theSelection)),
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
    auto index = selectedButtonId - (selection->isNullSelectionAllowed() ? 1 : 0);

    if (index < 0 || index > selection->getLastSelectionIndex())
    {
        jassertfalse;
        index = 0;
    }

    selectionButtons[index]->setToggleState (true, dontSendNotification);
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
