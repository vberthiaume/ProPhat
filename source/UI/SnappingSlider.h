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

#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class SnappingSlider : public juce::Slider
{
public:
    SnappingSlider (const SliderStyle& style = juce::Slider::RotaryVerticalDrag,
                    double snapValue = 0.0,
                    double snapTolerance = 0.02);

    double snapValue (double attemptedValue, DragMode) override
    {
        return std::abs (snapVal - attemptedValue) < tolerance ? snapVal : attemptedValue;
    }

private:
    double snapVal;     // The value of the slider at which to snap.
    double tolerance;   // The proximity (in proportion of the slider length) to the snap point before snapping.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnappingSlider)
};