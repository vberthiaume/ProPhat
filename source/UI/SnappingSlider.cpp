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

#include "SnappingSlider.h"

SnappingSlider::SnappingSlider (const SliderStyle& style, double snapValue, double snapTolerance)
    : juce::Slider (style, TextEntryBoxPosition::NoTextBox)
    , snapVal (snapValue)
    , tolerance (snapTolerance)
{
#if JUCE_DEBUG
    setPopupDisplayEnabled (true, false, nullptr);
#endif
}
