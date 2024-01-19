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

PhatOscillators::PhatOscillators (juce::AudioProcessorValueTreeState& processorState)
    : state (processorState)
    , osc1NoteOffset { static_cast<float> (Constants::middleCMidiNote - Constants::defaultOscMidiNote) }
    , osc2NoteOffset { osc1NoteOffset }
    , distribution (-1.f, 1.f)
{
    addParamListenersToState ();

    sub.setOscShape (OscShape::pulse);
    noise.setOscShape (OscShape::noise);
}

void PhatOscillators::addParamListenersToState ()
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

void PhatOscillators::parameterChanged (const juce::String& parameterID, float newValue)
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
}

void PhatOscillators::prepare (const juce::dsp::ProcessSpec& spec)
{
    //seems like auval doesn't initalize spec properly and we need to instantiate more memory than it's asking
    const auto auvalMultiplier = juce::PluginHostType ().getHostPath ().contains ("auval") ? 5 : 1;

    osc1Block = juce::dsp::AudioBlock<float> (heapBlock1, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);
    osc2Block = juce::dsp::AudioBlock<float> (heapBlock2, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);
    noiseBlock = juce::dsp::AudioBlock<float> (heapBlockNoise, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);

    sub.prepare (spec);
    noise.prepare (spec);
    osc1.prepare (spec);
    osc2.prepare (spec);
}

juce::dsp::AudioBlock<float>& PhatOscillators::prepareRender (int numSamples)
{
    osc1Output = osc1Block.getSubBlock (0, (size_t) numSamples);
    osc1Output.clear ();

    osc2Output = osc2Block.getSubBlock (0, (size_t) numSamples);
    osc2Output.clear ();

    noiseOutput = noiseBlock.getSubBlock (0, (size_t) numSamples);
    noiseOutput.clear ();

    return noiseBlock;
}

juce::dsp::AudioBlock<float> PhatOscillators::process (int pos, int curBlockSize)
{
    //process osc1
    auto block1 { osc1Output.getSubBlock (pos, curBlockSize) };
    juce::dsp::ProcessContextReplacing<float> osc1Context (block1);
    sub.process (osc1Context);
    osc1.process (osc1Context);

    //process osc2
    auto block2 { osc2Output.getSubBlock (pos, curBlockSize) };
    juce::dsp::ProcessContextReplacing<float> osc2Context (block2);
    osc2.process (osc2Context);

    //process noise
    auto blockAll { noiseOutput.getSubBlock (pos, curBlockSize) };
    juce::dsp::ProcessContextReplacing<float> noiseContext (blockAll);
    noise.process (noiseContext);

    //process the sum of osc1 and osc2
    blockAll.add (block1);
    blockAll.add (block2);

    return blockAll;
}

void PhatOscillators::setLfoOsc1NoteOffset (float theLfoOsc1NoteOffset)
{
    lfoOsc1NoteOffset = theLfoOsc1NoteOffset;
    updateOscFrequenciesInternal ();
}

void PhatOscillators::setLfoOsc2NoteOffset (float theLfoOsc2NoteOffset)
{
    lfoOsc2NoteOffset = theLfoOsc2NoteOffset;
    updateOscFrequenciesInternal ();
}

void PhatOscillators::resetLfoOscNoteOffsets ()
{
    lfoOsc1NoteOffset = 0.f;
    lfoOsc2NoteOffset = 0.f;
}

void PhatOscillators::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
    updateOscFrequenciesInternal ();
}

void PhatOscillators::updateOscFrequencies (int theMidiNote, float velocity, int currentPitchWheelPosition)
{
    pitchWheelPosition = currentPitchWheelPosition;
    curVelocity = velocity;
    curMidiNote = theMidiNote;

    updateOscFrequenciesInternal ();
}

void PhatOscillators::updateOscFrequenciesInternal ()
{
    if (curMidiNote < 0)
        return;

    auto pitchWheelDeltaNote = Constants::pitchWheelNoteRange.convertFrom0to1 (pitchWheelPosition / 16383.f);

    slopOsc1 = distribution (generator);
    slopOsc2 = distribution (generator);

    const auto curOsc1Slop = slopOsc1 * slopMod;
    const auto curOsc2Slop = slopOsc2 * slopMod;

    const auto osc1FloatNote = curMidiNote - osc1NoteOffset + osc1TuningOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote + curOsc1Slop;
    sub.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote - 12), true);
    noise.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote), true);
    osc1.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote), true);

    const auto osc2Freq = Helpers::getFloatMidiNoteInHertz (curMidiNote - osc2NoteOffset + osc2TuningOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote + curOsc2Slop);
    osc2.setFrequency ((float) osc2Freq, true);
}

void PhatOscillators::setOscFreq (OscId oscNum, int newMidiNote)
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

void PhatOscillators::setOscShape (OscId oscNum, int newShape)
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

void PhatOscillators::setOscTuning (OscId oscNum, float newTuning)
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

void PhatOscillators::setOscSub (float newSub)
{
    jassert (Helpers::valueContainedInRange (newSub, Constants::sliderRange));
    curSubLevel = newSub;
    updateOscLevels ();
}

void PhatOscillators::setOscNoise (float noiseLevel)
{
    jassert (Helpers::valueContainedInRange (noiseLevel, Constants::sliderRange));
    curNoiseLevel = noiseLevel;
    updateOscLevels ();
}

void PhatOscillators::setOscSlop (float slop)
{
    jassert (Helpers::valueContainedInRange (slop, Constants::slopSliderRange));
    slopMod = slop;
    updateOscFrequenciesInternal ();
}

void PhatOscillators::setOscMix (float newMix)
{
    jassert (Helpers::valueContainedInRange (newMix, Constants::sliderRange));

    oscMix = newMix;
    updateOscLevels ();
}

void PhatOscillators::updateOscLevels ()
{
    sub.setGain (curVelocity * curSubLevel);
    noise.setGain (curVelocity * curNoiseLevel);
    osc1.setGain (curVelocity * (1 - oscMix));
    osc2.setGain (curVelocity * oscMix);
}
