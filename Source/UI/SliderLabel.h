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
#include "../Helpers.h"

/** Labels that are attached to Sliders.
*/
class SliderLabel : public juce::Label
{
public:
    SliderLabel ();

    /** Called when the attached slider is moved or resized. */
    void componentMovedOrResized (juce::Component& attachedSlider, bool /*wasMoved*/, bool /*wasResized*/) override;

    //void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::red); }

private:
    juce::SharedResourcePointer<SharedFonts> fonts;
};