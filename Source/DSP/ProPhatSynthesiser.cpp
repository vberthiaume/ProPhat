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

ProPhatSynthesiser::ProPhatSynthesiser ()
{
    for (auto i = 0; i < numVoices; ++i)
        addVoice (new ProPhatVoice (i, &voicesBeingKilled));

    addSound (new ProPhatSound ());

    setMasterGain (defaultMasterGain);
    fxChain.get<masterGainIndex> ().setRampDurationSeconds (0.1);

    //we need to manually override the default reverb params to make sure 0 values are set if needed
    fxChain.get<reverbIndex> ().setParameters (reverbParams);
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
    using namespace ProPhatAudioProcessorIDs;

    //DBG ("ProPhatSynthesiser::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    //TODO: make the voices or the osc listeners instead?
    //if (parameterID == osc1FreqID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscFreq (ProPhatVoice::OscId::osc1Index, (int) newValue); }, newValue);
    //else if (parameterID == osc2FreqID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscFreq (ProPhatVoice::OscId::osc2Index, (int) newValue); }, newValue);
    //else if (parameterID == osc1TuningID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscTuning (ProPhatVoice::OscId::osc1Index, newValue); }, newValue);
    //else if (parameterID == osc2TuningID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscTuning (ProPhatVoice::OscId::osc2Index, newValue); }, newValue);
    //else if (parameterID == osc1ShapeID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscShape (ProPhatVoice::OscId::osc1Index, (int) newValue); }, newValue);
    //else if (parameterID == osc2ShapeID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscShape (ProPhatVoice::OscId::osc2Index, (int) newValue); }, newValue);
    //else if (parameterID == oscSubID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscSub (newValue); }, newValue);
    //else if (parameterID == oscMixID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscMix (newValue); }, newValue);
    //else if (parameterID == oscNoiseID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscNoise (newValue); }, newValue);
    //else if (parameterID == oscSlopID.getParamID ())
    //    applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setOscSlop (newValue); }, newValue);

    //else 
        if (parameterID == ampAttackID.getParamID ()
             || parameterID == ampDecayID.getParamID ()
             || parameterID == ampSustainID.getParamID ()
             || parameterID == ampReleaseID.getParamID ())
        applyToAllVoices ([parameterID] (ProPhatVoice* voice, float newValue) { voice->setAmpParam (parameterID, newValue); }, newValue);

    else if (parameterID == filterEnvAttackID.getParamID ()
             || parameterID == filterEnvDecayID.getParamID ()
             || parameterID == filterEnvSustainID.getParamID ()
             || parameterID == filterEnvReleaseID.getParamID ())
        applyToAllVoices ([parameterID] (ProPhatVoice* voice, float newValue) { voice->setFilterEnvParam (parameterID, newValue); }, newValue);

    else if (parameterID == lfoShapeID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setLfoShape ((int) newValue); }, newValue);
    else if (parameterID == lfoDestID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setLfoDest ((int) newValue); }, newValue);
    else if (parameterID == lfoFreqID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setLfoFreq (newValue); }, newValue);
    else if (parameterID == lfoAmountID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setLfoAmount (newValue); }, newValue);

    else if (parameterID == filterCutoffID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setFilterCutoff (newValue); }, newValue);
    else if (parameterID == filterResonanceID.getParamID ())
        applyToAllVoices ([] (ProPhatVoice* voice, float newValue) { voice->setFilterResonance (newValue); }, newValue);

    else if (parameterID == effectParam1ID.getParamID () || parameterID == effectParam2ID.getParamID ())
        setEffectParam (parameterID, newValue);
    else if (parameterID == masterGainID.getParamID ())
        setMasterGain (newValue);
    //else
    //    jassertfalse;
}

void ProPhatSynthesiser::setEffectParam (juce::StringRef parameterID, float newValue)
{
    if (parameterID == ProPhatAudioProcessorIDs::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;
    else
        jassertfalse;   //unknown effect parameter!

    fxChain.get<reverbIndex> ().setParameters (reverbParams);
}

void ProPhatSynthesiser::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    {
        //this is lock in the audio thread??
        const juce::ScopedLock sl (lock);

        //don't start new voices in current buffer call if we have filled all voices already.
        //voicesBeingKilled should be reset after each renderNextBlock call
        if (voicesBeingKilled.size () >= numVoices)
            return;
    }

    Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}

void ProPhatSynthesiser::renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto block = juce::dsp::AudioBlock<float> (outputAudio);
    auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
    auto contextToUse = juce::dsp::ProcessContextReplacing<float> (blockToUse);
    fxChain.process (contextToUse);
}
