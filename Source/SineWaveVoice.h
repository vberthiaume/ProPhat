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

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;

protected:
    virtual forcedinline float getNextSample() noexcept { return (float) std::sin (currentAngle); }

    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};

//==============================================================================

class SineWaveTableVoice : public SineWaveVoice
{
public:
    SineWaveTableVoice (const AudioSampleBuffer& wavetableToUse) :
        wavetable (wavetableToUse),
        tableSize (wavetable.getNumSamples() - 1)
    {
        jassert (wavetable.getNumChannels() == 1);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;

protected:

    forcedinline float getNextSample() noexcept override
    {
        auto* table = wavetable.getReadPointer (0);

        //get the rounded table indices and values surrounding the next value we want
        auto firstIndex = (unsigned int) currentIndex;
        auto secondIndex = firstIndex + 1;
        auto firstValue = table[firstIndex];
        auto secondValue = table[secondIndex];

        //calculate the fraction required to get from the first index to the actual, float index
        auto frac = currentIndex - (float) firstIndex;
        //and do the interpolation
        auto currentSample = firstValue + frac * (secondValue - firstValue);

        //increase the index, going round the table if it overflows
        if ((currentIndex += tableDelta) > tableSize)
            currentIndex -= tableSize;

        return currentSample;
    }

private:
    void setFrequency (float frequency);

    const AudioSampleBuffer& wavetable;
    const int tableSize;
    float currentIndex = 0.0f, tableDelta = 0.0f;
};
