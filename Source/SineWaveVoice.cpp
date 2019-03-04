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


sBMP4Voice::sBMP4Voice (int vId) :
voiceId (vId)
{
    processorChain.get<masterGainIndex>().setGainLinear (defaultOscLevel);
    processorChain.get<filterIndex>().setCutoffFrequencyHz (defaultFilterCutoff);
    processorChain.get<filterIndex>().setResonance (defaultFilterResonance);

    sub.setOscShape (OscShape::pulse);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (defaultLfoFreq);
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
    osc1Block = dsp::AudioBlock<float> (heapBlock1, spec.numChannels, spec.maximumBlockSize);
    osc2Block = dsp::AudioBlock<float> (heapBlock2, spec.numChannels, spec.maximumBlockSize);

    sub.prepare (spec);
    osc1.prepare (spec);
    osc2.prepare (spec);
    processorChain.prepare (spec);

    adsr.setSampleRate (spec.sampleRate);
#if RAMP_ADSR
    updateNextParams();
#endif
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

        //case LfoShape::revSaw:
        //{
        //    std::lock_guard<std::mutex> lock (lfoMutex);
        //    lfo.initialise ([](float x)
        //    {
        //        return (float) jmap (x, -MathConstants<float>::pi, MathConstants<float>::pi, 1.f, 0.f);
        //    }, 2);
        //}
        //    break;

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

void sBMP4Voice::startNote (int /*midiNoteNumber*/, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
    pitchWheelPosition = currentPitchWheelPosition;
    adsr.noteOn();
    updateOscFrequencies();

    curVelocity = velocity;

    updateOscLevels();
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
        if (stopNoteRequested)
            return;

        adsr.noteOff();
        stopNoteRequested = true;
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
        stopNoteRequested = false;
    }
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

void sBMP4Voice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    auto osc1Output = osc1Block.getSubBlock (0, (size_t) numSamples);
    osc1Output.clear();

    auto osc2Output = osc2Block.getSubBlock (0, (size_t) numSamples);
    osc2Output.clear();

    for (size_t pos = 0; pos < numSamples;)
    {
        auto max = jmin (static_cast<size_t> (numSamples - pos), lfoUpdateCounter);

        //process osc1
        auto block1 = osc1Output.getSubBlock (pos, max);
        dsp::ProcessContextReplacing<float> osc1Context (block1);
        sub.process (osc1Context);
        osc1.process (osc1Context);

        //process osc2
        auto block2 = osc2Output.getSubBlock (pos, max);
        dsp::ProcessContextReplacing<float> osc2Context (block2);
        osc2.process (osc2Context);

        //process the sum of osc1 and osc2
        block2.add (block1);
        dsp::ProcessContextReplacing<float> summedContext (block2);
        processorChain.process (summedContext);

        processEnvelope (block2);

        pos += max;
        lfoUpdateCounter -= max;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
        }
    }

    dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (osc2Block);
}

void sBMP4Voice::processEnvelope (dsp::AudioBlock<float> block)
{
    auto samples = block.getNumSamples();
    auto numChannels = block.getNumChannels();

    for (auto start = 0; start < samples; ++start)
    {
        auto env = adsr.getNextSample();

        for (int i = 0; i < numChannels; ++i)
            block.getChannelPointer (i)[start] *= env;
    }

    if (stopNoteRequested && !adsr.isActive())
        stopNote (0.f, false);
}
