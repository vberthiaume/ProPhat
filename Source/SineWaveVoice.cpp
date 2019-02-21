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
    processorChain.get<masterGainIndex>().setGainLinear (0.6f);
    processorChain.get<filterIndex>().setCutoffFrequencyHz (1000.0f);
    processorChain.get<filterIndex>().setResonance (0.7f);

    processorChain2.get<masterGainIndex>().setGainLinear (0.6f);
    processorChain2.get<filterIndex>().setCutoffFrequencyHz (1000.0f);
    processorChain2.get<filterIndex>().setResonance (0.7f);

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (3.0f);
}

#if RAMP_ADSR
void sBMP4Voice::updateNextParams()
{
    nextAttack  = curParams.attack;
    nextDecay   = curParams.decay;
    nextSustain = curParams.sustain;
    nextRelease = curParams.release;
}
#endif

void sBMP4Voice::prepare (const dsp::ProcessSpec& spec)
{
    tempBlock = dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
    processorChain.prepare (spec);

    tempBlock2 = dsp::AudioBlock<float> (heapBlock2, spec.numChannels, spec.maximumBlockSize);
    processorChain2.prepare (spec);

    adsr.setSampleRate (spec.sampleRate);
#if RAMP_ADSR
    updateNextParams();
#endif
    adsr.setParameters (curParams);

    lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});

    isPrepared = true;
}

void sBMP4Voice::updateOscFrequencies()
{
    auto pitchWheelDeltaNote = jmap ((float) pitchWheelPosition, 0.f, 16383.f, -2.f, 2.f);

    auto osc1Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc1NoteOffset + lfoOsc1NoteOffset + pitchWheelDeltaNote);
    processorChain.get<osc1Index>().setFrequency ((float) osc1Freq, true);

    auto osc2Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc2NoteOffset + lfoOsc2NoteOffset + pitchWheelDeltaNote);
    processorChain2.get<osc1Index>().setFrequency ((float) osc2Freq, true);
}

void sBMP4Voice::setAmpParam (StringRef parameterID, float newValue)
{
    jassert (newValue > 0);

#if RAMP_ADSR

    if (parameterID == sBMP4AudioProcessorIDs::ampAttackID)
        nextAttack = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampDecayID)
        nextDecay = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampSustainID)
        nextSustain = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
        nextRelease = newValue;

    if (! adsr.isActive())
    {
        curParams.attack = nextAttack;
        curParams.decay = nextDecay;
        curParams.sustain = nextSustain;
        curParams.release = nextRelease;

        adsr.setParameters (curParams);
}
#else

    if (parameterID == sBMP4AudioProcessorIDs::ampAttackID)
        curParams.attack = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampDecayID)
        curParams.decay = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampSustainID)
        curParams.sustain = newValue;
    else if (parameterID == sBMP4AudioProcessorIDs::ampReleaseID)
        curParams.release = newValue;

    adsr.setParameters (curParams);

#endif
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (an only that one) should oscilate between [-1, 1]
void sBMP4Voice::setLfoShape (LfoShape shape)
{
    switch (shape)
    {
        case LfoShape::triangle:
            lfo.initialise ([](float x) { return (std::sin (x) + 1) / 2; }, 128);
            break;

        case LfoShape::saw:
            lfo.initialise ([](float x)
            {
                //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 0.f, 1.f);
            }, 2);
            break;

        //case LfoShape::revSaw:
        //    lfo.initialise ([](float x)
        //    {
        //        return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 1.f, 0.f);
        //    }, 2);
        //    break;

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
            break;

        case LfoShape::none:
        case LfoShape::total:
        default:
            jassertfalse;
            break;
    }
}

void sBMP4Voice::setLfoDest (LfoDest dest)
{
    //reset everything
    lfoOsc1NoteOffset = 0.f;
    lfoOsc2NoteOffset = 0.f;
    processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    processorChain.get<filterIndex>().setResonance (curFilterResonance);

    processorChain2.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    processorChain2.get<filterIndex>().setResonance (curFilterResonance);

    //change the destination
    lfoDest = dest;
}

void sBMP4Voice::startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
    midiNote = midiNoteNumber;
    pitchWheelPosition = currentPitchWheelPosition;

    adsrWasActive = true;
#if RAMP_ADSR
    updateNextParams();
#endif
    adsr.setParameters (curParams);
    adsr.noteOn();

    updateOscFrequencies();

    //@TODO the velocity here should probably be dependent on the number of voices... or add compression or something?
    processorChain.get<osc1Index>().setLevel (velocity / (numVoices / 2));
    //processorChain.get<osc1Index>().setLevel (0.f);

    processorChain2.get<osc1Index>().setLevel (velocity / (numVoices / 2));
    //processorChain.get<osc2Index>().setLevel (0.f);

    //processorChain.setBypassed<osc2Index> (true);

    //processorChain.get<masterGainIndex>().setGainLinear (velocity / 4);
}

void sBMP4Voice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
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
        clearCurrentNote();
        adsr.reset();
#if RAMP_ADSR
        updateNextParams();
#endif
    }
}

#if RAMP_ADSR
void sBMP4Voice::updateAdsr()
{
    const auto threshold = .03f;

    auto updateParam = [threshold] (float& curParam, float& nextParam )
    {
        if (curParam > nextParam + threshold)
            curParam -= threshold;
        else if (curParam < nextParam - threshold)
            curParam += threshold;
        else
            curParam = nextParam;
    };


    updateParam (curParams.attack, nextAttack);
    updateParam (curParams.decay, nextDecay);
    updateParam (curParams.sustain, nextSustain);
    updateParam (curParams.release, nextRelease);

    adsr.setParameters (curParams);
}
#endif

void sBMP4Voice::updateLfo()
{
    //@TODO For now, all lfos oscillate between [0, 1], even though the random one (an only that one) should oscilate between [-1, 1]
    auto lfoOut = lfo.processSample (0.0f) * lfoAmount;

    switch (lfoDest)
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
            processorChain2.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff + curoffFreqHz);
        }
        break;

        case LfoDest::filterResonance:
            processorChain.get<filterIndex>().setResonance (lfoOut);
            processorChain2.get<filterIndex>().setResonance (lfoOut);
            break;

        default:
            break;
    }
}

void sBMP4Voice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isKeyDown() && !adsr.isActive())
    {
#if RAMP_ADSR
        if (adsrWasActive)
        {
            updateNextParams();
            adsrWasActive = false;
        }
#endif
        return;
    }

    auto output = tempBlock.getSubBlock (0, (size_t) numSamples);
    output.clear();

    for (size_t pos = 0; pos < numSamples;)
    {
        auto max = jmin (static_cast<size_t> (numSamples - pos), lfoUpdateCounter);
        auto block = output.getSubBlock (pos, max);

        dsp::ProcessContextReplacing<float> context (block);
        processorChain.process (context);
        processorChain2.process (context);

        pos += max;
        lfoUpdateCounter -= max;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
#if RAMP_ADSR
            updateAdsr();
#endif
        }
    }

    dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (tempBlock);

    adsr.applyEnvelopeToBuffer (outputBuffer, startSample, numSamples);
}