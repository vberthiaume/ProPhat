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

#include "sBMP4Label.h"

sBMP4Label::sBMP4Label ()
{
    setJustificationType (juce::Justification::centredBottom);
    setFont (sharedFonts->regular.withHeight (Constants::labelFontHeight));
    setMinimumHorizontalScale (1.f);
}

void sBMP4Label::componentMovedOrResized (juce::Component& component, bool, bool)
{
    auto& lf = getLookAndFeel ();
    auto f = lf.getLabelFont (*this);
    auto borderSize = lf.getLabelBorderSize (*this);

    //TODO: we should probably set the width here to be the length of the text?
    auto height = borderSize.getTopAndBottom () + 6 + juce::roundToInt (f.getHeight () + 0.5f);
    setBounds (component.getX (), component.getY () + component.getHeight () - height + 7, component.getWidth (), height);
}
