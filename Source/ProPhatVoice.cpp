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
#include "UI/ButtonGroupComponent.h"

ProPhatVoice::ProPhatVoice (int vId, std::set<int>* activeVoiceSet)
    : voiceId (vId)
    , voicesBeingKilled (activeVoiceSet)
    , distribution (-1.f, 1.f)
{
    processorChain.get<(int)processorId::masterGainIndex>().setGainLinear (defaultOscLevel);
    setFilterCutoffInternal (defaultFilterCutoff);
    setFilterResonanceInternal (defaultFilterResonance);

    sub.setOscShape (OscShape::pulse);
    noise.setOscShape (OscShape::noise);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (defaultLfoFreq);
}

void ProPhatVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    //seems like auval doesn't initalize spec properly and we need to instantiate more memory than it's asking
    juce::PluginHostType host;
    const auto auvalMultiplier = host.getHostPath().contains ("auval") ? 5 : 1;

    osc1Block = juce::dsp::AudioBlock<float> (heapBlock1, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);
    osc2Block = juce::dsp::AudioBlock<float> (heapBlock2, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);
    noiseBlock = juce::dsp::AudioBlock<float> (heapBlockNoise, spec.numChannels, auvalMultiplier * spec.maximumBlockSize);

    overlap = std::make_unique<juce::AudioSampleBuffer> (juce::AudioSampleBuffer (spec.numChannels, killRampSamples));
    overlap->clear();

    sub.prepare (spec);
    noise.prepare (spec);
    osc1.prepare (spec);
    osc2.prepare (spec);
    processorChain.prepare (spec);

    ampADSR.setSampleRate (spec.sampleRate);
    ampADSR.setParameters (ampParams);

    filterEnvADSR.setSampleRate (spec.sampleRate);
    filterEnvADSR.setParameters (filterEnvParams);

    lfo.prepare ({spec.sampleRate / lfoUpdateRate, auvalMultiplier * spec.maximumBlockSize, spec.numChannels});
}

void ProPhatVoice::updateOscFrequencies()
{
    auto midiNote = getCurrentlyPlayingNote();

    if (midiNote < 0)
        return;

    auto pitchWheelDeltaNote = pitchWheelNoteRange.convertFrom0to1 (pitchWheelPosition / 16383.f);

    auto curOsc1Slop = slopOsc1 * slopMod;
    auto curOsc2Slop = slopOsc2 * slopMod;

    auto osc1FloatNote = midiNote - osc1NoteOffset + osc1TuningOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote + curOsc1Slop;
    sub.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote - 12), true);
    noise.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote), true);
    osc1.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote), true);

    auto osc2Freq = Helpers::getFloatMidiNoteInHertz (midiNote - osc2NoteOffset + osc2TuningOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote + curOsc2Slop);
    osc2.setFrequency ((float) osc2Freq, true);
}

void ProPhatVoice::setOscFreq (processorId oscNum, int newMidiNote)
{
    jassert (Helpers::valueContainedInRange (newMidiNote, midiNoteRange));

    switch (oscNum)
    {
        case processorId::osc1Index:
            osc1NoteOffset = middleCMidiNote - (float) newMidiNote;
            break;
        case processorId::osc2Index:
            osc2NoteOffset = middleCMidiNote - (float) newMidiNote;
            break;
        default:
            jassertfalse;
            break;
    }

    updateOscFrequencies ();
}

void ProPhatVoice::setOscShape (processorId oscNum, int newShape)
{
    switch (oscNum)
    {
        case processorId::osc1Index:
            osc1.setOscShape (newShape);
            break;
        case processorId::osc2Index:
            osc2.setOscShape (newShape);
            break;
        default:
            jassertfalse;
            break;
    }
}

void ProPhatVoice::setOscTuning (processorId oscNum, float newTuning)
{
    jassert (Helpers::valueContainedInRange (newTuning, tuningSliderRange));

    switch (oscNum)
    {
        case processorId::osc1Index:
            osc1TuningOffset = newTuning;
            break;
        case processorId::osc2Index:
            osc2TuningOffset = newTuning;
            break;
        default:
            jassertfalse;
            break;
    }
    updateOscFrequencies ();
}

void ProPhatVoice::setOscSub (float newSub)
{
    jassert (Helpers::valueContainedInRange (newSub, sliderRange));
    curSubLevel = newSub;
    updateOscLevels ();
}

void ProPhatVoice::setOscNoise (float noiseLevel)
{
    jassert (Helpers::valueContainedInRange (noiseLevel, sliderRange));
    curNoiseLevel = noiseLevel;
    updateOscLevels ();
}

