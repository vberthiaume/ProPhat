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
#include "SliderLabel.h"

SliderLabel::SliderLabel ()
{
    setJustificationType (juce::Justification::centredTop);
    setFont (fonts->regular.withHeight (Constants::labelFontHeight));
    setMinimumHorizontalScale (1.f);
}

void SliderLabel::componentMovedOrResized (juce::Component& attachedSlider, bool, bool)
{
    constexpr auto gap    { 7 };
    const auto theFont    { getFont () };
    const auto borderSize { getBorderSize () };

    const auto width      { juce::GlyphArrangement::getStringWidthInt (theFont, getText()) + borderSize.getLeftAndRight() };
    const auto height     { theFont.getHeight() };

    //position the label to be the width of its text and centered below the slider
    setBounds (static_cast<int> (attachedSlider.getX () + (attachedSlider.getWidth () - width) / 2),
               static_cast<int> (attachedSlider.getBottom () - height + gap),
               width,
               static_cast<int> (height));
}
