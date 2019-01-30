/*
  ==============================================================================

    SineWaveVoice.h
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

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
    sBMP4Voice()
    {
        auto& masterGain = processorChain.get<masterGainIndex>();
        masterGain.setGainLinear (0.7f);

        auto& filter = processorChain.get<filterIndex>();
        filter.setCutoffFrequencyHz (1000.0f);
        filter.setResonance (0.7f);

        lfo.initialise ([](float x) { return std::sin(x); }, 128);
        lfo.setFrequency (3.0f);
    }

    void prepare (const dsp::ProcessSpec& spec)
    {
        tempBlock = dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
    }

    /**
        pitchWheelValue is a 14 bit number with a range of 0-16383. Since it's a bipolar control, it always starts at the center, 8192.
        On rev 2, max values are +/- 2 note. So 0 = -2 notes, 8192 = same note, 16383 = 2 notes
    */
    float getFreqAfterApplyingPitchWheel (int pitchWheelValue)
    {
        auto deltaNote = jmap ((float) pitchWheelValue, 0.f, 16383.f, -2.f, 2.f);

        return jmap (deltaNote, -2.f, 2.f, (float) MidiMessage::getMidiNoteInHertz (midiNote - 2),
                                           (float) MidiMessage::getMidiNoteInHertz (midiNote + 2));
    }

    void setOscTuning (int oscNum, float newValue)
    {
        auto freq = jmap (newValue, (float) MidiMessage::getMidiNoteInHertz (0), (float) MidiMessage::getMidiNoteInHertz (69));

        processorChain.get<osc1Index>().setFrequency (freq);
    }

    virtual void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
    {
        midiNote = midiNoteNumber;
        freqHz = getFreqAfterApplyingPitchWheel (currentPitchWheelPosition);

        processorChain.get<osc1Index>().setFrequency (freqHz, true);
        processorChain.get<osc1Index>().setLevel (velocity);

        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f, true);
        processorChain.get<osc2Index>().setLevel (velocity);
    }


    void pitchWheelMoved (int newPitchWheelValue) override
    {
        auto newFreq = freqHz = getFreqAfterApplyingPitchWheel (newPitchWheelValue);

        processorChain.get<osc1Index>().setFrequency (newFreq);
        processorChain.get<osc2Index>().setFrequency (newFreq * 1.01f);
    }

    void stopNote (float /*velocity*/, bool /*allowTailOff*/) override
    {
        //@TODO this should actually stop the voice, but it doesn't
        clearCurrentNote();
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
                auto lfoOut = lfo.processSample (0.0f);
                auto curoffFreqHz = jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);
                processorChain.get<filterIndex>().setCutoffFrequencyHz (curoffFreqHz);
            }
        }

        dsp::AudioBlock<float> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (tempBlock);
    }

private:
    HeapBlock<char> heapBlock;
    dsp::AudioBlock<float> tempBlock;

    enum
    {
        osc1Index,
        osc2Index,
        filterIndex,
        masterGainIndex
    };

    dsp::ProcessorChain<GainedOscillator<float>, GainedOscillator<float>, dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;

    float freqHz = 0.f;
    int midiNote = 0;
};