void ProPhatVoice::setOscSlop (float slop)
{
    jassert (Helpers::valueContainedInRange (slop, slopSliderRange));
    slopMod = slop;
    updateOscFrequencies ();
}

void ProPhatVoice::setOscMix (float newMix)
{
    jassert (Helpers::valueContainedInRange (newMix, sliderRange));

    oscMix = newMix;
    updateOscLevels ();
}

void ProPhatVoice::updateOscLevels ()
{
    sub.setGain (curVelocity * curSubLevel);
    noise.setGain (curVelocity * curNoiseLevel);
    osc1.setGain (curVelocity * (1 - oscMix));
    osc2.setGain (curVelocity * oscMix);
}

void ProPhatVoice::setAmpParam (juce::StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == ProPhatAudioProcessorIDs::ampAttackID.getParamID())
        ampParams.attack = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::ampDecayID.getParamID())
        ampParams.decay = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::ampSustainID.getParamID())
        ampParams.sustain = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::ampReleaseID.getParamID())
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

    if (parameterID == ProPhatAudioProcessorIDs::filterEnvAttackID.getParamID())
        filterEnvParams.attack = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::filterEnvDecayID.getParamID())
        filterEnvParams.decay = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::filterEnvSustainID.getParamID())
        filterEnvParams.sustain = newValue;
    else if (parameterID == ProPhatAudioProcessorIDs::filterEnvReleaseID.getParamID())
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
    lfoOsc1NoteOffset = 0.f;
    lfoOsc2NoteOffset = 0.f;

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

void ProPhatVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
    updateOscFrequencies();
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscillate between [-1, 1]
inline void ProPhatVoice::updateLfo()
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
            lfoOsc1NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            updateOscFrequencies();
            break;

        case LfoDest::osc2Freq:
            lfoOsc2NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            updateOscFrequencies();
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
    processorChain.get<(int) processorId::filterIndex> ().setCutoffFrequencyHz (limitedCutOff);
}

inline void ProPhatVoice::setFilterResonanceInternal (float curResonance)
{
    const auto limitedResonance { juce::jlimit (0.f, 1.f, curResonance) };
    processorChain.get<(int) processorId::filterIndex> ().setResonance (limitedResonance);
}

void ProPhatVoice::startNote (int /*midiNoteNumber*/, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
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

    pitchWheelPosition = currentPitchWheelPosition;

    slopOsc1 = distribution (generator);
    slopOsc2 = distribution (generator);

    updateOscFrequencies();

    curVelocity = velocity;

    rampingUp = true;
    rampUpSamplesLeft = rampUpSamples;

    updateOscLevels();
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

    auto osc1Output = osc1Block.getSubBlock (0, (size_t) numSamples);
    osc1Output.clear();

    auto osc2Output = osc2Block.getSubBlock (0, (size_t) numSamples);
    osc2Output.clear();

    auto noiseOutput = noiseBlock.getSubBlock (0, (size_t) numSamples);
    noiseOutput.clear();

    for (size_t pos = 0; pos < numSamples;)
    {
        auto curBlockSize = juce::jmin (static_cast<size_t> (numSamples - pos), lfoUpdateCounter);

        //process osc1
        auto block1 = osc1Output.getSubBlock (pos, curBlockSize);
        juce::dsp::ProcessContextReplacing<float> osc1Context (block1);
        sub.process (osc1Context);
        osc1.process (osc1Context);

        //process osc2
        auto block2 = osc2Output.getSubBlock (pos, curBlockSize);
        juce::dsp::ProcessContextReplacing<float> osc2Context (block2);
        osc2.process (osc2Context);

        //process noise
        auto blockNoise = noiseOutput.getSubBlock (pos, curBlockSize);
        juce::dsp::ProcessContextReplacing<float> noiseContext (blockNoise);
        noise.process (noiseContext);

        //process the sum of osc1 and osc2
        blockNoise.add (block1);
        blockNoise.add (block2);
        juce::dsp::ProcessContextReplacing<float> summedContext (blockNoise);
        processorChain.process (summedContext);

        //during this call, the voice may become inactive, but we still have to finish this loop to ensure the voice stays muted for the rest of the buffer
        processEnvelope (blockNoise);

        if (rampingUp)
            processRampUp (blockNoise, (int) curBlockSize);

        if (overlapIndex > -1)
            processKillOverlap (blockNoise, (int) curBlockSize);

        pos += curBlockSize;
        lfoUpdateCounter -= curBlockSize;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
        }
    }

    juce::dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (noiseBlock);

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