/*
  ==============================================================================

    sBMP4LookAndFeel.cpp
    Created: 23 Jan 2019 4:44:40pm
    Author:  Haake

  ==============================================================================
*/

#include "sBMP4LookAndFeel.h"

sBMP4LookAndFeel::sBMP4LookAndFeel()
{
    tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);

#if 1
    rotarySliderImage = Helpers::getImage (BinaryData::metalKnobFitted_png, BinaryData::metalKnobFitted_pngSize);
#else
    //LOADING SVG USING DRAWABLES
    rotarySliderDrawableImage = Helpers::getDrawable (BinaryData::sBMP4Knob_svg, BinaryData::sBMP4Knob_svgSize);
#endif
}

void sBMP4LookAndFeel::drawTickBox (Graphics& g, Component& /*component*/,
                                    float x, float y, float w, float h,
                                    const bool ticked, const bool isEnabled,
                                    const bool shouldDrawButtonAsHighlighted,
                                    const bool shouldDrawButtonAsDown)
{
    ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    Rectangle<float> tickBounds (x, y, w, h);
    tickBounds.reduce (0, h / 6);

    if (ticked)
        g.drawImage (tickedButtonImage, tickBounds);
    else
        g.drawImage (untickedButtonImage, tickBounds);
}

void sBMP4LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

#if 1
    //USING TRIMMED PGN WITHOUT TRANSFORM (WOULD NEED TO TRANSFORM TO ROTATE)
    //g.drawImage (rotarySliderImage, bounds);


    auto imageBounds = rotarySliderImage.getBounds().toFloat();
    

    //get square side
    if (height < width)
    {
        auto squareSide = height;
        auto translation = (width - height) / 2.f;
        auto scaleFactor = squareSide / imageBounds.getWidth();

        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (scaleFactor)
                                                                  .translated (translation, 0)
                                                                  .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
    }
    else
    {
        auto squareSide = width;
        auto translation = (height - width) / 2.f;
        auto scaleFactor = squareSide / imageBounds.getWidth();

        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (scaleFactor)
                                                                   .translated (0, translation)
                                                                   .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
    }

#else
    ////LOADING SVG USING DRAWABLES
    auto sliderBounds = slider.getLocalBounds().withWidth (slider.getHeight()).toFloat();
    auto imageBounds = rotarySliderDrawableImage->getBounds().toFloat();
    auto xScaleFactor = sliderBounds.getWidth() / imageBounds.getWidth();
    auto yScaleFactor = sliderBounds.getHeight() / imageBounds.getHeight();

    rotarySliderDrawableImage->draw (g, 1.f, AffineTransform::scale (xScaleFactor, yScaleFactor)
                                                              .translated (sliderBounds.getX(), 0)
                                                              .rotated (toAngle, sliderBounds.getCentreX(), sliderBounds.getCentreY()));
        

    g.setColour (Colours::red);
    g.drawRect (bounds);

    g.setColour (Colours::blue);
    g.drawRect (sliderBounds);

#endif

    //g.setColour (Colours::red);
    //g.drawRect (bounds);
}