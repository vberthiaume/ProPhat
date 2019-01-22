/*
  ==============================================================================

    SineWaveVoice.h
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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
