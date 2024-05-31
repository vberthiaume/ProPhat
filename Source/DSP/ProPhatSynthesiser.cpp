/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

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

#include "ProPhatSynthesiser.h"

template class ProPhatSynthesiser<float>;
template class ProPhatSynthesiser<double>;

template <>
void ProPhatSynthesiser<float>::renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    renderVoicesTemplate (outputAudio, startSample, numSamples);
}

template <>
void ProPhatSynthesiser<double>::renderVoices (juce::AudioBuffer<float>&, int, int)
{
    //trying to render a float voice with a double synth doesn't make sense!
    jassertfalse;
}

template <>
void ProPhatSynthesiser<double>::renderVoices (juce::AudioBuffer<double>& outputAudio, int startSample, int numSamples)
{
    renderVoicesTemplate (outputAudio, startSample, numSamples);
}

template <>
void ProPhatSynthesiser<float>::renderVoices (juce::AudioBuffer<double>&, int, int)
{
    //trying to render a double voice with a float synth doesn't make sense!
    jassertfalse;
}
