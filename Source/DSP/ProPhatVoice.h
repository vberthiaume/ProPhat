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

#pragma once
#include "../Utility/Helpers.h"
#include "PhatOscillators.h"

#include <mutex>
#include <set>

struct ProPhatSound : public juce::SynthesiserSound
{
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================

/**
 * @brief The main voice for our synthesizer.
*/
template <std::floating_point T>
class ProPhatVoice : public juce::SynthesiserVoice
                   , public juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class ProcessorId
    {
        filterIndex = 0,
        masterGainIndex,
    };

    ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int voiceId, std::set<int>* activeVoiceSet);

    void addParamListenersToState ();
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);

    void setAmpParam (juce::StringRef parameterID, float newValue);
    void setFilterEnvParam (juce::StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }
    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (float newValue);
    void setFilterTiltCutoff (float newValue);
    void setFilterResonance (float newAmount);

    void pitchWheelMoved (int newPitchWheelValue) override { oscillators.pitchWheelMoved (newPitchWheelValue); }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<ProPhatSound*> (sound) != nullptr; }

    template <std::floating_point T>
    void renderNextBlockTemplate(juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
    {
        if (! currentlyKillingVoice && !isVoiceActive())
            return;

        //reserve an audio block of size numSamples. Auvaltool has a tendency to _not_ call prepare before rendering
        //with new buffer sizes, so just making sure we're not taking more samples than the audio block was prepared with.
        numSamples = juce::jmin(numSamples, curPreparedSamples);
        auto currentAudioBlock{ oscillators.prepareRender(numSamples) };

        for (int pos = 0; pos < numSamples;)
        {
            const auto subBlockSize = juce::jmin(numSamples - pos, lfoUpdateCounter);

            //render the oscillators
            //TODO CANNOT FIGURE THIS OUT
            juce::dsp::AudioBlock<T> oscBlock;// = oscillators.process(pos, subBlockSize);

            //render our effects
            juce::dsp::ProcessContextReplacing<T> oscContext(oscBlock);
            processorChain.process(oscContext);

            //during this call, the voice may become inactive, but we still have to finish this loop to ensure the voice stays muted for the rest of the buffer
            processEnvelope(oscBlock);

            if (rampingUp)
                processRampUp(oscBlock, (int)subBlockSize);

            if (overlapIndex > -1)
                processKillOverlap(oscBlock, (int)subBlockSize);

            pos += subBlockSize;
            lfoUpdateCounter -= subBlockSize;

            if (lfoUpdateCounter == 0)
            {
                lfoUpdateCounter = lfoUpdateRate;
                updateLfo();
            }
        }

        //add everything to the output buffer
        //TODO PUT BACK
        //juce::dsp::AudioBlock<T>(outputBuffer).getSubBlock((size_t)startSample, (size_t)numSamples).add(currentAudioBlock);

        if (currentlyKillingVoice)
            applyKillRamp(outputBuffer, startSample, numSamples);
#if DEBUG_VOICES
        else
            assertForDiscontinuities(outputBuffer, startSample, numSamples, {});
#endif
    }
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        //PUT BACK
        //renderNextBlockTemplate<float>(outputBuffer, startSample, numSamples);
    }

    void renderNextBlock(juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples) override
    {
        //PUT BACK
        //renderNextBlockTemplate<double>(outputBuffer, startSample, numSamples);
    }

    void controllerMoved (int controllerNumber, int newValue) override;

    int getVoiceId() { return voiceId; }

