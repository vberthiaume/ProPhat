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
        auto& osc = processorChain.template get<oscIndex>();

        //@TODO call this with different waveforms when the osc shape is changed?
        //also, if I wanted, I can NOT use wavetables by not supplying a second parameter here
#if 1
        osc.initialise ([](Type x)
        {
            //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
            return jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1));
        }, 2);
#else
        osc.initialise ([](Type x)
        {
            //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
            return sin (x);
        }, 128);
#endif
    }

    void setFrequency (Type newValue, bool force = false)
    {
        auto& osc = processorChain.template get<oscIndex>();
        osc.setFrequency (newValue, force);
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

        lfo.initialise ([](float x) { return x; /*std::sin(x);*/ }, 128);
        lfo.setFrequency (3.0f);
    }

    void prepare (const dsp::ProcessSpec& spec)
    {
        tempBlock = dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        adsr.setSampleRate (spec.sampleRate);
        ADSR::Parameters params;
        params.attack = 1.f;
        adsr.setParameters (params);

        lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
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

    void setOscShape (processorId /*oscNum*/, oscShape newShape)
    {
        switch (newShape)
        {
            case oscShape::saw:
                break;
            case oscShape::sawTri:
                break;
            case oscShape::triangle:
                break;
            case oscShape::pulse:
                break;
            case oscShape::total:
                break;
            default:
                break;
        }
    }

    virtual void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
    {
        midiNote = midiNoteNumber;
        pitchWheelPosition = currentPitchWheelPosition;

        //@TODO these need to be modified when we change the sliders
        //adsr.setParameters (sound->params);
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
                /*
                auto lfoOut = lfo.processSample (0.0f);
                auto curoffFreqHz = jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);
                processorChain.get<filterIndex>().setCutoffFrequencyHz (curoffFreqHz);
                */
            }
        }

        dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (tempBlock);

        adsr.applyEnvelopeToBuffer (outputBuffer, startSample, numSamples);
    }

private:
    HeapBlock<char> heapBlock;
    dsp::AudioBlock<float> tempBlock;
    dsp::ProcessorChain<GainedOscillator<float>, GainedOscillator<float>, dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    juce::ADSR adsr;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;

    int midiNote = 0;
    int pitchWheelPosition = 0;

    float osc1NoteOffset = 0.f;
    float osc2NoteOffset = 0.f;
};
