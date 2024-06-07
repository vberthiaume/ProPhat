#include "ProPhatVoice.h"

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate (outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<double>&, int, int)
{
    //trying to render doubles with a float voice!
    jassertfalse;
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate(outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<float>&, int, int)
{
    //trying to render floats with a double voice!
    jassertfalse;
}

template class ProPhatVoice<float>;
template class ProPhatVoice<double>;
