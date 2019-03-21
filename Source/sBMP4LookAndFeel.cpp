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

#include "sBMP4LookAndFeel.h"

sBMP4LookAndFeel::sBMP4LookAndFeel()
{
    /*tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);*/

    tickedButtonImage = Helpers::getImage (BinaryData::redLight_png, BinaryData::redLight_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackLight_png, BinaryData::blackLight_pngSize);

#if USE_SVG
    rotarySliderDrawableImage = Helpers::getDrawable (BinaryData::sBMP4KnobUniform_svg, BinaryData::sBMP4KnobUniform_svgSize);
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
    auto outline = Colours::white;
    auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (10);

    //draw background
    {
        g.setColour (outline);
        Path path;

#if 0
        path.startNewSubPath (bounds.getX(), bounds.getCentreY());
        path.lineTo (bounds.getRight(), bounds.getCentreY());
#else
        auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto lineW = jmin (8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;
        path.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
#endif

        auto type = PathStrokeType (1.f, PathStrokeType::beveled, PathStrokeType::square);
        float lenghts[] = {1.f, 8.f};
        type.createDashedStroke (path, path, lenghts, 2);

        g.fillPath (path);
    }

    //draw slider itself
    {
        const auto gap = 20;
        x += gap;
        y += gap;
        width -= gap * 2;
        height -= gap * 2;

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
    }
}