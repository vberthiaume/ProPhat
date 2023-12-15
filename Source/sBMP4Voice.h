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

#pragma once
#include "ButtonGroupComponent.h"
#include "GainedOscillator.h"
#include "Helpers.h"

#include <mutex>
#include <set>

struct sBMP4Sound : public juce::SynthesiserSound
{
    sBMP4Sound() {}

    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================

using namespace Constants;
class sBMP4Voice : public juce::SynthesiserVoice
{
public:

    enum class processorId
    {
        filterIndex = 0,
        masterGainIndex,

        osc1Index = 0,
        osc2Index,
    };

    sBMP4Voice (int voiceId, std::set<int>* activeVoiceSet);

    void prepare (const juce::dsp::ProcessSpec& spec);

    void updateOscFrequencies();

    void setOscFreq (processorId oscNum, int newMidiNote);
    void setOscShape (processorId oscNum, int newShape);
    void setOscTuning (processorId oscNum, float newTuning);
    void setOscSub (float newSub);
    void setOscNoise (float noiseLevel);
    void setOscSlop (float slop);
    void setOscMix (float newMix);

    void updateOscLevels();

    void setAmpParam (juce::StringRef parameterID, float newValue);
    void setFilterEnvParam (juce::StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }
    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (float newValue);
    void setFilterResonance (float newAmount);

    void pitchWheelMoved (int newPitchWheelValue) override;

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<sBMP4Sound*> (sound) != nullptr; }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/)  override {}

    int getVoiceId() { return voiceId; }

private:
    int voiceId;

    void updateLfo();
    void processEnvelope (juce::dsp::AudioBlock<float>& block);
    void processRampUp (juce::dsp::AudioBlock<float>& block, int curBlockSize);
    void processKillOverlap (juce::dsp::AudioBlock<float>& block, int curBlockSize);
    void applyKillRamp (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    void assertForDiscontinuities (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);

    juce::HeapBlock<char> heapBlock1, heapBlock2, heapBlockNoise;
    juce::dsp::AudioBlock<float> osc1Block, osc2Block, noiseBlock;
    GainedOscillator<float> sub, osc1, osc2, noise;

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
    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    juce::dsp::Oscillator<float> lfo;
    std::mutex lfoMutex;
    float lfoAmount = defaultLfoAmount;
    LfoDest lfoDest;
    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    //for the random lfo
    juce::Random rng;
    float randomValue = 0.f;
    bool valueWasBig = false;

    int pitchWheelPosition = 0;

    float osc1NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;
    float osc2NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;

    float osc1TuningOffset = 0.f;
    float osc2TuningOffset = 0.f;

    float curVelocity = 0.f;
    float curSubLevel = 0.f;
    float oscMix = 0.f;
    float curNoiseLevel = 0.f;

    float slopOsc1 = 0.f, slopOsc2 = 0.f, slopMod = 0.f;

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    float filterEnvelope{};

    std::uniform_real_distribution<float> distribution;
    std::default_random_engine generator;
};
