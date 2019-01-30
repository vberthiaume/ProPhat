/*
  ==============================================================================

    SineWaveVoice.h
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

#if 0


//==============================================================================
struct SineWaveSound : public SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class SineWaveVoice : public SynthesiserVoice
{
public:
    SineWaveVoice() {}

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<SineWaveSound*> (sound) != nullptr; }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

protected:
    virtual forcedinline double getNextSample() noexcept
    {
        auto angle = currentAngle;
        currentAngle += delta;
        return std::sin (angle);
    }

    virtual void setFrequency (double frequency);

    double currentAngle = 0.0, level = 0.0, tailOff = 0.0;

    /**
        When using SineWaveVoice, delta is a deltaAngle used to increment the currentAngle used by the sin function.
        When using SineWaveTableVoice, delta is a table delta, used to increment the current, floating point index in the wave table.
    */
    double delta = 0.0;

    bool isPlaying = false;
};

//==============================================================================

class SineWaveTableVoice : public SineWaveVoice
{
public:
    SineWaveTableVoice (const AudioBuffer<double>& wavetableToUse) :
        wavetable (wavetableToUse),
        tableSize (wavetable.getNumSamples() - 1)
    {
        jassert (wavetable.getNumChannels() == 1);
    }

protected:

    forcedinline double getNextSample() noexcept override
    {
        auto* table = wavetable.getReadPointer (0);

        //get the rounded table indices and values surrounding the next value we want
        auto firstIndex = (unsigned int) currentIndex;
        auto secondIndex = firstIndex + 1;
        auto firstValue = table[firstIndex];
        auto secondValue = table[secondIndex];

        //calculate the fraction required to get from the first index to the actual, floating point index
        auto frac = currentIndex - firstIndex;
        //and do the interpolation
        auto currentSample = firstValue + frac * (secondValue - firstValue);

        //increase the index, going round the table if it overflows
        if ((currentIndex += delta) > tableSize)
            currentIndex -= tableSize;

        return currentSample;
    }

    virtual void setFrequency (double frequency) override
    {
        //the sample rate and the table size are used to calculate the table delta equivalent to the frequency
        auto tableSizeOverSampleRate = tableSize / getSampleRate();
        delta = frequency * tableSizeOverSampleRate;
    }

private:

    const AudioBuffer<double>& wavetable;
    const int tableSize;
    double currentIndex = 0.0f;
};
#endif

/*
  ==============================================================================

    voice.h
    Created: 29 Jan 2019 10:59:38am
    Author:  Haake

  ==============================================================================
*/

#pragma once

struct SineWaveSound : public SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};


//==============================================================================
template <typename Type>
class CustomOscillator
{
public:

    CustomOscillator()
    {
        auto& osc = processorChain.template get<oscIndex>();
        osc.initialise ([](Type x)
        {
            return jmap (x, Type (-MathConstants<double>::pi), Type (MathConstants<double>::pi), Type (-1), Type (1));
        }, 2);
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
class Voice : public SynthesiserVoice
{
public:
    Voice()
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

    virtual void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int /*currentPitchWheelPosition*/)
    {
        //@TODO use the currentPitchWheelPosition

        midiNote = midiNoteNumber;
        freqHz = (float) MidiMessage::getMidiNoteInHertz (midiNote);

        processorChain.get<osc1Index>().setFrequency (freqHz, true);
        processorChain.get<osc1Index>().setLevel (velocity);

        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f, true);
        processorChain.get<osc2Index>().setLevel (velocity);
    }

    /**
        newPitchWheelValue is a 14 bit number with a range of 0-16383. Since it's a bipolar control, it always starts at the center, 8192
    */
    void pitchWheelMoved (int newPitchWheelValue) override
    {
        //@TODO this works but the new picth doesn't make any sense
        //jmap (FloatType (i), FloatType (0), FloatType (numPoints - 1), minInputValueToUse, maxInputValueToUse))
        auto ratio = newPitchWheelValue / 16382.f + 1;
        processorChain.get<osc1Index>().setFrequency (freqHz * ratio);
        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f * ratio);
    }

    void stopNote (float /*velocity*/, bool /*allowTailOff*/) override
    {
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

    dsp::ProcessorChain<CustomOscillator<float>, CustomOscillator<float>, dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;

    float freqHz = 0.f;
    int midiNote = 0;
};
