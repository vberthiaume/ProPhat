/*
  ==============================================================================

    sBMP4Synthesiser.h
    Created: 29 Jan 2019 10:59:23am
    Author:  Haake

  ==============================================================================
*/

#pragma once

#include "SineWaveVoice.h"
#include "Helpers.h"

class sBMP4Synthesiser : public Synthesiser, public AudioProcessorValueTreeState::Listener
{
public:
    sBMP4Synthesiser()
    {
        for (auto i = 0; i < numVoices; ++i)
            addVoice (new sBMP4Voice (i, &voicesBeingKilled));

        addSound (new sBMP4Sound());
    }

    void prepare (const dsp::ProcessSpec& spec) noexcept
    {
        if (Helpers::areSameSpecs (curSpecs, spec))
            return;

        curSpecs = spec;

        setCurrentPlaybackSampleRate (spec.sampleRate);

        for (auto* v : voices)
            dynamic_cast<sBMP4Voice*> (v)->prepare (spec);

        fxChain.prepare (spec);
    }

    void parameterChanged (const String& parameterID, float newValue) override
    {
        if (parameterID == sBMP4AudioProcessorIDs::osc1FreqID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::osc1Index, (int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc2FreqID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscFreq (sBMP4Voice::osc2Index, (int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc1TuningID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::osc1Index, newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc2TuningID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscTuning (sBMP4Voice::osc2Index, newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc1ShapeID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::osc1Index, (int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc2ShapeID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscShape (sBMP4Voice::osc2Index, (int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::oscSubID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscSub (newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::oscMixID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setOscMix (newValue); }, newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::ampAttackID
                || parameterID == sBMP4AudioProcessorIDs::ampDecayID
                || parameterID == sBMP4AudioProcessorIDs::ampSustainID
                || parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
            for (auto voice : voices)
                dynamic_cast<sBMP4Voice*> (voice)->setAmpParam (parameterID, newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::lfoShapeID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setLfoShape ((int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::lfoDestID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setLfoDest ((int) newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::lfoFreqID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setLfoFreq (newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::lfoAmountID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setLfoAmount (newValue); }, newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::filterCutoffID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setFilterCutoff (newValue); }, newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::filterResonanceID)
            applyToAllVoices ([](sBMP4Voice* voice, float newValue) { voice->setFilterResonance (newValue); }, newValue);
        else
            jassertfalse;
    }

    using VoiceOperation = std::function<void (sBMP4Voice*, float)>;
    void applyToAllVoices (VoiceOperation operation, float newValue)
    {
        for (auto voice : voices)
            operation (dynamic_cast<sBMP4Voice*> (voice), newValue);
    }

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override
    {
        {
            const ScopedLock sl (lock);

            //don't start new voices in current buffer call if we have filled all voices already.
            //voicesBeingKilled should be reset after each renderNextBlock call
            if (voicesBeingKilled.size() >= numVoices)
                return;
        }

        Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

private:

    enum
    {
        reverbIndex
    };

    //@TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled{};

    dsp::ProcessorChain<dsp::Reverb> fxChain;

    dsp::ProcessSpec curSpecs{};
};
