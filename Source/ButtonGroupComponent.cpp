/*
  ==============================================================================

    ButtonGroupComponent.cpp
    Created: 23 Jan 2019 4:41:28pm
    Author:  Haake

  ==============================================================================
*/

#include "ButtonGroupComponent.h"

ButtonGroupComponent::ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames) :
    mainButton (mainButtonName, DrawableButton::ImageAboveTextLabel)
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

void ButtonGroupComponent::selectNextToggleButton()
{
    auto selected = -1;
    auto i = 0;
    for (; i < selectionButtons.size(); ++i)
    {
        if (selectionButtons[i]->getToggleState())
        {
            selected = i;
            break;
        }
    }

    if (selected == -1)
        selectionButtons[0]->setToggleState (true, sendNotification);
    else if (selected == selectionButtons.size() - 1)
        selectionButtons[selected]->setToggleState (false, sendNotification);
    else
        selectionButtons[++i]->setToggleState (true, sendNotification);
}
