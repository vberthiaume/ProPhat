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
#include <mutex>

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
        setLevel (defaultOscLevel);
    }

    void setFrequency (Type newValue, bool force = false)
    {
        jassert (newValue > 0);

        auto& osc = processorChain.template get<oscIndex>();
        osc.setFrequency (newValue, force);
    }

    void setOscShape (int newShape)
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

    void setLevel (Type newValue)
    {
        if (!isActive)
            newValue = 0;
        else
            lastActiveLevel = newValue;

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
        std::lock_guard<std::mutex> lock (processMutex);

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

    std::mutex processMutex;

    bool isActive = true;

    Type lastActiveLevel{};

    dsp::ProcessorChain<dsp::Oscillator<Type>, dsp::Gain<Type>> processorChain;
};

//==============================================================================
using namespace Constants;
class sBMP4Voice : public SynthesiserVoice
{
public:

    enum processorId
    {
        filterIndex,
        masterGainIndex
    };

    sBMP4Voice (int voiceId);

    void prepare (const dsp::ProcessSpec& spec);

    void updateOscFrequencies();

    void setOsc1Tuning (int newMidiNote)
    {
        osc1NoteOffset = middleCMidiNote - (float) newMidiNote;
        updateOscFrequencies();
    }

    void setOsc2Tuning (int newMidiNote)
    {
        osc2NoteOffset = middleCMidiNote - (float) newMidiNote;
        updateOscFrequencies();
    }

    void setOsc1Shape (int newShape)
    {
        osc1.setOscShape (newShape);
    }

    void setOsc2Shape (int newShape)
    {
        osc2.setOscShape (newShape);
    }

    void setAmpParam (StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }
    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (float newValue)
    {
        curFilterCutoff = newValue;
        processorChain.get<filterIndex>().setCutoffFrequencyHz (curFilterCutoff);
    }

    void setFilterResonance (float newAmount)
    {
        curFilterResonance = newAmount;
        processorChain.get<filterIndex>().setResonance (curFilterResonance);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int currentPitchWheelPosition);

    void pitchWheelMoved (int newPitchWheelValue) override;

    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<SineWaveSound*> (sound) != nullptr; }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) {}

#if RAMP_ADSR
    void updateNextParams();
#endif

private:

    void updateLfo();
    void processEnvelope (dsp::AudioBlock<float> block2);

    int voiceId;

    HeapBlock<char> heapBlock1, heapBlock2;
    dsp::AudioBlock<float> osc1Block, osc2Block;
    GainedOscillator<float> osc1, osc2;

    dsp::ProcessorChain<dsp::LadderFilter<float>, dsp::Gain<float>> processorChain;

    ADSR adsr;
    ADSR::Parameters curParams;
#if RAMP_ADSR
    float nextAttack = defaultAmpA;
    float nextDecay = defaultAmpD;
    float nextSustain = defaultAmpS;
    float nextRelease = defaultAmpR;
#endif
    bool stopNoteRequested = false;

    float curFilterCutoff = defaultFilterCutoff;
    float curFilterResonance = defaultFilterResonance;

    //lfo stuff
    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    dsp::Oscillator<float> lfo;
    std::mutex lfoMutex;
    float lfoAmount = defaultLfoAmount;
    LfoDest lfoDest;
    float lfoOsc1NoteOffset = 0.f;
    float lfoOsc2NoteOffset = 0.f;

    //for the random lfo
    Random rng;
    float randomValue = 0.f;
    bool valueWasBig = false;

    int pitchWheelPosition = 0;

    float osc1NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;
    float osc2NoteOffset = (float) middleCMidiNote - defaultOscMidiNote;
};
