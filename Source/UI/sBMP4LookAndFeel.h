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

#pragma once
#include "../Helpers.h"

#ifndef USE_SVG
#define USE_SVG 1
#endif

/** The main look and feel for the plugin. Implements all the drawXXX functions we need.
*   Most of these are just copies from juce::LookAndFeel_V4 so we can use our own font.
*/
class sBMP4LookAndFeel : public juce::LookAndFeel_V4
{
public:
    sBMP4LookAndFeel();

    juce::Font getRegularFont (float h) { return sharedFonts->regular.withHeight (h); }

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

    juce::SharedResourcePointer<SharedFonts> sharedFonts;
};
