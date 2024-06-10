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

#include "ProPhatVoice.h"

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate (outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<double>&, int, int)
{
    //trying to render doubles with a float voice!
    jassertfalse;
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate(outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<float>&, int, int)
{
    //trying to render floats with a double voice!
    jassertfalse;
}

template class ProPhatVoice<float>;
template class ProPhatVoice<double>;
