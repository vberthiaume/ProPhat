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

ProPhatSynthesiser::ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState)
    : state (processorState)
{
    for (auto i = 0; i < Constants::numVoices; ++i)
        addVoice (new ProPhatVoice (state, i, &voicesBeingKilled));

    addSound (new ProPhatSound ());

    addParamListenersToState ();

    setMasterGain (Constants::defaultMasterGain);
    fxChain.get<masterGainIndex> ().setRampDurationSeconds (0.1);

    //we need to manually override the default reverb params to make sure 0 values are set if needed
    fxChain.get<reverbIndex> ().setParameters (reverbParams);
}

void ProPhatSynthesiser::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    state.addParameterListener (effectParam1ID.getParamID (), this);
    state.addParameterListener (effectParam2ID.getParamID (), this);

    state.addParameterListener (masterGainID.getParamID (), this);
}

void ProPhatSynthesiser::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    if (Helpers::areSameSpecs (curSpecs, spec))
        return;

    curSpecs = spec;

    setCurrentPlaybackSampleRate (spec.sampleRate);

    for (auto* v : voices)
        dynamic_cast<ProPhatVoice*> (v)->prepare (spec);

    fxChain.prepare (spec);
}

void ProPhatSynthesiser::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatSynthesiser::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == effectParam1ID.getParamID () || parameterID == effectParam2ID.getParamID ())
        setEffectParam (parameterID, newValue);
    else if (parameterID == masterGainID.getParamID ())
        setMasterGain (newValue);
    else
        jassertfalse;
}

void ProPhatSynthesiser::setEffectParam (juce::StringRef parameterID, float newValue)
{
    if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;
    else
        jassertfalse;   //unknown effect parameter!

    fxChain.get<reverbIndex> ().setParameters (reverbParams);
}

void ProPhatSynthesiser::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    {
        //TODO lock in the audio thread??
        const juce::ScopedLock sl (lock);

        //don't start new voices in current buffer call if we have filled all voices already.
        //voicesBeingKilled should be reset after each renderNextBlock call
        if (voicesBeingKilled.size () >= Constants::numVoices)
            return;
    }

    Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}

void ProPhatSynthesiser::renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    renderVoices<float>(outputAudio, startSample, numSamples);
}

void ProPhatSynthesiser::renderVoices (juce::AudioBuffer<double>& outputAudio, int startSample, int numSamples)
{
    renderVoices<double> (outputAudio, startSample, numSamples);
}
