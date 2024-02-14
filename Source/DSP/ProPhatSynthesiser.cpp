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

template <std::floating_point T>
ProPhatSynthesiser<T>::ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState)
    : state (processorState)
{
    for (auto i = 0; i < Constants::numVoices; ++i)
        addVoice (new ProPhatVoice<T> (state, i, &voicesBeingKilled));

    addSound (new ProPhatSound ());

    addParamListenersToState ();

    setMasterGain (Constants::defaultMasterGain);
    fxChain.get<masterGainIndex> ().setRampDurationSeconds (0.1);

#if USE_REVERB
    //we need to manually override the default reverb params to make sure 0 values are set if needed
    fxChain.get<reverbIndex> ().setParameters (reverbParams);
#endif
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    state.addParameterListener (effectParam1ID.getParamID (), this);
    state.addParameterListener (effectParam2ID.getParamID (), this);

    state.addParameterListener (masterGainID.getParamID (), this);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    if (Helpers::areSameSpecs (curSpecs, spec))
        return;

    curSpecs = spec;

    setCurrentPlaybackSampleRate (spec.sampleRate);

    for (auto* v : voices)
        dynamic_cast<ProPhatVoice<T>*> (v)->prepare (spec);

    fxChain.prepare (spec);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::parameterChanged (const juce::String& parameterID, float newValue)
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

template <std::floating_point T>
void ProPhatSynthesiser<T>::setEffectParam ([[maybe_unused]] juce::StringRef parameterID, [[maybe_unused]] float newValue)
{
#if USE_REVERB
    if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;
    else
        jassertfalse;   //unknown effect parameter!


    fxChain.get<reverbIndex> ().setParameters (reverbParams);
#endif
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
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

template <>
void ProPhatSynthesiser<float>::renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    renderVoices<float> (outputAudio, startSample, numSamples);
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
    renderVoices<double> (outputAudio, startSample, numSamples);
}

template <>
void ProPhatSynthesiser<float>::renderVoices (juce::AudioBuffer<double>&, int, int)
{
    //trying to render a double voice with a float synth doesn't make sense!
    jassertfalse;
}
