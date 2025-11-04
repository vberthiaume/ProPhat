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

#ifndef USE_SVG
#define USE_SVG 1
#endif

/** The main look and feel for the plugin. Implements all the drawXXX functions we need.
*   Most of these are just copies from juce::LookAndFeel_V4 so we can use our own font.
*/
class ProPhatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ProPhatLookAndFeel();

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
#if JUCE_MAC
        const auto factor { .7f };
#else
        const auto factor { .8f };
#endif
        return fonts->getRegularFont (juce::jmin (static_cast<float> (Constants::labelFontHeight), static_cast<float>(buttonHeight) * factor));
    }

    void drawCornerResizer (juce::Graphics&, int, int, bool, bool) override {}

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      float x, float y, float w, float h,
                      const bool ticked, const bool isEnabled,
                      const bool shouldDrawButtonAsHighlighted,
                      const bool shouldDrawButtonAsDown) override;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;

    void drawGroupComponentOutline (juce::Graphics& g, int width, int height, const juce::String& text,
                                    const juce::Justification& position, juce::GroupComponent& group) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawDrawableButton (juce::Graphics& g, juce::DrawableButton& button,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    juce::Image tickedButtonImage, untickedButtonImage;

#if USE_SVG
    std::unique_ptr<juce::Drawable> rotarySliderDrawableImage;
#else
    juce::Image rotarySliderImage;
#endif

    juce::SharedResourcePointer<SharedFonts> fonts;
};
