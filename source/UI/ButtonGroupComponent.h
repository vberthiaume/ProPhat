/*
  ==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2024 Vincent Berthiaume

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

    ~ButtonGroupComponent () { state.removeParameterListener (parameterID, this); }

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
