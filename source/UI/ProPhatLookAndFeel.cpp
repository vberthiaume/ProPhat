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

#include "ProPhatLookAndFeel.h"
#include "../Utility/Macros.h"

ProPhatLookAndFeel::ProPhatLookAndFeel()
{
    /*tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);*/

    tickedButtonImage = Helpers::getImage (BinaryData::redLight_png, BinaryData::redLight_pngSize);
    untickedButtonImage = Helpers::getImage (BinaryData::blackLight_png, BinaryData::blackLight_pngSize);

#if USE_SVG
    rotarySliderDrawableImage = Helpers::getDrawable (BinaryData::phatKnob_svg, BinaryData::phatKnob_svgSize);
#else
    //LOADING SVG USING DRAWABLES
    rotarySliderImage = Helpers::getImage (BinaryData::metalKnobFitted_png, BinaryData::metalKnobFitted_pngSize);
#endif
}

void ProPhatLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto cornerSize = 6.0f;
    auto bounds = button.getLocalBounds ().toFloat ().reduced (0.5f, 0.5f);

    const auto backgroundColour { juce::Colours::transparentBlack };
    auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 2.f : 0.9f)
                                      .withMultipliedAlpha (button.hasKeyboardFocus (true) ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting (shouldDrawButtonAsHighlighted ? 0.2f : 0.05f);

    g.setColour (baseColour);

    auto flatOnLeft = button.isConnectedOnLeft ();
    auto flatOnRight = button.isConnectedOnRight ();
    auto flatOnTop = button.isConnectedOnTop ();
    auto flatOnBottom = button.isConnectedOnBottom ();

    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
    {
        juce::Path path;
        path.addRoundedRectangle (bounds.getX (), bounds.getY (),
                                  bounds.getWidth (), bounds.getHeight (),
                                  cornerSize, cornerSize,
                                  ! (flatOnLeft || flatOnTop),
                                  ! (flatOnRight || flatOnTop),
                                  ! (flatOnLeft || flatOnBottom),
                                  ! (flatOnRight || flatOnBottom));

        g.fillPath (path);

        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.strokePath (path, juce::PathStrokeType (1.0f));
    }
    else
    {
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }
}

void ProPhatLookAndFeel::drawTickBox (juce::Graphics& g, juce::Component& /*component*/,
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

void ProPhatLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
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
                                                                                           .rotated (toAngle, bounds.getCentreX (), bounds.getCentreY ()));
#else
        auto imageBounds = rotarySliderImage.getBounds().toFloat();
        auto scaleFactor = squareSide / imageBounds.getWidth();

        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (scaleFactor).translated (xTranslation + gap, yTranslation + gap)
                                                                   .rotated (toAngle, bounds.getCentreX(), bounds.getCentreY()));
#endif
    }
}

void ProPhatLookAndFeel::drawGroupComponentOutline (juce::Graphics& g, int width, int height, const juce::String& text,
                                                  const juce::Justification& position, juce::GroupComponent& group)
{
    const float indent = 3.0f;
    const float textEdgeGap = 4.0f;
    auto cs = 5.0f;

    using namespace juce;

    Font f (fonts->regular.withHeight (Constants::groupComponentFontHeight));

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

#if ! USE_BACKGROUND_IMAGE
    g.setColour (juce::Colour { Constants::phatGrey });
    g.fillPath (p);
#endif

    g.setColour (group.findColour (GroupComponent::outlineColourId).withMultipliedAlpha (alpha));
    g.strokePath (p, PathStrokeType (2.0f));

    g.setColour (group.findColour (GroupComponent::textColourId).withMultipliedAlpha (alpha));
    g.setFont (f);
    g.drawText (text,
                roundToInt (x + textX), 0,
                roundToInt (textW),
                roundToInt (Constants::groupComponentFontHeight),
                Justification::centred, false);
}

void ProPhatLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    using namespace juce;

    auto fontSize = Constants::buttonFontHeight;
    auto tickWidth = fontSize * 1.1f;

    drawTickBox (g, button, 4.0f, ((float) button.getHeight () - tickWidth) * 0.5f,
                 tickWidth, tickWidth,
                 button.getToggleState (),
                 button.isEnabled (),
                 shouldDrawButtonAsHighlighted,
                 shouldDrawButtonAsDown);

    const auto font { fonts->regular.withHeight (fontSize) };
    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (font);

    if (! button.isEnabled ())
        g.setOpacity (0.5f);

    const auto text { button.getButtonText () };
    const auto textWidth { font.getStringWidth (text) };
    g.drawFittedText (text,
                      button.getLocalBounds ().withTrimmedLeft (roundToInt (tickWidth) + 10) .withWidth (textWidth),
                      Justification::centredLeft,
                      1,
                      1.f);
}

void ProPhatLookAndFeel::drawDrawableButton (juce::Graphics& g, juce::DrawableButton& button,
                                           bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    using namespace juce;

    const bool toggleState = button.getToggleState ();
    g.fillAll (button.findColour (toggleState ? DrawableButton::backgroundOnColourId : DrawableButton::backgroundColourId));

    const auto font { fonts->regular.withHeight (Constants::buttonSelectorFontHeight) };
    g.setFont (font);

    g.setColour (button.findColour (toggleState ? DrawableButton::textColourOnId : DrawableButton::textColourId)
                       .withMultipliedAlpha (button.isEnabled () ? 1.0f : 0.4f));

    const auto text { button.getButtonText () };
    const auto textWidth { font.getStringWidth (text) };
    g.drawFittedText (text,
                      (button.getWidth () - textWidth) / 2,
                      button.getHeight () - Constants::buttonSelectorFontHeight - 1,
                      textWidth,
                      Constants::buttonSelectorFontHeight,
                      Justification::centred,
                      1,
                      1.f);
}
