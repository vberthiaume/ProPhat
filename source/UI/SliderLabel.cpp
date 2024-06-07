/*
  ==============================================================================

   Copyright (c) 2023 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

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

    const auto width      { theFont.getStringWidth (getText ()) + borderSize.getLeftAndRight () };
    const auto height     { theFont.getHeight() };

    //position the label to be the width of its text and centered below the slider
    setBounds (static_cast<int> (attachedSlider.getX () + (attachedSlider.getWidth () - width) / 2),
               static_cast<int> (attachedSlider.getBottom () - height + gap),
               width,
               static_cast<int> (height));
}
