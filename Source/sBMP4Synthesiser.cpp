/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   sBMP4 IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "sBMP4Synthesiser.h"

sBMP4Synthesiser::sBMP4Synthesiser ()
{
    for (auto i = 0; i < numVoices; ++i)
        addVoice (new sBMP4Voice (i, &voicesBeingKilled));

    addSound (new sBMP4Sound ());

    setMasterGain (defaultMasterGain);
    fxChain.get<masterGainIndex> ().setRampDurationSeconds (0.1);
}

void sBMP4Synthesiser::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    if (Helpers::areSameSpecs (curSpecs, spec))
        return;

    curSpecs = spec;

    setCurrentPlaybackSampleRate (spec.sampleRate);

    for (auto* v : voices)
        dynamic_cast<sBMP4Voice*> (v)->prepare (spec);

    fxChain.prepare (spec);
}

void sBMP4Synthesiser::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace sBMP4AudioProcessorIDs;

    if (parameterID == osc1FreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::processorId::osc1Index, (int) newValue); }, newValue);
    else if (parameterID == osc2FreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::processorId::osc2Index, (int) newValue); }, newValue);
    else if (parameterID == osc1TuningID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::processorId::osc1Index, newValue); }, newValue);
    else if (parameterID == osc2TuningID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::processorId::osc2Index, newValue); }, newValue);
    else if (parameterID == osc1ShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::processorId::osc1Index, (int) newValue); }, newValue);
    else if (parameterID == osc2ShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::processorId::osc2Index, (int) newValue); }, newValue);
    else if (parameterID == oscSubID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscSub (newValue); }, newValue);
    else if (parameterID == oscMixID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscMix (newValue); }, newValue);
    else if (parameterID == oscNoiseID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscNoise (newValue); }, newValue);
    else if (parameterID == oscSlopID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscSlop (newValue); }, newValue);

    else if (parameterID == ampAttackID.getParamID ()
             || parameterID == ampDecayID.getParamID ()
             || parameterID == ampSustainID.getParamID ()
             || parameterID == ampReleaseID.getParamID ())
        applyToAllVoices ([parameterID] (sBMP4Voice* voice, float newValue) { voice->setAmpParam (parameterID, newValue); }, newValue);

    else if (parameterID == filterEnvAttackID.getParamID ()
             || parameterID == filterEnvDecayID.getParamID ()
             || parameterID == filterEnvSustainID.getParamID ()
             || parameterID == filterEnvReleaseID.getParamID ())
        applyToAllVoices ([parameterID] (sBMP4Voice* voice, float newValue) { voice->setFilterEnvParam (parameterID, newValue); }, newValue);

    else if (parameterID == lfoShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoShape ((int) newValue); }, newValue);
    else if (parameterID == lfoDestID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoDest ((int) newValue); }, newValue);
    else if (parameterID == lfoFreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoFreq (newValue); }, newValue);
    else if (parameterID == lfoAmountID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoAmount (newValue); }, newValue);

    else if (parameterID == filterCutoffID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setFilterCutoff (newValue); }, newValue);
    else if (parameterID == filterResonanceID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setFilterResonance (newValue); }, newValue);

    else if (parameterID == effectParam1ID.getParamID () || parameterID == effectParam2ID.getParamID ())
        setEffectParam (parameterID, newValue);
    else if (parameterID == masterGainID.getParamID ())
        setMasterGain (newValue);
    else
        jassertfalse;
}

void sBMP4Synthesiser::setEffectParam (juce::StringRef parameterID, float newValue)
{
    if (parameterID == sBMP4AudioProcessorIDs::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;

    fxChain.get<reverbIndex> ().setParameters (reverbParams);
}

void sBMP4Synthesiser::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
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

void sBMP4Synthesiser::renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto block = juce::dsp::AudioBlock<float> (outputAudio);
    auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
    auto contextToUse = juce::dsp::ProcessContextReplacing<float> (blockToUse);
    fxChain.process (contextToUse);
}
