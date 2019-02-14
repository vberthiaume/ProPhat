/*
  ==============================================================================

    SineWaveVoice.h
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Helpers.h"

struct SineWaveSound : public SynthesiserSound
{
    SineWaveSound() {}

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
    }

    void setFrequency (Type newValue, bool force = false)
    {
        auto& osc = processorChain.template get<oscIndex>();
        osc.setFrequency (newValue, force);
    }

    void setOscShape (OscShape newShape)
    {
        auto& osc = processorChain.template get<oscIndex>();

        switch (newShape)
        {
            case OscShape::saw:
                osc.initialise ([](Type x)
                {
                    //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                    return jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1));
                }, 2);
                break;

            case OscShape::sawTri:

                osc.initialise ([](Type x)
                {
                    Type y = jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1)) / 2;

                    if (x < 0)
                        return y += jmap (x, Type (-MathConstants<double>::pi), Type (0), Type (-1), Type (1)) / 2;
                    else
                        return y += jmap (x, Type (0), Type (MathConstants<double>::pi), Type (1), Type (-1)) / 2;

                }, 128);

                break;

            case OscShape::triangle:
                osc.initialise ([](Type x)
                {
                    if (x < 0)
                        return jmap (x, Type (-MathConstants<double>::pi), Type (0), Type (-1), Type (1));
                    else
                        return jmap (x, Type (0), Type (MathConstants<double>::pi), Type (1), Type (-1));

                }, 128);
                break;

            case OscShape::pulse:
                osc.initialise ([](Type x)
                {
                    if (x < 0)
                        return Type (-1);
                    else
                        return Type (1);
                }, 128);
                break;

            case OscShape::none:
            case OscShape::total:
                jassertfalse;
                break;
            default:
                break;
        }
    }

    void setLevel (Type newValue)
    {
        auto& gain = processorChain.template get<gainIndex>();
        gain.setGainLinear (newValue);
    }

    void reset() noexcept
    {
        processorChain.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
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

    dsp::ProcessorChain<dsp::Oscillator<Type>, dsp::Gain<Type>> processorChain;
};

//==============================================================================
class sBMP4Voice : public SynthesiserVoice
{
public:

    enum processorId
    {
        osc1Index,
        osc2Index,
        filterIndex,
        masterGainIndex
    };

    sBMP4Voice()
    {
        auto& masterGain = processorChain.get<masterGainIndex>();
        masterGain.setGainLinear (0.7f);

        auto& filter = processorChain.get<filterIndex>();
        filter.setCutoffFrequencyHz (1000.0f);
        filter.setResonance (0.7f);

        setLfoShape (LfoShape::triangle);
        lfo.setFrequency (3.0f);
    }

    void prepare (const dsp::ProcessSpec& spec)
    {
        tempBlock = dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        adsr.setSampleRate (spec.sampleRate);
        adsr.setParameters (params);

        lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});

        isPrepared = true;
    }

    bool isActive()
    {
        return isKeyDown() || adsr.isActive();
    }

    void updateOscFrequencies()
    {
        auto pitchWheelDeltaNote = jmap ((float) pitchWheelPosition, 0.f, 16383.f, -2.f, 2.f);

        auto osc1Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc1NoteOffset + pitchWheelDeltaNote);
        processorChain.get<osc1Index>().setFrequency ((float) osc1Freq, true);

        auto osc2Freq = Helpers::getDoubleMidiNoteInHertz (midiNote - osc2NoteOffset + pitchWheelDeltaNote);
        processorChain.get<osc2Index>().setFrequency ((float) osc2Freq, true);
    }

    /**
        So here, we have to transform newMidiNote into a delta.
        The frequency to use for oscilators will be whatever
    */
    void setOscTuning (processorId oscNum, int newMidiNote)
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

    void setOscShape (processorId oscNum, OscShape newShape)
    {
        if (oscNum == osc1Index)
            processorChain.get<osc1Index>().setOscShape (newShape);
        else if (oscNum == osc2Index)
            processorChain.get<osc2Index>().setOscShape (newShape);
    }

    void setAmpParam (StringRef parameterID, float newValue)
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

    void setLfoShape (LfoShape shape)
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
                        return -1.f;
                    else
                        return 1.f;
                }, 128);
                break;

            case LfoShape::random:
                lfo.initialise ([](float x)
                {
                    if (x < 0.f)
                        return -1.f;
                    else
                        return 1.f;
                }, 128);
                break;

            case LfoShape::none:
            case LfoShape::total:
            default:
                jassertfalse;
                break;
        }
    }

    void setLfoFreq (float newFreq)
    {
        lfo.setFrequency (newFreq);
    }

    void setLfoAmount (float newAmount)
    {
        lfoVelocity = newAmount;
    }

    void setFilterCutoff (float newAmount)
    {
        processorChain.get<filterIndex>().setCutoffFrequencyHz (newAmount);
    }

    void setFilterResonance (float newAmount)
    {
        processorChain.get<filterIndex>().setResonance (newAmount);
    }

    virtual void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
    {
        midiNote = midiNoteNumber;
        pitchWheelPosition = currentPitchWheelPosition;

        adsr.setParameters (params);
        adsr.noteOn();

        updateOscFrequencies();

        processorChain.get<osc1Index>().setLevel (velocity);
        processorChain.get<osc2Index>().setLevel (velocity);
    }

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        pitchWheelPosition = newPitchWheelValue;
        updateOscFrequencies();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
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

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void controllerMoved (int, int) override {}

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
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

                auto lfoOut = lfo.processSample (0.0f) * lfoVelocity;

                //auto curoffFreqHz = jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);
                //processorChain.get<filterIndex>().setCutoffFrequencyHz (curoffFreqHz);
            }
        }

        dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (tempBlock);

        adsr.applyEnvelopeToBuffer (outputBuffer, startSample, numSamples);
    }

private:
    bool isPrepared = false;

    HeapBlock<char> heapBlock;
    dsp::AudioBlock<float> tempBlock;
    dsp::ProcessorChain<GainedOscillator<float>, GainedOscillator<float>, dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    juce::ADSR adsr;
    ADSR::Parameters params;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;
    float lfoVelocity = 1.0;

    int midiNote = 0;
    int pitchWheelPosition = 0;

    float osc1NoteOffset = 0.f;
    float osc2NoteOffset = 0.f;
};
