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
    sBMP4LookAndFeel()
    {
        tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
        untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);
        rotarySliderImage = Helpers::getImage (BinaryData::metalKnob_png, BinaryData::metalKnob_pngSize);
    }

    void drawTickBox (Graphics& g, Component& /*component*/,
                                      float x, float y, float w, float h,
                                      const bool ticked,
                                      const bool isEnabled,
                                      const bool shouldDrawButtonAsHighlighted,
                                      const bool shouldDrawButtonAsDown) override
    {
        ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        Rectangle<float> tickBounds (x, y, w, h);
        tickBounds.reduce (0, h / 6);

        if (ticked)
            g.drawImage (tickedButtonImage, tickBounds);
        else
            g.drawImage (untickedButtonImage, tickBounds);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       const float rotaryStartAngle, const float rotaryEndAngle, Slider& /*slider*/) override
    {
        const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
        const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);


        //@TODO: this is a joke! Proper way to rotate and translated using non-random values is needed. The slider is not centered.
        const auto translationRatio = .2f;
        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (.05f)
                                                            .translated (translationRatio * width, 0)
                                                            .rotated (toAngle, bounds.getCentreX() - 2.f, bounds.getCentreY() + 5.2f));

        //g.setColour (Colours::red);
        //g.drawRect (bounds);
    }

private:
    Image tickedButtonImage, untickedButtonImage, rotarySliderImage;
};