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

#include "SnappingSlider.h"

SnappingSlider::SnappingSlider (const SliderStyle& style, double snapValue, double snapTolerance)
    : juce::Slider (style, TextEntryBoxPosition::NoTextBox)
    , snapVal (snapValue)
    , tolerance (snapTolerance)
{
    setPopupDisplayEnabled (true, false, nullptr);
}
