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
#pragma once

#include "../Utility/Helpers.h"
#include <memory>

enum defaults
{
    defaultOscShape = (int) OscShape::saw,
    defaultLfoShape = (int) LfoShape::triangle,
    defaultLfoDest = (int) LfoDest::filterCutOff
};

class FilledDrawableButton : public juce::DrawableButton
{
public:
    FilledDrawableButton (const juce::String& buttonName, ButtonStyle buttonStyle) :
        juce::DrawableButton (buttonName, buttonStyle)
    {
    }

    juce::Rectangle<float> getImageBounds() const override
    {
        auto bounds = getLocalBounds().toFloat();
        auto half = bounds.getHeight() / 4;
        auto reduced = bounds.reduced (0, half);
        reduced.translate (0.f, -half / 2);
        return reduced;
    }
};

//=====================================================================

/** A component with a main button on the left that toggles between a series of selectionButtons
*   (which are juce::ToggleButtons) on the right. Selection buttons are also selectable directly
    by clicking on them.
*/
class ButtonGroupComponent : public juce::Component
                           , public juce::Button::Listener
                           , public juce::AudioProcessorValueTreeState::Listener
{
public:

    ButtonGroupComponent (juce::AudioProcessorValueTreeState& state, const juce::String& parameterID, std::unique_ptr<Selection> theSelection,
                          juce::StringRef mainButtonName, juce::Array<juce::StringRef> selectionButtonNames, bool allowEmpty = false);

    void buttonClicked (juce::Button* button) override;

    void paint (juce::Graphics& /*g*/) override {}

    void setSelection (int selectionIndex);

    void setSelectedButton (int selectedButtonId);

    void resized() override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

protected:
    void selectNext();

    juce::AudioProcessorValueTreeState& state;
    juce::String parameterID;

    FilledDrawableButton mainButton;
    juce::OwnedArray<juce::ToggleButton> selectionButtons;
    std::unique_ptr<Selection> selection;
    bool allowEmptySelection = false;
};
