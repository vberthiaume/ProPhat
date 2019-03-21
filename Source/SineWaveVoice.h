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
#include "../JuceLibraryCode/JuceHeader.h"
#include "Helpers.h"
#include <mutex>
#include <set>
#include "ButtonGroupComponent.h"

struct sBMP4Sound : public SynthesiserSound
{
    sBMP4Sound() {}

    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
template <typename Type>
class GainedOscillator
{
public:

    GainedOscillator()
    {
        setOscShape (OscShape::saw);
        setLevel (Constants::defaultOscLevel);
    }

    void setFrequency (Type newValue, bool force = false)
    {
        jassert (newValue > 0);

        auto& osc = processorChain.template get<oscIndex>();
        osc.setFrequency (newValue, force);
    }

    void setOscShape (int newShape);

    void setLevel (Type newValue)
    {
        if (!isActive)
            newValue = 0;
        else
            lastActiveLevel = newValue;

        auto& gain = processorChain.template get<gainIndex>();
        gain.setGainLinear (newValue);
    }

    Type getLevel()
    {
        return lastActiveLevel;
    }

    void reset() noexcept
    {
        processorChain.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        std::lock_guard<std::mutex> lock (processMutex);

        processorChain.process (context);
    }

    void prepare (const dsp::ProcessSpec& spec)
    {
        processorChain.prepare (spec);
    }

private:
    enum
    {
        oscIndex,
        gainIndex
    };

    std::mutex processMutex;

    bool isActive = true;

    Type lastActiveLevel{};

    dsp::ProcessorChain<dsp::Oscillator<Type>, dsp::Gain<Type>> processorChain;
};

//==============================================================================
using namespace Constants;
class sBMP4Voice : public SynthesiserVoice
{
public:

    enum processorId
    {
        filterIndex = 0,
        masterGainIndex,

        osc1Index = 0,
        osc2Index,
    };

    sBMP4Voice (int voiceId, std::set<int>* activeVoiceSet);

    void prepare (const dsp::ProcessSpec& spec);

    void updateOscFrequencies();

    void setOscFreq (processorId oscNum, int newMidiNote)
    {
        jassert (Helpers::valueContainedInRange (newMidiNote, midiNoteRange));

        switch (oscNum)
        {
            case sBMP4Voice::osc1Index:
                osc1NoteOffset = middleCMidiNote - (float) newMidiNote;
                break;
            case sBMP4Voice::osc2Index:
                osc2NoteOffset = middleCMidiNote - (float) newMidiNote;
                break;
            default:
                jassertfalse;
                break;
        }

        updateOscFrequencies();
    }

    void setOscShape (processorId oscNum, int newShape)
    {
        switch (oscNum)
        {
            case sBMP4Voice::osc1Index:
                osc1.setOscShape (newShape);
                break;
            case sBMP4Voice::osc2Index:
                osc2.setOscShape (newShape);
                break;
            default:
                jassertfalse;
                break;
        }
    }

    void setOscTuning (processorId oscNum, float newTuning)
    {
        jassert (Helpers::valueContainedInRange (newTuning, centeredSliderRange));

        switch (oscNum)
        {
            case sBMP4Voice::osc1Index:
                osc1TuningOffset = newTuning;
                break;
            case sBMP4Voice::osc2Index:
                osc2TuningOffset = newTuning;
                break;
            default:
                jassertfalse;
                break;
        }
        updateOscFrequencies();
    }

    void setOscSub (float newSub)
    {
        jassert (Helpers::valueContainedInRange (newSub, sliderRange));
        curSubLevel = newSub;
        updateOscLevels();
    }

    void setOscMix (float newMix)
    {
        jassert (Helpers::valueContainedInRange (newMix, sliderRange));

        oscMix = newMix;
        updateOscLevels();
    }

    void updateOscLevels()
    {
        sub.setLevel (curVelocity * curSubLevel);
        osc1.setLevel (curVelocity * (1 - oscMix));
        osc2.setLevel (curVelocity * oscMix);
    }

    void setAmpParam (StringRef parameterID, float newValue);
    void setFilterEnvParam (StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }
    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (float newValue)
    {
        curFilterCutoff = newValue;
        processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    }

    void setFilterResonance (float newAmount)
    {
        curFilterResonance = newAmount;
        processorChain.get<filterIndex>().setResonance (curFilterResonance);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition) override;

    void pitchWheelMoved (int newPitchWheelValue) override;

    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<sBMP4Sound*> (sound) != nullptr; }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/)  override {}

    int getVoiceId() { return voiceId; }

private:
    int voiceId;

    void updateLfo();
    void processEnvelope (dsp::AudioBlock<float>& block);
    void processRampUp (dsp::AudioBlock<float>& block, int curBlockSize);
    void processKillOverlap (dsp::AudioBlock<float>& block, int curBlockSize);
    void applyKillRamp (AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    void assertForDiscontinuities (AudioBuffer<float>& outputBuffer, int startSample, int numSamples, String dbgPrefix);

    HeapBlock<char> heapBlock1, heapBlock2;
    dsp::AudioBlock<float> osc1Block, osc2Block;
    GainedOscillator<float> sub, osc1, osc2;

    std::unique_ptr<AudioBuffer<float>> overlap;
    int overlapIndex = -1;
    //@TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    dsp::ProcessorChain<dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    ADSR ampADSR, filterEnvADSR;
    ADSR::Parameters ampParams, filterEnvParams;
    bool currentlyReleasingNote = false, justDoneReleaseEnvelope = false;

    float curFilterCutoff = defaultFilterCutoff;
    float curFilterResonance = defaultFilterResonance;

    //lfo stuff
    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;
    std::mutex lfoMutex;
    float lfoAmount = defaultLfoAmount;
    LfoDest lfoDest;
    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    //for the random lfo
    Random rng;
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

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    float filterEnvelope{};
};
