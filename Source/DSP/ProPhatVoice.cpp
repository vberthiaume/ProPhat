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

#include "ProPhatVoice.h"
#include "../UI/ButtonGroupComponent.h"
#include "../Utility/Macros.h"

ProPhatVoice::ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int vId, std::set<int>* activeVoiceSet)
    : state (processorState)
    , oscillators (state)
    , voiceId (vId)
    , voicesBeingKilled (activeVoiceSet)
{
    addParamListenersToState ();

    processorChain.get<(int)ProcessorId::masterGainIndex>().setGainLinear (defaultOscLevel);
    setFilterCutoffInternal (defaultFilterCutoff);
    setFilterResonanceInternal (defaultFilterResonance);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (defaultLfoFreq);
}

void ProPhatVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    oscillators.prepare (spec);

    overlap = std::make_unique<juce::AudioSampleBuffer> (juce::AudioSampleBuffer (spec.numChannels, killRampSamples));
    overlap->clear();

    processorChain.prepare (spec);

    ampADSR.setSampleRate (spec.sampleRate);
    ampADSR.setParameters (ampParams);

    filterEnvADSR.setSampleRate (spec.sampleRate);
    filterEnvADSR.setParameters (filterEnvParams);

    //seems like auval doesn't initalize spec properly and we need to instantiate more memory than it's asking
    const auto auvalMultiplier = juce::PluginHostType ().getHostPath ().contains ("auval") ? 5 : 1;
    lfo.prepare ({spec.sampleRate / lfoUpdateRate, auvalMultiplier * spec.maximumBlockSize, spec.numChannels});
}

void ProPhatVoice::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    //add our synth as listener to all parameters so we can do automations
    state.addParameterListener (filterCutoffID.getParamID (), this);
    state.addParameterListener (filterResonanceID.getParamID (), this);
    state.addParameterListener (filterEnvAttackID.getParamID (), this);
    state.addParameterListener (filterEnvDecayID.getParamID (), this);
    state.addParameterListener (filterEnvSustainID.getParamID (), this);
    state.addParameterListener (filterEnvReleaseID.getParamID (), this);

    state.addParameterListener (ampAttackID.getParamID (), this);
    state.addParameterListener (ampDecayID.getParamID (), this);
    state.addParameterListener (ampSustainID.getParamID (), this);
    state.addParameterListener (ampReleaseID.getParamID (), this);

    state.addParameterListener (lfoShapeID.getParamID (), this);
    state.addParameterListener (lfoDestID.getParamID (), this);
    state.addParameterListener (lfoFreqID.getParamID (), this);
    state.addParameterListener (lfoAmountID.getParamID (), this);
}

void ProPhatVoice::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatVoice::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == ampAttackID.getParamID ()
             || parameterID == ampDecayID.getParamID ()
             || parameterID == ampSustainID.getParamID ()
             || parameterID == ampReleaseID.getParamID ())
        setAmpParam (parameterID, newValue);

    else if (parameterID == filterEnvAttackID.getParamID ()
             || parameterID == filterEnvDecayID.getParamID ()
             || parameterID == filterEnvSustainID.getParamID ()
             || parameterID == filterEnvReleaseID.getParamID ())
        setFilterEnvParam (parameterID, newValue);

    else if (parameterID == lfoShapeID.getParamID ())
        setLfoShape ((int) newValue);
    else if (parameterID == lfoDestID.getParamID ())
        setLfoDest ((int) newValue);
    else if (parameterID == lfoFreqID.getParamID ())
        setLfoFreq (newValue);
    else if (parameterID == lfoAmountID.getParamID ())
        setLfoAmount (newValue);

    else if (parameterID == filterCutoffID.getParamID ())
        setFilterCutoff (newValue);
    else if (parameterID == filterResonanceID.getParamID ())
        setFilterResonance (newValue);

    else
        jassertfalse;
}

void ProPhatVoice::setAmpParam (juce::StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == ProPhatParameterIds::ampAttackID.getParamID())
        ampParams.attack = newValue;
    else if (parameterID == ProPhatParameterIds::ampDecayID.getParamID())
        ampParams.decay = newValue;
    else if (parameterID == ProPhatParameterIds::ampSustainID.getParamID())
        ampParams.sustain = newValue;
    else if (parameterID == ProPhatParameterIds::ampReleaseID.getParamID())
        ampParams.release = newValue;

    ampADSR.setParameters (ampParams);
}

