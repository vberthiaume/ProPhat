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
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};

//==============================================================================

class SineWaveTableVoice : public SineWaveVoice
{
public:
    SineWaveTableVoice (const AudioSampleBuffer& wavetableToUse)
        : wavetable (wavetableToUse),
        tableSize (wavetable.getNumSamples() - 1)
    {
        jassert (wavetable.getNumChannels() == 1);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;

    void setFrequency (float frequency);

    forcedinline float getNextSample() noexcept;

private:
    const AudioSampleBuffer& wavetable;
    const int tableSize;
    float currentIndex = 0.0f, tableDelta = 0.0f;
};


