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
        jassert (newValue > 0);

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

    sBMP4Voice();

    void prepare (const dsp::ProcessSpec& spec);

    bool isActive()
    {
        return isKeyDown() || adsr.isActive();
    }

    void updateOscFrequencies();

    void setOscTuning (processorId oscNum, int newMidiNote);

    void setOscShape (processorId oscNum, OscShape newShape);

    void setAmpParam (StringRef parameterID, float newValue);

    void setLfoShape (LfoShape shape);

    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }

    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (float newValue)
    {
        curFilterCutoff = newValue;
        processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    }

    void setFilterResonance (float newAmount)
    {
        processorChain.get<filterIndex>().setResonance (newAmount);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition);

    void pitchWheelMoved (int newPitchWheelValue) override;

    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<SineWaveSound*> (sound) != nullptr; }

    void processLfo();

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) {}

private:
    bool isPrepared = false;

    HeapBlock<char> heapBlock;
    dsp::AudioBlock<float> tempBlock;
    dsp::ProcessorChain<GainedOscillator<float>, GainedOscillator<float>, dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    juce::ADSR adsr;
    ADSR::Parameters params;

    float curFilterCutoff = 0.f;

    //lfo stuff
    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;
    float lfoAmount = 0.f;
    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    //for the random lfo
    Random rng;
    float randomValue = 0.f;
    bool valueWasBig = false;

    int midiNote = 0;
    int pitchWheelPosition = 0;

    float osc1NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;
    float osc2NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;
};
