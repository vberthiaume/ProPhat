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

void sBMP4LookAndFeel::drawTickBox (juce::Graphics& g, juce::Component& /*component*/,
                                    float x, float y, float w, float h,
                                    const bool ticked, const bool isEnabled,
                                    const bool shouldDrawButtonAsHighlighted,
                                    const bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    juce::Rectangle<float> tickBounds (x, y, w, h);
    tickBounds.reduce (0, h / 6);

    if (ticked)
        g.drawImage (tickedButtonImage, tickBounds);
    else
        g.drawImage (untickedButtonImage, tickBounds);
}

void sBMP4LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& /*slider*/)
{
    auto outline = juce::Colours::white;
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);

    //draw background
    {
        g.setColour (outline);
        juce::Path path;

#if 0
        path.startNewSubPath (bounds.getX(), bounds.getCentreY());
        path.lineTo (bounds.getRight(), bounds.getCentreY());
#else
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto lineW = juce::jmin (8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;
        path.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
#endif

        auto type = juce::PathStrokeType (1.f, juce::PathStrokeType::beveled, juce::PathStrokeType::square);
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

        rotarySliderDrawableImage->draw (g, 1.f, juce::AffineTransform::scale (scaleFactor).translated (xTranslation + gap, yTranslation + gap)
                                                                 .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
#else
        auto imageBounds = rotarySliderImage.getBounds().toFloat();
        auto scaleFactor = squareSide / imageBounds.getWidth();

        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (scaleFactor).translated (xTranslation + gap, yTranslation + gap)
                                                                   .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
#endif
    }
}

void sBMP4LookAndFeel::drawGroupComponentOutline (juce::Graphics& g, int width, int height, const juce::String& text,
                                                  const juce::Justification& position, juce::GroupComponent& group)
{
    const float indent = 3.0f;
    const float textEdgeGap = 4.0f;
    auto cs = 5.0f;

    using namespace juce;

    Font f (sharedFonts->regular.withHeight (Constants::groupComponentFontHeight));

    Path p;
    auto x = indent;
    auto y = f.getAscent () - 3.0f;
    auto w = jmax (0.0f, (float) width - x * 2.0f);
    auto h = jmax (0.0f, (float) height - y - indent);
    cs = jmin (cs, w * 0.5f, h * 0.5f);
    auto cs2 = 2.0f * cs;

    auto textW = text.isEmpty () ? 0
        : jlimit (0.0f,
                  jmax (0.0f, w - cs2 - textEdgeGap * 2),
                  (float) f.getStringWidth (text) + textEdgeGap * 2.0f);
    auto textX = cs + textEdgeGap;

    if (position.testFlags (Justification::horizontallyCentred))
        textX = cs + (w - cs2 - textW) * 0.5f;
    else if (position.testFlags (Justification::right))
        textX = w - cs - textW - textEdgeGap;

    p.startNewSubPath (x + textX + textW, y);
    p.lineTo (x + w - cs, y);

    p.addArc (x + w - cs2, y, cs2, cs2, 0, MathConstants<float>::halfPi);
    p.lineTo (x + w, y + h - cs);

    p.addArc (x + w - cs2, y + h - cs2, cs2, cs2, MathConstants<float>::halfPi, MathConstants<float>::pi);
    p.lineTo (x + cs, y + h);

    p.addArc (x, y + h - cs2, cs2, cs2, MathConstants<float>::pi, MathConstants<float>::pi * 1.5f);
    p.lineTo (x, y + cs);

    p.addArc (x, y, cs2, cs2, MathConstants<float>::pi * 1.5f, MathConstants<float>::twoPi);
    p.lineTo (x + textX, y);

    auto alpha = group.isEnabled () ? 1.0f : 0.5f;

    g.setColour (group.findColour (GroupComponent::outlineColourId)
                 .withMultipliedAlpha (alpha));

    g.strokePath (p, PathStrokeType (2.0f));

    g.setColour (group.findColour (GroupComponent::textColourId)
                 .withMultipliedAlpha (alpha));
    g.setFont (f);
    g.drawText (text,
                roundToInt (x + textX), 0,
                roundToInt (textW),
                roundToInt (Constants::groupComponentFontHeight),
                Justification::centred, false);
}

void sBMP4LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    using namespace juce;

    auto fontSize = Constants::buttonFontHeight; //jmin (15.0f, (float) button.getHeight () * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    drawTickBox (g, button, 4.0f, ((float) button.getHeight () - tickWidth) * 0.5f,
                 tickWidth, tickWidth,
                 button.getToggleState (),
                 button.isEnabled (),
                 shouldDrawButtonAsHighlighted,
                 shouldDrawButtonAsDown);

    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (sharedFonts->regular.withHeight (fontSize));

    if (! button.isEnabled ())
        g.setOpacity (0.5f);

    g.drawFittedText (button.getButtonText (),
                      button.getLocalBounds ().withTrimmedLeft (roundToInt (tickWidth) + 10)
                                              .withTrimmedRight (2),
                      Justification::centredLeft, 1, 1.f);
}

void sBMP4LookAndFeel::drawDrawableButton (juce::Graphics& g, juce::DrawableButton& button,
                                           bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    using namespace juce;

    bool toggleState = button.getToggleState ();

    g.fillAll (button.findColour (toggleState ? DrawableButton::backgroundOnColourId
                                  : DrawableButton::backgroundColourId));

    g.setFont (sharedFonts->regular.withHeight (Constants::buttonSelectorFontHeight));

    g.setColour (button.findColour (toggleState ? DrawableButton::textColourOnId
                                    : DrawableButton::textColourId)
                    .withMultipliedAlpha (button.isEnabled () ? 1.0f : 0.4f));

    g.drawFittedText (button.getButtonText (),
                      2, button.getHeight () - Constants::buttonSelectorFontHeight - 1,
                      button.getWidth () - 4, Constants::buttonSelectorFontHeight,
                      Justification::centred, 1, 1.f);
}
