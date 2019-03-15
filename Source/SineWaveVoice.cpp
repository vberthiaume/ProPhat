/*
  ==============================================================================

    SineWaveVoice.cpp
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#include "SineWaveVoice.h"
#include "ButtonGroupComponent.h"

template <typename Type>
void GainedOscillator<Type>::setOscShape (int newShape)
{
    auto& osc = processorChain.template get<oscIndex>();

    bool wasActive = isActive;
    isActive = true;

    switch (newShape)
    {
        case OscShape::none:
            isActive = false;
            break;

        case OscShape::saw:
        {
            std::lock_guard<std::mutex> lock (processMutex);
            osc.initialise ([](Type x)
            {
                //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                return jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1));
            }, 2);
        }
        break;

        case OscShape::sawTri:
        {
            std::lock_guard<std::mutex> lock (processMutex);
            osc.initialise ([](Type x)
            {
                Type y = jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1)) / 2;

                if (x < 0)
                    return y += jmap (x, Type (-MathConstants<double>::pi), Type (0), Type (-1), Type (1)) / 2;
                else
                    return y += jmap (x, Type (0), Type (MathConstants<double>::pi), Type (1), Type (-1)) / 2;

            }, 128);
        }
        break;

        case OscShape::triangle:
        {
            std::lock_guard<std::mutex> lock (processMutex);
            osc.initialise ([](Type x)
            {
                if (x < 0)
                    return jmap (x, Type (-MathConstants<double>::pi), Type (0), Type (-1), Type (1));
                else
                    return jmap (x, Type (0), Type (MathConstants<double>::pi), Type (1), Type (-1));

            }, 128);
        }
        break;

        case OscShape::pulse:
        {
            std::lock_guard<std::mutex> lock (processMutex);
            osc.initialise ([](Type x)
            {
                if (x < 0)
                    return Type (-1);
                else
                    return Type (1);
            }, 128);
        }
        break;

        case OscShape::total:
            jassertfalse;
            break;
        default:
            break;
    }

    if (wasActive != isActive)
    {
        if (isActive)
            setLevel (lastActiveLevel);
        else
            setLevel (0);
    }
}


sBMP4Voice::sBMP4Voice (int vId, std::set<int>* activeVoiceSet) :
voiceId (vId),
voicesBeingKilled (activeVoiceSet)
{
    processorChain.get<masterGainIndex>().setGainLinear (defaultOscLevel);
    processorChain.get<filterIndex>().setCutoffFrequencyHz (defaultFilterCutoff);
    processorChain.get<filterIndex>().setResonance (defaultFilterResonance);

    sub.setOscShape (OscShape::pulse);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (defaultLfoFreq);
}

void sBMP4Voice::prepare (const dsp::ProcessSpec& spec)
{
    osc1Block = dsp::AudioBlock<float> (heapBlock1, spec.numChannels, spec.maximumBlockSize);
    osc2Block = dsp::AudioBlock<float> (heapBlock2, spec.numChannels, spec.maximumBlockSize);

    overlap = std::make_unique<AudioSampleBuffer> (AudioSampleBuffer (spec.numChannels, killRampSamples));
    overlap->clear();

    sub.prepare (spec);
    osc1.prepare (spec);
    osc2.prepare (spec);
    processorChain.prepare (spec);

    adsr.setSampleRate (spec.sampleRate);
    adsr.setParameters (curParams);

    lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
}

void sBMP4Voice::updateOscFrequencies()
{
    auto midiNote = getCurrentlyPlayingNote();

    if (midiNote < 0)
        return;

    auto pitchWheelDeltaNote = pitchWheelNoteRange.convertFrom0to1 (pitchWheelPosition / 16383.f);

    auto osc1FloatNote = midiNote - osc1NoteOffset + osc1TuningOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote;
    sub.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote - 12), true);
    osc1.setFrequency ((float) Helpers::getFloatMidiNoteInHertz (osc1FloatNote), true);

    auto osc2Freq = Helpers::getFloatMidiNoteInHertz (midiNote - osc2NoteOffset + osc2TuningOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote);
    osc2.setFrequency ((float) osc2Freq, true);
}

void sBMP4Voice::setAmpParam (StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == sBMP4AudioProcessorIDs::ampAttackID)
        curParams.attack = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampDecayID)
        curParams.decay = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampSustainID)
        curParams.sustain = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
        curParams.release = newValue;

    adsr.setParameters (curParams);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (an only that one) should oscilate between [-1, 1]
void sBMP4Voice::setLfoShape (int shape)
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
                return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 0.f, 1.f);
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
                return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 1.f, 0.f);
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

        case LfoShape::total:
        default:
            jassertfalse;
            break;
    }
}

void sBMP4Voice::setLfoDest (int dest)
{
    //reset everything
    lfoOsc1NoteOffset = 0.f;
    lfoOsc2NoteOffset = 0.f;
    processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    processorChain.get<filterIndex>().setResonance (curFilterResonance);

    //change the destination
    lfoDest.curSelection = dest;
}

void sBMP4Voice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
    updateOscFrequencies();
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (an only that one) should oscilate between [-1, 1]
void sBMP4Voice::updateLfo()
{
    float lfoOut;
    {
        std::lock_guard<std::mutex> lock (lfoMutex);
        lfoOut = lfo.processSample (0.0f) * lfoAmount;
    }

    //@TODO get this switch out of here, this is awful for performances
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

        case LfoDest::filterCurOff:
        {
            auto curoffFreqHz = jmap (lfoOut, 0.0f, 1.0f, 100.0f, 2000.0f);
            processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff + curoffFreqHz);
        }
        break;

        case LfoDest::filterResonance:
            processorChain.get<filterIndex>().setResonance (lfoOut);
            break;

        default:
            break;
    }
}

void sBMP4Voice::startNote (int /*midiNoteNumber*/, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG start: " + String (voiceId));
#endif

    adsr.setParameters (curParams);
    adsr.reset();
    adsr.noteOn();

    pitchWheelPosition = currentPitchWheelPosition;
    updateOscFrequencies();

    curVelocity = velocity;

    rampingUp = true;
    rampUpSamplesLeft = rampUpSamples;

    updateOscLevels();
}

