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

    ScopedPointer<Drawable> nonSelectedDrawable = Helpers::getDrawable (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);
    ScopedPointer<Drawable> selectedDrawable = Helpers::getDrawable (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
    mainButton.setImages (nonSelectedDrawable, nonSelectedDrawable, selectedDrawable);

    addAndMakeVisible (mainButton);

    for (auto names : selectionButtonNames)
    {
        auto button = new ToggleButton (names);
        button->setRadioGroupId (oscShapeRadioGroupId);
        button->setLookAndFeel (&lf);
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
}

//@TODO at the end of this cycle, we should get to a 0 option which deactivates the oscillator
#if 0
void ButtonOscGroupComponent::selectNextToggleButton()
{
    auto prevSelected = OscShape::none;
    for (auto i = 0; i < selectionButtons.size(); ++i)
    {
        if (selectionButtons[i]->getToggleState())
        {
            prevSelected = OscShape (i + 1);
            break;
        }
    }

    /** 
        selectionButtons range is 0 (saw) to 3 (pulse)
        prevSelected range = 0 (none), 1 (saw) to 4 (pulse)
        curSelected range = 0 (none), 1 (saw) to 4 (pulse)
    */

    auto curSelected = OscShape::saw;
    if (prevSelected == OscShape::none)
    {
        //nothing was selected, select saw
        selectionButtons[0]->setToggleState (true, sendNotification);
    }
    else if (prevSelected == OscShape ((int) OscShape::total - 1))
    {
        //last shape was selected, select nothing if allowed or saw otherwise
        if (allowEmptySelection)
        {
            selectionButtons[(int) prevSelected - 1]->setToggleState (false, sendNotification);
            curSelected = OscShape::none;
        }
        else
        {
            selectionButtons[0]->setToggleState (true, sendNotification);
        }        
    }
    else
    {
        //selection is a shape that's not the first nor the last, just increase
        curSelected = OscShape ((int) prevSelected + 1);
        selectionButtons[(int) curSelected - 1]->setToggleState (true, sendNotification);
    }

    setSelectedId ((int) curSelected);
}
#else
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

    selectionButtons[(int) selected - 1]->setToggleState (true, sendNotification);
    setSelectedId ((int) selected);
}
#endif



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

    selectionButtons[(int) selected - 1]->setToggleState (true, sendNotification);
    setSelectedId ((int) selected);
}
