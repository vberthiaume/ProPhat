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
            addVoice (new sBMP4Voice());

        setNoteStealingEnabled (true);

        addSound (new SineWaveSound());
    }

    void prepare (const dsp::ProcessSpec& spec) noexcept
    {
        setCurrentPlaybackSampleRate (spec.sampleRate);

        for (auto* v : voices)
            dynamic_cast<sBMP4Voice*> (v)->prepare (spec);

        fxChain.prepare (spec);
    }

    void parameterChanged (const String& parameterID, float newValue) override
    {
        if (parameterID == sBMP4AudioProcessorIDs::osc1FreqID)
            setOscTuning (sBMP4Voice::processorId::osc1Index, (int) newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::osc2FreqID)
            setOscTuning (sBMP4Voice::processorId::osc2Index, (int) newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::osc1ShapeID)
            setOscShape (sBMP4Voice::processorId::osc2Index, OscShape ((int) newValue + 1));
        else if (parameterID == sBMP4AudioProcessorIDs::osc2ShapeID)
            setOscShape (sBMP4Voice::processorId::osc2Index, OscShape ((int) newValue + 1));

        else if (parameterID == sBMP4AudioProcessorIDs::ampAttackID
                || parameterID == sBMP4AudioProcessorIDs::ampDecayID
                || parameterID == sBMP4AudioProcessorIDs::ampSustainID
                || parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
            setAmpParam (parameterID, newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::lfoShapeID)
            setLfoShape (LfoShape ((int) newValue + 1));
        else if (parameterID == sBMP4AudioProcessorIDs::lfoFreqID)
            setLfoFreq (newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::lfoAmountID)
            setLfoAmount (newValue);

        else if (parameterID == sBMP4AudioProcessorIDs::filterCutoffID)
            setFilterCutoff (newValue);
        else if (parameterID == sBMP4AudioProcessorIDs::filterResonanceID)
            setFilterResonance (newValue);
    }

    void setAmpParam (StringRef parameterID, float newValue)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setAmpParam (parameterID, newValue);
    }

    void setOscTuning (sBMP4Voice::processorId oscNum, int newMidiNote)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setOscTuning (oscNum, newMidiNote);
    }

    void setOscShape (sBMP4Voice::processorId oscNum, OscShape newShape)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setOscShape (oscNum, newShape);
    }

    void setLfoShape (LfoShape newShape)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setLfoShape (newShape);
    }

    void setLfoFreq (float newFreq)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setLfoFreq (newFreq);
    }

    void setLfoAmount (float newAmount)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setLfoAmount (newAmount);
    }

    void setFilterCutoff (float newAmount)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setFilterCutoff (newAmount);
    }

    void setFilterResonance (float newAmount)
    {
        for (auto voice : voices)
            dynamic_cast<sBMP4Voice*> (voice)->setFilterResonance (newAmount);
    }

private:

    enum
    {
        reverbIndex
    };

    void renderVoices (AudioBuffer<float>& outputAudio, int startSample, int numSamples) override
    {
        for (auto* voice : voices)
            if (auto sbmp4Voice = dynamic_cast<sBMP4Voice*> (voice))
                sbmp4Voice->renderNextBlock (outputAudio, startSample, numSamples);

        auto block = dsp::AudioBlock<float> (outputAudio);
        auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
        auto contextToUse = dsp::ProcessContextReplacing<float> (blockToUse);
        fxChain.process (contextToUse);
    }

    dsp::ProcessorChain<dsp::Reverb> fxChain;
};
