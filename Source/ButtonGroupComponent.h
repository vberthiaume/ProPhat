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

class ButtonGroupComponent : public Component, public Button::Listener
{
public:

    ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames) :
        mainButton (mainButtonName, DrawableButton::ImageAboveTextLabel)
    {
        mainButton.addListener (this);

        ScopedPointer<Drawable> nonSelectedDrawable = Helpers::getDrawable (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);

        ScopedPointer<DrawableImage> selectedDrawable = new DrawableImage();
        Image image = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
        selectedDrawable->setImage (image);
        selectedDrawable->setBoundingBox (mainButton.getBounds().reduced (50).toFloat());

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

    void buttonClicked (Button* button) override
    {
        if (button == &mainButton)
            selectNextToggleButton();
    }

    //void paint (Graphics& g) override
    //{
    //    g.fillAll (Colours::red);
    //}

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto ogHeight = bounds.getHeight();

        auto mainButtonBounds = bounds.removeFromLeft (bounds.getWidth() / 3);
        mainButtonBounds.reduce (0, ogHeight / 5);
        mainButton.setBounds (mainButtonBounds);

        for (auto button : selectionButtons)
            button->setBounds (bounds.removeFromTop (ogHeight / selectionButtons.size()));
    }

private:
    void selectNextToggleButton()
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

    sBMP4LookAndFeel lf;

    DrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;
};