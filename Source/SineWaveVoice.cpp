/*
  ==============================================================================

    SineWaveVoice.cpp
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#include "SineWaveVoice.h"

sBMP4Voice::sBMP4Voice()
{
    auto& masterGain = processorChain.get<masterGainIndex>();
    masterGain.setGainLinear (0.7f);

    auto& filter = processorChain.get<filterIndex>();
    filter.setCutoffFrequencyHz (1000.0f);
    filter.setResonance (0.7f);

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (3.0f);
}

void sBMP4Voice::prepare (const dsp::ProcessSpec& spec)
{
    tempBlock = dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
    processorChain.prepare (spec);

    adsr.setSampleRate (spec.sampleRate);
    adsr.setParameters (params);

    lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});

    isPrepared = true;
}

void sBMP4Voice::updateOscFrequencies()
{
    auto pitchWheelDeltaNote = jmap ((float) pitchWheelPosition, 0.f, 16383.f, -2.f, 2.f);

    auto osc1Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc1NoteOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote);
    processorChain.get<osc1Index>().setFrequency ((float) osc1Freq, true);

    auto osc2Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc2NoteOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote);
    processorChain.get<osc2Index>().setFrequency ((float) osc2Freq, true);
}

/**
    So here, we have to transform newMidiNote into a delta.
    The frequency to use for oscilators will be whatever
*/
void sBMP4Voice::setOscTuning (processorId oscNum, int newMidiNote)
{
    switch (oscNum)
    {
        case sBMP4Voice::osc1Index:
            osc1NoteOffset = middleCMidiNote - (float) newMidiNote;
            updateOscFrequencies();
            break;
        case sBMP4Voice::osc2Index:
            osc2NoteOffset = middleCMidiNote - (float) newMidiNote;
            updateOscFrequencies();
            break;
        default:
            jassertfalse;
            break;
    }
}

void sBMP4Voice::setOscShape (processorId oscNum, OscShape newShape)
{
    if (oscNum == osc1Index)
        processorChain.get<osc1Index>().setOscShape (newShape);
    else if (oscNum == osc2Index)
        processorChain.get<osc2Index>().setOscShape (newShape);
}

void sBMP4Voice::setAmpParam (StringRef parameterID, float newValue)
{
    if (parameterID == sBMP4AudioProcessorIDs::ampAttackID)
        params.attack = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampDecayID)
        params.decay = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampSustainID)
        params.sustain = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
        params.release = newValue;

    adsr.setParameters (params);
}

void sBMP4Voice::setLfoShape (LfoShape shape)
{
    switch (shape)
    {
        case LfoShape::triangle:
            lfo.initialise ([](float x) { return std::sin (x); }, 128);
            break;

        case LfoShape::saw:
            lfo.initialise ([](float x)
            {
                //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, -1.f, 1.f);
            }, 2);
            break;

        case LfoShape::revSaw:
            lfo.initialise ([](float x)
            {
                return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 1.f, -1.f);
            }, 2);
            break;

        case LfoShape::square:
            lfo.initialise ([](float x)
            {
                if (x < 0.f)
                    return 0.f;
                else
                    return 1.f;
            });
            break;

        case LfoShape::random:
            lfo.initialise ([this](float x)
            {
                if (x <= 0.f && valueWasBig)
                {
                    randomValue = rng.nextFloat() * 2 - 1;
                    valueWasBig = false;
                }
                else if (x > 0.f && ! valueWasBig)
                {
                    randomValue = rng.nextFloat() * 2 - 1;
                    valueWasBig = true;
                }

                return randomValue;
            });
            break;

        case LfoShape::none:
        case LfoShape::total:
        default:
            jassertfalse;
            break;
    }
}

void sBMP4Voice::startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
    midiNote = midiNoteNumber;
    pitchWheelPosition = currentPitchWheelPosition;

    adsr.setParameters (params);
    adsr.noteOn();

    updateOscFrequencies();

    processorChain.get<osc1Index>().setLevel (0.f/*velocity*/);
    processorChain.get<osc2Index>().setLevel (velocity);
}

void sBMP4Voice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
    DBG (pitchWheelPosition);
    updateOscFrequencies();
}

void sBMP4Voice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        //@TODO this should actually stop the voice, but it doesn't
        clearCurrentNote();
        adsr.reset();
    }
}
//NOW HERE
//    processSample can either return something in the range[-1, 0] or [0, 1], and I have to deal with it somehow.
//    the issue is that with square, we only want to go from 0 to 1, and for random we can go below.

void sBMP4Voice::processLfo()
{
    auto lfoOut = lfo.processSample (0.0f) * lfoVelocity;

    //@TODO when I implement setLfoDestination, I need to make sure we reset the lfoOsc1NoteOffset variables (and others) everytime the destination is changed
    LfoDest dest = LfoDest::filterCurOff;
    switch (dest)
    {
        case LfoDest::osc1Freq:
            lfoOsc1NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut / 2.f + .5f);
            updateOscFrequencies();
            break;

        case LfoDest::osc2Freq:
            lfoOsc2NoteOffset = lfoNoteRange.convertFrom0to1 ((lfoOut + 1) / 2);
            updateOscFrequencies();
            break;

        case LfoDest::filterCurOff:
        {
            auto curoffFreqHz = jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);
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

void sBMP4Voice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    auto output = tempBlock.getSubBlock (0, (size_t) numSamples);
    output.clear();

    for (size_t pos = 0; pos < numSamples;)
    {

        auto max = jmin (static_cast<size_t> (numSamples - pos), lfoUpdateCounter);
        auto block = output.getSubBlock (pos, max);

        dsp::ProcessContextReplacing<float> context (block);
        processorChain.process (context);

        pos += max;
        lfoUpdateCounter -= max;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            processLfo();
        }
    }

    dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (tempBlock);

    adsr.applyEnvelopeToBuffer (outputBuffer, startSample, numSamples);
}