/*
  ==============================================================================

   Copyright (c) 2023 - Vincent Berthiaume

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

#include "SliderLabel.h"

SliderLabel::SliderLabel ()
{
    setJustificationType (juce::Justification::centredBottom);
    setFont (sharedFonts->regular.withHeight (Constants::labelFontHeight));
    setMinimumHorizontalScale (1.f);
}

void SliderLabel::componentMovedOrResized (juce::Component& attachedSlider, bool, bool)
{
    const auto theFont    { getFont () };
    const auto textWidth  { theFont.getStringWidthFloat (getText ()) };
    const auto borderSize { getBorderSize () };

    //TODO: we should probably set the width here to be the length of the text?
    auto height = borderSize.getTopAndBottom () + 6 + juce::roundToInt (theFont.getHeight () + 0.5f);
    setBounds (attachedSlider.getX (), attachedSlider.getY () + attachedSlider.getHeight () - height + 7, attachedSlider.getWidth (), height);
}