void ProPhatVoice::setFilterEnvParam (juce::StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == ProPhatParameterIds::filterEnvAttackID.getParamID())
        filterEnvParams.attack = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvDecayID.getParamID())
        filterEnvParams.decay = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvSustainID.getParamID())
        filterEnvParams.sustain = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvReleaseID.getParamID())
        filterEnvParams.release = newValue;

    filterEnvADSR.setParameters (filterEnvParams);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscilate between [-1, 1]
void ProPhatVoice::setLfoShape (int shape)
{
    switch (shape)
    {
        case LfoShape::triangle:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](float x) { return (std::sin (x) + 1) / 2; }, 128);
        }
            break;

        case LfoShape::saw:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](float x)
            {
                //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                return (float) juce::jmap (x, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, 0.f, 1.f);
            }, 2);
        }
            break;

        //@TODO add this once we have more room in the UI for lfo destinations
        /*
        case LfoShape::revSaw:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](float x)
            {
                return (float) juce::jmap (x, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, 1.f, 0.f);
            }, 2);
        }
            break;
        */

        case LfoShape::square:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](float x)
            {
                if (x < 0.f)
                    return 0.f;
                else
                    return 1.f;
            });
        }
            break;

        case LfoShape::random:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([this](float x)
            {
                if (x <= 0.f && valueWasBig)
                {
                    randomValue = rng.nextFloat()/* * 2 - 1*/;
                    valueWasBig = false;
                }
                else if (x > 0.f && ! valueWasBig)
                {
                    randomValue = rng.nextFloat()/* * 2 - 1*/;
                    valueWasBig = true;
                }

                return randomValue;
            });
        }
            break;

        default:
            jassertfalse;
            break;
    }
}

void ProPhatVoice::setLfoDest (int dest)
{
    //reset everything
    oscillators.resetLfoOscNoteOffsets ();

    //change the destination
    lfoDest.curSelection = dest;
}

void ProPhatVoice::setFilterCutoff (float newValue)
{
    curFilterCutoff = newValue;
    setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
}

void ProPhatVoice::setFilterTiltCutoff (float newValue)
{
    tiltCutoff = newValue;
    setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
}

void ProPhatVoice::setFilterResonance (float newAmount)
{
    curFilterResonance = newAmount;
    setFilterResonanceInternal (curFilterResonance);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscillate between [-1, 1]
void ProPhatVoice::updateLfo()
{
    //apply filter envelope
    //TODO make this into a slider
    const auto envelopeAmount = 2;

    float lfoOut;
    {
        //TODO: LOCK IN AUDIO THREAD
        std::lock_guard<std::mutex> lock (lfoMutex);
        lfoOut = lfo.processSample (0.0f) * lfoAmount;
    }

    //TODO get this switch out of here, this is awful for performances
    switch (lfoDest.curSelection)
    {
        case LfoDest::osc1Freq:
            //lfoOsc1NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            //oscillators.updateOscFrequencies();
            oscillators.setLfoOsc1NoteOffset (lfoNoteRange.convertFrom0to1 (lfoOut));
            break;

        case LfoDest::osc2Freq:
            /*lfoOsc2NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            oscillators.updateOscFrequencies();*/
            oscillators.setLfoOsc2NoteOffset (lfoNoteRange.convertFrom0to1 (lfoOut));
            break;

        case LfoDest::filterCutOff:
        {
            const auto lfoCutOffContributionHz { juce::jmap (lfoOut, 0.0f, 1.0f, 10.0f, 10000.0f) };
            const auto curCutOff { (curFilterCutoff + tiltCutoff) * (1 + envelopeAmount * filterEnvelope) + lfoCutOffContributionHz };
            setFilterCutoffInternal (curCutOff);
        }
        break;

        case LfoDest::filterResonance:
            setFilterResonanceInternal (curFilterResonance * (1 + envelopeAmount * lfoOut));
            break;

        default:
            break;
    }
}

inline void ProPhatVoice::setFilterCutoffInternal (float curCutOff)
{
    const auto limitedCutOff { juce::jlimit (cutOffRange.start, cutOffRange.end, curCutOff) };
    processorChain.get<(int) ProcessorId::filterIndex> ().setCutoffFrequencyHz (limitedCutOff);
}

inline void ProPhatVoice::setFilterResonanceInternal (float curResonance)
{
    const auto limitedResonance { juce::jlimit (0.f, 1.f, curResonance) };
    processorChain.get<(int) ProcessorId::filterIndex> ().setResonance (limitedResonance);
}

void ProPhatVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG start: " + juce::String (voiceId));
#endif

    ampADSR.setParameters (ampParams);
    ampADSR.reset();
    ampADSR.noteOn();

    filterEnvADSR.setParameters (filterEnvParams);
    filterEnvADSR.reset();
    filterEnvADSR.noteOn();

    oscillators.updateOscFrequencies (midiNoteNumber, velocity, currentPitchWheelPosition);

    rampingUp = true;
    rampUpSamplesLeft = rampUpSamples;

    oscillators.updateOscLevels();
}

void ProPhatVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        currentlyReleasingNote = true;
        ampADSR.noteOff();
        filterEnvADSR.noteOff();

#if DEBUG_VOICES
        DBG ("\tDEBUG tailoff voice: " + juce::String (voiceId));
#endif
    }
    else
    {
        if (getSampleRate() != 0.f && ! justDoneReleaseEnvelope)
        {
            rampingUp = false;

            overlap->clear();
            voicesBeingKilled->insert (voiceId);
            currentlyKillingVoice = true;
            renderNextBlock (*overlap, 0, killRampSamples);
            overlapIndex = 0;
        }

        justDoneReleaseEnvelope = false;
        clearCurrentNote();

#if DEBUG_VOICES
        DBG ("\tDEBUG kill voice: " + juce::String (voiceId));
#endif
    }
}

void ProPhatVoice::processEnvelope (juce::dsp::AudioBlock<float>& block)
{
    auto samples = block.getNumSamples();
    auto numChannels = block.getNumChannels();

    for (auto i = 0; i < samples; ++i)
    {
        filterEnvelope = filterEnvADSR.getNextSample();
        auto env = ampADSR.getNextSample();

        for (int c = 0; c < numChannels; ++c)
            block.getChannelPointer (c)[i] *= env;
    }

    if (currentlyReleasingNote && !ampADSR.isActive())
    {
        currentlyReleasingNote = false;
        justDoneReleaseEnvelope = true;
        stopNote (0.f, false);

#if DEBUG_VOICES
        DBG ("\tDEBUG ENVELOPPE DONE");
#endif
    }
}

void ProPhatVoice::processRampUp (juce::dsp::AudioBlock<float>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG RAMP UP " + juce::String (rampUpSamples - rampUpSamplesLeft));
#endif
    auto curRampUpLenght = juce::jmin ((int) curBlockSize, rampUpSamplesLeft);
    auto prevRampUpValue = (rampUpSamples - rampUpSamplesLeft) / (float) rampUpSamples;
    auto nextRampUpValue = prevRampUpValue + curRampUpLenght / (float) rampUpSamples;
    auto incr = (nextRampUpValue - prevRampUpValue) / (curRampUpLenght);

    jassert (nextRampUpValue >= 0.f && nextRampUpValue <= 1.0001f);

    for (int c = 0; c < block.getNumChannels(); ++c)
    {
        for (int i = 0; i < curRampUpLenght; ++i)
        {
            auto value = block.getSample (c, i);
            auto ramp = prevRampUpValue + i * incr;
            block.setSample (c, i, value * ramp);
        }
    }

    rampUpSamplesLeft -= curRampUpLenght;

    if (rampUpSamplesLeft <= 0)
    {
        rampingUp = false;
#if DEBUG_VOICES
        DBG ("\tDEBUG RAMP UP DONE");
#endif
    }
}

