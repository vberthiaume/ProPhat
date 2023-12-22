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

#pragma once
#include "../Helpers.h"

/** The main labels used in the plugin.
*/
class sBMP4Label : public juce::Label
{
public:
    sBMP4Label ();

    void componentMovedOrResized (juce::Component& component, bool /*wasMoved*/, bool /*wasResized*/) override;

private:
    juce::SharedResourcePointer<SharedFonts> sharedFonts;
};