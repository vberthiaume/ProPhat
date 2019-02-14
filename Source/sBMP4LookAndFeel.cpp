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
    /*tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);*/

    tickedButtonImage = Helpers::getImage (BinaryData::redLight_png, BinaryData::redLight_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackLight_png, BinaryData::blackLight_pngSize);

#if USE_SVG
    rotarySliderDrawableImage = Helpers::getDrawable (BinaryData::sBMP4Knob_svg, BinaryData::sBMP4Knob_svgSize);
#else
    //LOADING SVG USING DRAWABLES
    rotarySliderImage = Helpers::getImage (BinaryData::metalKnobFitted_png, BinaryData::metalKnobFitted_pngSize);
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
                                         const float rotaryStartAngle, const float rotaryEndAngle, Slider& /*slider*/)
{
    const auto gap = 6;
    x += gap;
    y += gap;
    width -= gap * 2;
    height -= gap * 2;

    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    float squareSide = 0.f, xTranslation = 0.f, yTranslation = 0.f;
    if (height < width)
    {
        squareSide = (float) height;
        xTranslation = (width - height) / 2.f;
    }
    else
    {
        squareSide = (float) width;
        yTranslation = (height - width) / 2.f;
    }

#if USE_SVG
    const auto imageBounds = rotarySliderDrawableImage->getBounds().toFloat();
    const auto scaleFactor = squareSide / imageBounds.getWidth();

    rotarySliderDrawableImage->draw (g, 1.f, AffineTransform::scale (scaleFactor).translated (xTranslation + gap, yTranslation + gap)
                                                             .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
#else
    auto imageBounds = rotarySliderImage.getBounds().toFloat();
    auto scaleFactor = squareSide / imageBounds.getWidth();

    g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (scaleFactor).translated (xTranslation + gap, yTranslation + gap)
                                                               .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
#endif

    //g.setColour (Colours::red);
    //g.drawRect (bounds);
}