void ProPhatVoice::processKillOverlap (juce::dsp::AudioBlock<float>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ADD OVERLAP" + juce::String (overlapIndex));
#endif

    auto curSamples = juce::jmin (killRampSamples - overlapIndex, (int) curBlockSize);

    for (int c = 0; c < block.getNumChannels(); ++c)
    {
        for (int i = 0; i < curSamples; ++i)
        {
            auto prev = block.getSample (c, i);
            auto overl = overlap->getSample (c, overlapIndex + i);
            auto total = prev + overl;

            jassert (total > -1 && total < 1);

            block.setSample (c, i, total);

#if PRINT_ALL_SAMPLES
            if (c == 0)
                DBG ("\tADD\t" + juce::String (prev) + "\t" + juce::String (overl) + "\t" + juce::String (total));
#endif
        }
    }

    overlapIndex += curSamples;

    if (overlapIndex >= killRampSamples)
    {
        overlapIndex = -1;
        voicesBeingKilled->erase (voiceId);
#if DEBUG_VOICES
        DBG ("\tDEBUG OVERLAP DONE");
#endif
    }
}

void ProPhatVoice::assertForDiscontinuities (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix)
{
    auto prev = outputBuffer.getSample (0, startSample);
    auto prevDiff = abs (outputBuffer.getSample (0, startSample + 1) - prev);

    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
    {
        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            //@TODO need some kind of compression to avoid values above 1.f...
            jassert (abs (outputBuffer.getSample (c, i)) < 1.5f);

            if (c == 0)
            {
#if PRINT_ALL_SAMPLES
                DBG (dbgPrefix + juce::String (outputBuffer.getSample (0, i)));
#endif
                auto cur = outputBuffer.getSample (0, i);
                jassert (abs (cur - prev) < .2f);

                auto curDiff = abs (cur - prev);
                jassert (curDiff - prevDiff < .08f);

                prev = cur;
                prevDiff = curDiff;
            }
        }
    }
}

void ProPhatVoice::applyKillRamp (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.applyGainRamp (startSample, numSamples, 1.f, 0.f);
    currentlyKillingVoice = false;

#if DEBUG_VOICES
    DBG ("\tDEBUG START KILLRAMP");
    assertForDiscontinuities (outputBuffer, startSample, numSamples, "\tBUILDING KILLRAMP\t");
    DBG ("\tDEBUG stop KILLRAMP");
#endif
}

void ProPhatVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! currentlyKillingVoice && ! isVoiceActive())
        return;

    //TODO: BIG BIG if here lol
    auto totalBlockQuestionMark = oscillators.prepareRender (numSamples);

    for (int pos = 0; pos < numSamples;)
    {
        const auto curBlockSize = juce::jmin (numSamples - pos, lfoUpdateCounter);

        auto oscBlock { oscillators.process (pos, curBlockSize) };

        juce::dsp::ProcessContextReplacing<float> oscContext (oscBlock);
        processorChain.process (oscContext);

        //during this call, the voice may become inactive, but we still have to finish this loop to ensure the voice stays muted for the rest of the buffer
        processEnvelope (oscBlock);

        if (rampingUp)
            processRampUp (oscBlock, (int) curBlockSize);

        if (overlapIndex > -1)
            processKillOverlap (oscBlock, (int) curBlockSize);

        pos += curBlockSize;
        lfoUpdateCounter -= curBlockSize;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
        }
    }

    juce::dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (totalBlockQuestionMark);

    if (currentlyKillingVoice)
        applyKillRamp (outputBuffer, startSample, numSamples);
#if DEBUG_VOICES
    else
        assertForDiscontinuities (outputBuffer, startSample, numSamples, {});
#endif
}

//TODO: I think we need to catch this controller moved business somewhere higher up, like in the processor, where we have access to the state
//and then we can set the paramId right in the state and have both the audio and the UI change with the orba tilt
void ProPhatVoice::controllerMoved (int controllerNumber, int newValue)
{
    //DBG ("controllerNumber: " + juce::String (controllerNumber) + ", newValue: " + juce::String (newValue));

    //1 == orba tilt. The newValue range [0-127] is converted to [curFilterCutoff, cutOffRange.end]
    if (controllerNumber == 1)
        setFilterTiltCutoff (juce::jmap (static_cast<float> (newValue), 0.f, 127.f, curFilterCutoff, cutOffRange.end));
}
