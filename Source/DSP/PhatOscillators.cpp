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

#include "PhatOscillators.h"
#include "../Utility/Helpers.h"

template class PhatOscillators<float>;
template class PhatOscillators<double>;

template <std::floating_point T>
PhatOscillators<T>::PhatOscillators (juce::AudioProcessorValueTreeState& processorState)
    : state (processorState)
    , osc1NoteOffset { static_cast<float> (Constants::middleCMidiNote - Constants::defaultOscMidiNote) }
    , osc2NoteOffset { osc1NoteOffset }
    , distribution (-1.f, 1.f)
{
    addParamListenersToState ();

    sub.setOscShape (OscShape::pulse);
    noise.setOscShape (OscShape::noise);
}

template <std::floating_point T>
void PhatOscillators<T>::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    //add our synth as listener to all parameters so we can do automations
    state.addParameterListener (osc1FreqID.getParamID (), this);
    state.addParameterListener (osc2FreqID.getParamID (), this);

    state.addParameterListener (osc1TuningID.getParamID (), this);
    state.addParameterListener (osc2TuningID.getParamID (), this);

    state.addParameterListener (osc1ShapeID.getParamID (), this);
    state.addParameterListener (osc2ShapeID.getParamID (), this);

    state.addParameterListener (oscSubID.getParamID (), this);
    state.addParameterListener (oscMixID.getParamID (), this);
    state.addParameterListener (oscNoiseID.getParamID (), this);
    state.addParameterListener (oscSlopID.getParamID (), this);
}

template <std::floating_point T>
void PhatOscillators<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    if (parameterID == osc1FreqID.getParamID ())
        setOscFreq (OscId::osc1Index, (int) newValue);
    else if (parameterID == osc2FreqID.getParamID ())
        setOscFreq (OscId::osc2Index, (int) newValue);
    else if (parameterID == osc1TuningID.getParamID ())
        setOscTuning (OscId::osc1Index, newValue);
    else if (parameterID == osc2TuningID.getParamID ())
        setOscTuning (OscId::osc2Index, newValue);
    else if (parameterID == osc1ShapeID.getParamID ())
        setOscShape (OscId::osc1Index, (int) newValue);
    else if (parameterID == osc2ShapeID.getParamID ())
        setOscShape (OscId::osc2Index, (int) newValue);
    else if (parameterID == oscSubID.getParamID ())
        setOscSub (newValue);
    else if (parameterID == oscMixID.getParamID ())
        setOscMix (newValue);
    else if (parameterID == oscNoiseID.getParamID ())
        setOscNoise (newValue);
    else if (parameterID == oscSlopID.getParamID ())
        setOscSlop (newValue);
    else
        jassertfalse;
}

template <std::floating_point T>
void PhatOscillators<T>::prepare (const juce::dsp::ProcessSpec& spec)
{
    osc1Block = juce::dsp::AudioBlock<T> (heapBlock1, spec.numChannels, spec.maximumBlockSize);
    osc2Block = juce::dsp::AudioBlock<T> (heapBlock2, spec.numChannels, spec.maximumBlockSize);
    noiseBlock = juce::dsp::AudioBlock<T> (heapBlockNoise, spec.numChannels, spec.maximumBlockSize);

    sub.prepare (spec);
    noise.prepare (spec);
    osc1.prepare (spec);
    osc2.prepare (spec);
}

template <std::floating_point T>
juce::dsp::AudioBlock<T>& PhatOscillators<T>::prepareRender (int numSamples)
{
    osc1Output = osc1Block.getSubBlock (0, (size_t) numSamples);
    osc1Output.clear ();

    osc2Output = osc2Block.getSubBlock (0, (size_t) numSamples);
    osc2Output.clear ();

    noiseOutput = noiseBlock.getSubBlock (0, (size_t) numSamples);
    noiseOutput.clear ();

    return noiseBlock;
}

template <std::floating_point T>
juce::dsp::AudioBlock<T> PhatOscillators<T>::process (int pos, int subBlockSize)
{
    //process osc1
    auto block1 { osc1Output.getSubBlock (pos, subBlockSize) };
    juce::dsp::ProcessContextReplacing<T> osc1Context (block1);
    sub.process (osc1Context); //TODO: is the sub on osc1 because that's how it is on the real prophet? Should it be added to the noise below instead?
    osc1.process (osc1Context);

    //process osc2
    auto block2 { osc2Output.getSubBlock (pos, subBlockSize) };
    juce::dsp::ProcessContextReplacing<T> osc2Context (block2);
    osc2.process (osc2Context);

    //process noise
    auto blockAll { noiseOutput.getSubBlock (pos, subBlockSize) };
    juce::dsp::ProcessContextReplacing<T> noiseContext (blockAll);
    noise.process (noiseContext);

    //process the sum of osc1 and osc2
    //blockAll.add (block1);
    //blockAll.add (block2);

    //and return that to the voice so it can render what's after the oscillators
    return blockAll;
}

