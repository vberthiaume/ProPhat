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
template <std::floating_point T>
class PhatOscillators : public juce::AudioProcessorValueTreeState::Listener
{
public:
    PhatOscillators (juce::AudioProcessorValueTreeState& processorState);

    void addParamListenersToState ();
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);

    juce::dsp::AudioBlock<T>& prepareRender (int numSamples);
    juce::dsp::AudioBlock<T> process (int pos, int curBlockSize);

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

    juce::AudioProcessorValueTreeState& state;

    juce::HeapBlock<char> heapBlock1, heapBlock2, heapBlockNoise;

    //TODO: make both these templates
    juce::dsp::AudioBlock<T> osc1Block, osc2Block, noiseBlock, osc1Output, osc2Output, noiseOutput;
    GainedOscillator<T> sub, osc1, osc2, noise;

    float osc1NoteOffset, osc2NoteOffset;

    std::uniform_real_distribution<T> distribution;
    std::default_random_engine generator;

    float osc1TuningOffset = 0.f;
    float osc2TuningOffset = 0.f;

    float curVelocity = 0.f;
    float curSubLevel = 0.f;
    float oscMix = 0.f;
    float curNoiseLevel = 0.f;

    T slopOsc1{ 0 }, slopOsc2{ 0 }, slopMod{ 0 };

    int pitchWheelPosition = 0;

    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    int curMidiNote;
};