void sBMP4Voice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        currentlyReleasingNote = true;
        adsr.noteOff();

#if DEBUG_VOICES
        DBG ("\tDEBUG tailoff voice: " + String (voiceId));
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
        DBG ("\tDEBUG kill voice: " + String (voiceId));
#endif
    }
}

void sBMP4Voice::processEnvelope (dsp::AudioBlock<float>& block)
{
    auto samples = block.getNumSamples();
    auto numChannels = block.getNumChannels();

    float env{};
    for (auto i = 0; i < samples; ++i)
    {
        env = adsr.getNextSample();

        for (int c = 0; c < numChannels; ++c)
            block.getChannelPointer (c)[i] *= env;
    }

    if (currentlyReleasingNote && !adsr.isActive())
    {
        currentlyReleasingNote = false;
        justDoneReleaseEnvelope = true;
        stopNote (0.f, false);

#if DEBUG_VOICES
        DBG ("\tDEBUG ENVELOPPE DONE");
#endif
    }
}

void sBMP4Voice::processRampUp (dsp::AudioBlock<float>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG RAMP UP " + String (rampUpSamples - rampUpSamplesLeft));
#endif
    auto curRampUpLenght = jmin ((int) curBlockSize, rampUpSamplesLeft);
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

void sBMP4Voice::processKillOverlap (dsp::AudioBlock<float>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ADD OVERLAP" + String (overlapIndex));
#endif

    auto curSamples = jmin (killRampSamples - overlapIndex, (int) curBlockSize);

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
                DBG ("\tADD\t" + String (prev) + "\t" + String (overl) + "\t" + String (total));
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

void sBMP4Voice::assertForDiscontinuities (AudioBuffer<float>& outputBuffer, int startSample, int numSamples, String dbgPrefix)
{
    auto prev = outputBuffer.getSample (0, startSample);
    auto prevDiff = abs (outputBuffer.getSample (0, startSample + 1) - prev);

    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
    {
        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            //@TODO need some kind of compression to avoid valoes about 1.f...
            auto curSample = outputBuffer.getSample (c, i);
            jassert (abs (curSample) < 1.5f);

            if (c == 0)
            {
#if PRINT_ALL_SAMPLES
                DBG (dbgPrefix + String (outputBuffer.getSample (0, i)));
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

void sBMP4Voice::applyKillRamp (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.applyGainRamp (startSample, numSamples, 1.f, 0.f);
    currentlyKillingVoice = false;

#if DEBUG_VOICES
    DBG ("\tDEBUG START KILLRAMP");
    assertForDiscontinuities (outputBuffer, startSample, numSamples, "\tBUILDING KILLRAMP\t");
    DBG ("\tDEBUG stop KILLRAMP");
#endif

}

void sBMP4Voice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! currentlyKillingVoice && ! isVoiceActive())
        return;

    auto osc1Output = osc1Block.getSubBlock (0, (size_t) numSamples);
    osc1Output.clear();

    auto osc2Output = osc2Block.getSubBlock (0, (size_t) numSamples);
    osc2Output.clear();

    for (size_t pos = 0; pos < numSamples;)
    {
        auto curBlockSize = jmin (static_cast<size_t> (numSamples - pos), lfoUpdateCounter);

        //process osc1
        auto block1 = osc1Output.getSubBlock (pos, curBlockSize);
        dsp::ProcessContextReplacing<float> osc1Context (block1);
        sub.process (osc1Context);
        osc1.process (osc1Context);

        //process osc2
        auto block2 = osc2Output.getSubBlock (pos, curBlockSize);
        dsp::ProcessContextReplacing<float> osc2Context (block2);
        osc2.process (osc2Context);

        //process the sum of osc1 and osc2
        block2.add (block1);
        dsp::ProcessContextReplacing<float> summedContext (block2);
        processorChain.process (summedContext);

        //during this call, the voice may become inactive, but we still have to finish this loop to ensure the voice stays muted for the rest of the buffer
        processEnvelope (block2);

        if (rampingUp)
            processRampUp (block2, (int) curBlockSize);

        if (overlapIndex > -1)
            processKillOverlap (block2, (int) curBlockSize);

        pos += curBlockSize;
        lfoUpdateCounter -= curBlockSize;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
        }
    }

    dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (osc2Block);

    if (currentlyKillingVoice)
        applyKillRamp (outputBuffer, startSample, numSamples);
#if DEBUG_VOICES
    else
        assertForDiscontinuities (outputBuffer, startSample, numSamples, {});
#endif
}
