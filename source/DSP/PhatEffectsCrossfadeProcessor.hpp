#include "../Utility/Helpers.h"

//NOW HERE -- TOGGLEING THIS TO 1 MAKES THE EFFECT CROSSOVER GLITCH GO AWAY
#define USE_ONLY_2_EFFECTS 0

enum class EffectType
{
    verb = 0,
    chorus,
    phaser,
    transitioning
};

//==============================================================================

template <typename ProcessorType, std::floating_point T>
struct EffectProcessorWrapper
{
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        processor.prepare (spec);
    }

    void process (const juce::dsp::ProcessContextReplacing<T>& context)
    {
        processor.process (context);
    }

    void reset()
    {
        processor.reset();
    }

    ProcessorType processor;
};

template <std::floating_point T>
class EffectsCrossfadeProcessor
{
public:
    EffectsCrossfadeProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        smoothedGain.reset (spec.sampleRate, .1);
    }

    void changeEffect()
    {
        //first reverse the smoothedGain. If we're not at one of the extremes, we got a double click which we'll ignore
        jassert (juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1))
                 || juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (0)));
        if (juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1)))
            smoothedGain.setTargetValue (0.);
        else
            smoothedGain.setTargetValue (1.);

        if (curEffect == EffectType::verb)
            curEffect = EffectType::chorus;
        else if (curEffect == EffectType::chorus)
#if USE_ONLY_2_EFFECTS
            curEffect = EffectType::verb;
#else
            curEffect = EffectType::phaser;
        else if (curEffect == EffectType::phaser)
            curEffect = EffectType::verb;
#endif
        else
            jassertfalse;
    }

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGain.isSmoothing())
            return EffectType::transitioning;

        return curEffect;
    }

    void process (const juce::AudioBuffer<T>& previousEffectBuffer,
                  const juce::AudioBuffer<T>& nextEffectBuffer,
                  juce::AudioBuffer<T>&       outputBuffer)
    {
        jassert (previousEffectBuffer.getNumChannels() == nextEffectBuffer.getNumChannels() && nextEffectBuffer.getNumChannels() == outputBuffer.getNumChannels());
        jassert (previousEffectBuffer.getNumSamples() == nextEffectBuffer.getNumSamples() && nextEffectBuffer.getNumSamples() == outputBuffer.getNumSamples());

        const auto channels = outputBuffer.getNumChannels();
        const auto samples  = outputBuffer.getNumSamples();

        const auto needToInverse {juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1))};
        if (needToInverse)
        {
            for (int channel = 0; channel < channels; ++channel)
            {
                for (int sample = 0; sample < samples; ++sample)
                {
                    // Get individual samples
                    const auto prev = previousEffectBuffer.getSample (channel, sample);
                    const auto next = nextEffectBuffer.getSample (channel, sample);

                    // Mix and send to output
                    const auto gain   = 1 - smoothedGain.getNextValue();
                    const auto output = static_cast<T> (prev * gain + next * (1 - gain));
                    outputBuffer.setSample (channel, sample, static_cast<T> (output));
                }
            }
        }
        else
        {
            for (int channel = 0; channel < channels; ++channel)
            {
                for (int sample = 0; sample < samples; ++sample)
                {
                    // Get individual samples
                    const auto prev = previousEffectBuffer.getSample (channel, sample);
                    const auto next = nextEffectBuffer.getSample (channel, sample);

                    // Mix and send to output
                    const auto gain   = smoothedGain.getNextValue();
                    const auto output = static_cast<T> (prev * gain + next * (1 - gain));
                    outputBuffer.setSample (channel, sample, static_cast<T> (output));
                }
            }
        }
    }

    EffectType curEffect = EffectType::verb;

  private:
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGain;

    //TODO: this needs to be stored and retrieved from the state
    //EffectType curEffect = EffectType::verb;
};
