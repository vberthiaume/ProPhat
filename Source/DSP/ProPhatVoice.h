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
        if (! currentlyKillingVoice && ! isVoiceActive ())
            return;

        //reserve an audio block of size numSamples. Auvaltool has a tendency to _not_ call prepare before rendering
        //with new buffer sizes, so just making sure we're not taking more samples than the audio block was prepared with.
        numSamples = juce::jmin (numSamples, curPreparedSamples);
        auto currentAudioBlock { oscillators.prepareRender (numSamples) };

        for (int pos = 0; pos < numSamples;)
        {
            const auto subBlockSize = juce::jmin (numSamples - pos, lfoUpdateCounter);

            //render the oscillators
            auto oscBlock { oscillators.process (pos, subBlockSize) };

            //render our effects
            juce::dsp::ProcessContextReplacing<T> oscContext (oscBlock);
            processorChain.process (oscContext);

            //apply the enveloppes. We calculate and apply the amp envelope on a sample basis,
            //but for the filter env we increment it on a sample basis but only apply it
            //once per buffer, just like the LFO -- see below.
            auto filterEnvelope { 0.f };
            {
                const auto numChannels { oscBlock.getNumChannels () };
                for (auto i = 0; i < subBlockSize; ++i)
                {
                    //calculate and atore filter envelope
                    filterEnvelope = filterADSR.getNextSample ();

                    //calculate and apply amp envelope
                    const auto ampEnv = ampADSR.getNextSample ();
                    for (int c = 0; c < numChannels; ++c)
                        oscBlock.getChannelPointer (c)[i] *= ampEnv;
                }

                if (currentlyReleasingNote && ! ampADSR.isActive ())
                {
                    currentlyReleasingNote = false;
                    justDoneReleaseEnvelope = true;
                    stopNote (0.f, false);

#if DEBUG_VOICES
                    DBG ("\tDEBUG ENVELOPPE DONE");
#endif
                }
            }

            if (rampingUp)
                processRampUp (oscBlock, (int) subBlockSize);

            if (overlapIndex > -1)
                processKillOverlap (oscBlock, (int) subBlockSize);

            //update our lfos at the end of the block
            lfoUpdateCounter -= subBlockSize;
            if (lfoUpdateCounter == 0)
            {
                lfoUpdateCounter = lfoUpdateRate;
                updateLfo ();
            }

            //apply our filter envelope once per buffer
            const auto curCutOff { (curFilterCutoff + tiltCutoff) * (1 + envelopeAmount * filterEnvelope) + lfoCutOffContributionHz };
            setFilterCutoffInternal (curCutOff);

            //increment our position
            pos += subBlockSize;
        }

        //add everything to the output buffer
        juce::dsp::AudioBlock<T> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (currentAudioBlock);

        if (currentlyKillingVoice)
            applyKillRamp (outputBuffer, startSample, numSamples);
#if DEBUG_VOICES
        else
            assertForDiscontinuities (outputBuffer, startSample, numSamples, {});
#endif
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    void renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int controllerNumber, int newValue) override;

    int getVoiceId() { return voiceId; }

private:
    juce::AudioProcessorValueTreeState& state;

    int voiceId;

    float lfoCutOffContributionHz { 0.f };
    void setFilterCutoffInternal (float curCutOff);
    void setFilterResonanceInternal (float curCutOff);

    /** Calculate LFO values. Called on the audio thread. */
    inline void updateLfo();
    void processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);
    void applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples);

    PhatOscillators<T> oscillators;

    std::unique_ptr<juce::AudioBuffer<T>> overlap;
    int overlapIndex = -1;
    //TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<T>, juce::dsp::Gain<T>> processorChain;
    //TODO: use a slider for this
    static constexpr auto envelopeAmount { 2 };

    juce::ADSR ampADSR, filterADSR;
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
    T randomValue = 0.f;
    bool valueWasBig = false;

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    float tiltCutoff { 0.f };

    int curPreparedSamples = 0;
};
