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

using namespace Constants;
class ProPhatVoice : public juce::SynthesiserVoice
{
public:

    enum class ProcessorId
    {
        filterIndex = 0,
        masterGainIndex,
    };

    ProPhatVoice (int voiceId, std::set<int>* activeVoiceSet);

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

    void pitchWheelMoved (int newPitchWheelValue) override;

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<ProPhatSound*> (sound) != nullptr; }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int controllerNumber, int newValue) override;

    int getVoiceId() { return voiceId; }

private:
    int voiceId;

    void setFilterCutoffInternal (float curCutOff);
    void setFilterResonanceInternal (float curCutOff);

    /** Calculate LFO values. Called on the audio thread. */
    inline void updateLfo();
    void processEnvelope (juce::dsp::AudioBlock<float>& block);
    void processRampUp (juce::dsp::AudioBlock<float>& block, int curBlockSize);
    void processKillOverlap (juce::dsp::AudioBlock<float>& block, int curBlockSize);
    void applyKillRamp (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    void assertForDiscontinuities (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);

    PhatOscillators oscillators;

    std::unique_ptr<juce::AudioBuffer<float>> overlap;
    int overlapIndex = -1;
    //@TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<float>, juce::dsp::Gain<float>> processorChain;

    juce::ADSR ampADSR, filterEnvADSR;
    juce::ADSR::Parameters ampParams, filterEnvParams;
    bool currentlyReleasingNote = false, justDoneReleaseEnvelope = false;

    float curFilterCutoff = defaultFilterCutoff;
    float curFilterResonance = defaultFilterResonance;

    //lfo stuff
    static constexpr auto lfoUpdateRate = 100;
    int lfoUpdateCounter = lfoUpdateRate;
    juce::dsp::Oscillator<float> lfo;
    std::mutex lfoMutex;
    float lfoAmount = defaultLfoAmount;
    LfoDest lfoDest;

    //for the random lfo
    juce::Random rng;
    float randomValue = 0.f;
    bool valueWasBig = false;

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    float filterEnvelope{};

    float tiltCutoff { 0.f };
};
