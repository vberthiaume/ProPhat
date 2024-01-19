/*
  ==============================================================================

   Copyright (c) 2024 - Vincent Berthiaume

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
#include "GainedOscillator.h"

/**
 * @brief A container for all our oscillators.
*/
class PhatOscillators
{
public:
    PhatOscillators ();

    void prepare (const juce::dsp::ProcessSpec& spec);

    //void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    juce::dsp::AudioBlock<float>& prepareRender (int numSamples);
    juce::dsp::AudioBlock<float> process (int pos, int curBlockSize);

    void setLfoOsc1NoteOffset (float theLfoOsc1NoteOffset);
    void setLfoOsc2NoteOffset (float theLfoOsc2NoteOffset);

    enum class OscId
    {
        osc1Index = 0,
        osc2Index,
    };
    void updateOscFrequencies (int midiNote, float velocity, int currentPitchWheelPosition);

    void setOscFreq (OscId oscNum, int newMidiNote);
    void setOscShape (OscId oscNum, int newShape);
    void setOscTuning (OscId oscNum, float newTuning);
    void setOscSub (float newSub);
    void setOscNoise (float noiseLevel);
    void setOscSlop (float slop);
    void setOscMix (float newMix);

    void updateOscLevels ();

    void resetLfoOscNoteOffsets ();

    void pitchWheelMoved (int newPitchWheelValue);

private:
    void updateOscFrequenciesInternal ();

    juce::HeapBlock<char> heapBlock1, heapBlock2, heapBlockNoise;

    //TODO: make both these templates
    juce::dsp::AudioBlock<float> osc1Block, osc2Block, noiseBlock, osc1Output, osc2Output, noiseOutput;
    GainedOscillator<float> sub, osc1, osc2, noise;

    float osc1NoteOffset, osc2NoteOffset;

    std::uniform_real_distribution<float> distribution;
    std::default_random_engine generator;

    float osc1TuningOffset = 0.f;
    float osc2TuningOffset = 0.f;

    float curVelocity = 0.f;
    float curSubLevel = 0.f;
    float oscMix = 0.f;
    float curNoiseLevel = 0.f;

    float slopOsc1 = 0.f, slopOsc2 = 0.f, slopMod = 0.f;

    int pitchWheelPosition = 0;

    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    int curMidiNote;
};