template <std::floating_point T>
void PhatOscillators<T>::setLfoOsc1NoteOffset (float theLfoOsc1NoteOffset)
{
    lfoOsc1NoteOffset = theLfoOsc1NoteOffset;
    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::setLfoOsc2NoteOffset (float theLfoOsc2NoteOffset)
{
    lfoOsc2NoteOffset = theLfoOsc2NoteOffset;
    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::resetLfoOscNoteOffsets ()
{
    lfoOsc1NoteOffset = 0.f;
    lfoOsc2NoteOffset = 0.f;
}

template <std::floating_point T>
void PhatOscillators<T>::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::updateOscFrequencies (int theMidiNote, float velocity, int currentPitchWheelPosition)
{
    pitchWheelPosition = currentPitchWheelPosition;
    curVelocity = velocity;
    curMidiNote = theMidiNote;

    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::updateOscFrequenciesInternal ()
{
    if (curMidiNote < 0)
        return;

    auto pitchWheelDeltaNote = Constants::pitchWheelNoteRange.convertFrom0to1 (pitchWheelPosition / 16383.f);

    slopOsc1 = distribution (generator);
    slopOsc2 = distribution (generator);

    const auto curOsc1Slop = slopOsc1 * slopMod;
    const auto curOsc2Slop = slopOsc2 * slopMod;

    const auto osc1FloatNote = curMidiNote - osc1NoteOffset + osc1TuningOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote + curOsc1Slop;
    sub.setFrequency (Helpers::getMidiNoteInHertz(osc1FloatNote - 12), true);
    noise.setFrequency (Helpers::getMidiNoteInHertz (osc1FloatNote), true);
    osc1.setFrequency (Helpers::getMidiNoteInHertz (osc1FloatNote), true);

    const auto osc2Freq = Helpers::getMidiNoteInHertz (curMidiNote - osc2NoteOffset + osc2TuningOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote + curOsc2Slop);
    osc2.setFrequency (osc2Freq, true);
}

template <std::floating_point T>
void PhatOscillators<T>::setOscFreq (OscId oscNum, int newMidiNote)
{
    jassert (Helpers::valueContainedInRange (newMidiNote, Constants::midiNoteRange));

    switch (oscNum)
    {
        case OscId::osc1Index:
            osc1NoteOffset = Constants::middleCMidiNote - (float) newMidiNote;
            break;
        case OscId::osc2Index:
            osc2NoteOffset = Constants::middleCMidiNote - (float) newMidiNote;
            break;
        default:
            jassertfalse;
            break;
    }

    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::setOscShape (OscId oscNum, int newShape)
{
    switch (oscNum)
    {
        case OscId::osc1Index:
            osc1.setOscShape (newShape);
            break;
        case OscId::osc2Index:
            osc2.setOscShape (newShape);
            break;
        default:
            jassertfalse;
            break;
    }
}

template <std::floating_point T>
void PhatOscillators<T>::setOscTuning (OscId oscNum, float newTuning)
{
    jassert (Helpers::valueContainedInRange (newTuning, Constants::tuningSliderRange));

    switch (oscNum)
    {
        case OscId::osc1Index:
            osc1TuningOffset = newTuning;
            break;
        case OscId::osc2Index:
            osc2TuningOffset = newTuning;
            break;
        default:
            jassertfalse;
            break;
    }
    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::setOscSub (float newSub)
{
    jassert (Helpers::valueContainedInRange (newSub, Constants::sliderRange));
    curSubLevel = newSub;
    updateOscLevels ();
}

template <std::floating_point T>
void PhatOscillators<T>::setOscNoise (float noiseLevel)
{
    jassert (Helpers::valueContainedInRange (noiseLevel, Constants::sliderRange));
    curNoiseLevel = noiseLevel;
    updateOscLevels ();
}

template <std::floating_point T>
void PhatOscillators<T>::setOscSlop (float slop)
{
    jassert (Helpers::valueContainedInRange (slop, Constants::slopSliderRange));
    slopMod = slop;
    updateOscFrequenciesInternal ();
}

template <std::floating_point T>
void PhatOscillators<T>::setOscMix (float newMix)
{
    jassert (Helpers::valueContainedInRange (newMix, Constants::sliderRange));

    oscMix = newMix;
    updateOscLevels ();
}

template <std::floating_point T>
void PhatOscillators<T>::updateOscLevels ()
{
    sub.setGain (curVelocity * curSubLevel);
    noise.setGain (curVelocity * curNoiseLevel);
    osc1.setGain (curVelocity * (1 - oscMix));
    osc2.setGain (curVelocity * oscMix);
}