private:
    juce::AudioProcessorValueTreeState& state;

    int voiceId;

    void setFilterCutoffInternal (float curCutOff);
    void setFilterResonanceInternal (float curCutOff);

    /** Calculate LFO values. Called on the audio thread. */
    inline void updateLfo();

    //template <std::floating_point T>
    //void processEnvelope (juce::dsp::AudioBlock<T>& block);
    //template <std::floating_point T>
    //void processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    //template <std::floating_point T>
    //void processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    //template <std::floating_point T>
    //void applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples);
    //template <std::floating_point T>
    //void assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);

    template <std::floating_point T>
    void processEnvelope(juce::dsp::AudioBlock<T>& block)
    {
        auto samples = block.getNumSamples();
        auto numChannels = block.getNumChannels();

        for (auto i = 0; i < samples; ++i)
        {
            filterEnvelope = filterEnvADSR.getNextSample();
            auto env = ampADSR.getNextSample();

            for (int c = 0; c < numChannels; ++c)
                block.getChannelPointer(c)[i] *= env;
        }

        if (currentlyReleasingNote && !ampADSR.isActive())
        {
            currentlyReleasingNote = false;
            justDoneReleaseEnvelope = true;
            stopNote(0.f, false);

#if DEBUG_VOICES
            DBG("\tDEBUG ENVELOPPE DONE");
#endif
        }
    }

    template <std::floating_point T>
    void processRampUp(juce::dsp::AudioBlock<T>& block, int curBlockSize)
    {
#if DEBUG_VOICES
        DBG("\tDEBUG RAMP UP " + juce::String(rampUpSamples - rampUpSamplesLeft));
#endif
        auto curRampUpLenght = juce::jmin((int)curBlockSize, rampUpSamplesLeft);
        auto prevRampUpValue = (Constants::rampUpSamples - rampUpSamplesLeft) / (float)Constants::rampUpSamples;
        auto nextRampUpValue = prevRampUpValue + curRampUpLenght / (float)Constants::rampUpSamples;
        auto incr = (nextRampUpValue - prevRampUpValue) / (curRampUpLenght);

        jassert(nextRampUpValue >= 0.f && nextRampUpValue <= 1.0001f);

        for (int c = 0; c < block.getNumChannels(); ++c)
        {
            for (int i = 0; i < curRampUpLenght; ++i)
            {
                auto value = block.getSample(c, i);
                auto ramp = prevRampUpValue + i * incr;
                block.setSample(c, i, value * ramp);
            }
        }

        rampUpSamplesLeft -= curRampUpLenght;

        if (rampUpSamplesLeft <= 0)
        {
            rampingUp = false;
#if DEBUG_VOICES
            DBG("\tDEBUG RAMP UP DONE");
#endif
        }
    }

    template <std::floating_point T>
    void processKillOverlap(juce::dsp::AudioBlock<T>& block, int curBlockSize)
    {
#if DEBUG_VOICES
        DBG("\tDEBUG ADD OVERLAP" + juce::String(overlapIndex));
#endif

        auto curSamples = juce::jmin(Constants::killRampSamples - overlapIndex, (int)curBlockSize);

        for (int c = 0; c < block.getNumChannels(); ++c)
        {
            for (int i = 0; i < curSamples; ++i)
            {
                auto prev = block.getSample(c, i);
                auto overl = static_cast<T> (overlap->getSample(c, overlapIndex + i));
                auto total = prev + overl;

                jassert(total > -1 && total < 1);

                block.setSample(c, i, total);

#if PRINT_ALL_SAMPLES
                if (c == 0)
                    DBG("\tADD\t" + juce::String(prev) + "\t" + juce::String(overl) + "\t" + juce::String(total));
#endif
            }
        }

        overlapIndex += curSamples;

        if (overlapIndex >= Constants::killRampSamples)
        {
            overlapIndex = -1;
            voicesBeingKilled->erase(voiceId);
#if DEBUG_VOICES
            DBG("\tDEBUG OVERLAP DONE");
#endif
        }
    }

    template <std::floating_point T>
    void assertForDiscontinuities(juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix)
    {
        auto prev = outputBuffer.getSample(0, startSample);
        auto prevDiff = abs(outputBuffer.getSample(0, startSample + 1) - prev);

        for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
        {
            for (int i = startSample; i < startSample + numSamples; ++i)
            {
                //@TODO need some kind of compression to avoid values above 1.f...
                jassert(abs(outputBuffer.getSample(c, i)) < 1.5f);

                if (c == 0)
                {
#if PRINT_ALL_SAMPLES
                    DBG(dbgPrefix + juce::String(outputBuffer.getSample(0, i)));
#endif
                    auto cur = outputBuffer.getSample(0, i);
                    jassert(abs(cur - prev) < .2f);

                    auto curDiff = abs(cur - prev);
                    jassert(curDiff - prevDiff < .08f);

                    prev = cur;
                    prevDiff = curDiff;
                }
            }
        }
    }

    template <std::floating_point T>
    void applyKillRamp(juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
    {
        outputBuffer.applyGainRamp(startSample, numSamples, 1.f, 0.f);
        currentlyKillingVoice = false;

#if DEBUG_VOICES
        DBG("\tDEBUG START KILLRAMP");
        assertForDiscontinuities(outputBuffer, startSample, numSamples, "\tBUILDING KILLRAMP\t");
        DBG("\tDEBUG stop KILLRAMP");
#endif
    }


    PhatOscillators<T> oscillators;

    std::unique_ptr<juce::AudioBuffer<T>> overlap;
    int overlapIndex = -1;
    //@TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<T>, juce::dsp::Gain<T>> processorChain;

    juce::ADSR ampADSR, filterEnvADSR;
    juce::ADSR::Parameters ampParams, filterEnvParams;
    bool currentlyReleasingNote = false, justDoneReleaseEnvelope = false;

    float curFilterCutoff = Constants::defaultFilterCutoff;
    float curFilterResonance = Constants::defaultFilterResonance;

    //lfo stuff
    static constexpr auto lfoUpdateRate = 100;
    int lfoUpdateCounter = lfoUpdateRate;
    juce::dsp::Oscillator<T> lfo;
    std::mutex lfoMutex;
    T lfoAmount = static_cast<T> (Constants::defaultLfoAmount);
    LfoDest lfoDest;

    //for the random lfo
    juce::Random rng;
    float randomValue = 0.f;
    bool valueWasBig = false;

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    float filterEnvelope{};

    float tiltCutoff { 0.f };

    int curPreparedSamples = 0;
};
