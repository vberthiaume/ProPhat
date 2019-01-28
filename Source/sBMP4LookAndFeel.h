/*
  ==============================================================================

    sBMP4LookAndFeel.h
    Created: 23 Jan 2019 4:44:40pm
    Author:  Haake

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Helpers.h"

class sBMP4LookAndFeel : public LookAndFeel_V4
{
public:
    sBMP4LookAndFeel();

    void drawTickBox (Graphics& g, Component& /*component*/,
                      float x, float y, float w, float h,
                      const bool ticked, const bool isEnabled,
                      const bool shouldDrawButtonAsHighlighted,
                      const bool shouldDrawButtonAsDown) override;

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle,
                           Slider& /*slider*/) override;

private:
    Image tickedButtonImage, untickedButtonImage, rotarySliderImage;
};