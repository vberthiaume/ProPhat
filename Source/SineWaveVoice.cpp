/*
  ==============================================================================

    SineWaveVoice.cpp
    Created: 18 Jan 2019 4:37:09pm
    Author:  Haake

  ==============================================================================
*/

#include "SineWaveVoice.h"

#if 0
void SineWaveVoice::startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
    level = velocity * 0.15;
    tailOff = 0.0;

    setFrequency ((float) MidiMessage::getMidiNoteInHertz (midiNoteNumber));

    isPlaying = true;
}

void SineWaveVoice::setFrequency (double frequency)
{
    currentAngle = 0.0;
    auto cyclesPerSample = frequency / getSampleRate();
    delta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
}

void SineWaveVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (tailOff == 0.0)
            tailOff = 1.0;
    }
    else
    {
        clearCurrentNote();
        isPlaying = false;
    }
}

void SineWaveVoice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isPlaying)
        return;

    while (--numSamples >= 0)
    {
        auto curTailOff = tailOff > 0.0 ? tailOff : 1;

        auto currentSample = (float) (getNextSample() * level * curTailOff);
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
            outputBuffer.addSample (i, startSample, currentSample);

        ++startSample;

        if (tailOff > 0.0)
        {
            tailOff *= 0.99;

            if (tailOff <= 0.005)
            {
                clearCurrentNote();
                isPlaying = false;
                break;
            }
        }
    }
}

#endif
