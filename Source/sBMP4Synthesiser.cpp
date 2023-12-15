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

void sBMP4Synthesiser::prepare (const dsp::ProcessSpec& spec) noexcept
{
    if (Helpers::areSameSpecs (curSpecs, spec))
        return;

    curSpecs = spec;

    setCurrentPlaybackSampleRate (spec.sampleRate);

    for (auto* v : voices)
        dynamic_cast<sBMP4Voice*> (v)->prepare (spec);

    fxChain.prepare (spec);
}

void sBMP4Synthesiser::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID == sBMP4AudioProcessorIDs::osc1FreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::processorId::osc1Index, (int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::osc2FreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::processorId::osc2Index, (int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::osc1TuningID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::processorId::osc1Index, newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::osc2TuningID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::processorId::osc2Index, newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::osc1ShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::processorId::osc1Index, (int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::osc2ShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::processorId::osc2Index, (int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::oscSubID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscSub (newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::oscMixID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscMix (newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::oscNoiseID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscNoise (newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::oscSlopID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setOscSlop (newValue); }, newValue);

    else if (parameterID == sBMP4AudioProcessorIDs::ampAttackID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::ampDecayID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::ampSustainID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::ampReleaseID.getParamID ())
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setAmpParam (parameterID, newValue);

    else if (parameterID == sBMP4AudioProcessorIDs::filterEnvAttackID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::filterEnvDecayID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::filterEnvSustainID.getParamID ()
             || parameterID == sBMP4AudioProcessorIDs::filterEnvReleaseID.getParamID ())
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setFilterEnvParam (parameterID, newValue);

    else if (parameterID == sBMP4AudioProcessorIDs::lfoShapeID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoShape ((int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::lfoDestID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoDest ((int) newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::lfoFreqID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoFreq (newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::lfoAmountID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setLfoAmount (newValue); }, newValue);

    else if (parameterID == sBMP4AudioProcessorIDs::filterCutoffID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setFilterCutoff (newValue); }, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::filterResonanceID.getParamID ())
        applyToAllVoices ([] (sBMP4Voice* voice, float newValue) { voice->setFilterResonance (newValue); }, newValue);

    else if (parameterID == sBMP4AudioProcessorIDs::effectParam1ID.getParamID () || parameterID == sBMP4AudioProcessorIDs::effectParam2ID.getParamID ())
        setEffectParam (parameterID, newValue);
    else if (parameterID == sBMP4AudioProcessorIDs::masterGainID.getParamID ())
        setMasterGain (newValue);
    else
        jassertfalse;
}

void sBMP4Synthesiser::applyToAllVoices (VoiceOperation operation, float newValue)
{
    for (auto voice : voices)
        operation (dynamic_cast<sBMP4Voice*> (voice), newValue);
}

void sBMP4Synthesiser::setEffectParam (StringRef parameterID, float newValue)
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
        const ScopedLock sl (lock);

        //don't start new voices in current buffer call if we have filled all voices already.
        //voicesBeingKilled should be reset after each renderNextBlock call
        if (voicesBeingKilled.size () >= numVoices)
            return;
    }

    Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}

void sBMP4Synthesiser::renderVoices (AudioBuffer<float>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto block = dsp::AudioBlock<float> (outputAudio);
    auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
    auto contextToUse = dsp::ProcessContextReplacing<float> (blockToUse);
    fxChain.process (contextToUse);
}
