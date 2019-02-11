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

class sBMP4Synthesiser : public Synthesiser
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

    void setOscTuning (sBMP4Voice::processorId oscNum, int newMidiNote)
    {
        for (auto voice : voices)
            //if (voice->isKeyDown())
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

private:
#if 1
    void renderVoices (AudioBuffer<float>& outputAudio, int startSample, int numSamples) override
    {
        //for (auto* voice : voices)
        //    //@TODO for some FUCKED UP reason, this renderVoices renders all the time if I don't check this. It should NOT render when there's no note off
        //    if (voice->isKeyDown())
        //        voice->renderNextBlock (outputAudio, startSample, numSamples);

        for (int i = 0; i < voices.size(); ++i)
            if (voices[i]->isKeyDown())
            {
                //DBG (i);
                voices[i]->renderNextBlock (outputAudio, startSample, numSamples);

            }

        auto block = dsp::AudioBlock<float> (outputAudio);
        auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
        auto contextToUse = dsp::ProcessContextReplacing<float> (blockToUse);
        fxChain.process (contextToUse);
    }
#endif

    enum
    {
        reverbIndex
    };

    dsp::ProcessorChain<dsp::Reverb> fxChain;
};