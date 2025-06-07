#pragma once

#include "../Utility/Helpers.h"
#include "../modules/DebugLog/Source/DebugLog.hpp"
#include <sstream>

enum class EffectType
{
    none = 0,
    verb,
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
        jassert (spec.numChannels == 2);
        smoothedGainL.reset (spec.sampleRate, .1);
        smoothedGainR.reset (spec.sampleRate, .1);

        gains.resize (spec.maximumBlockSize);
    }

    void changeEffect(EffectType effect)
    {
        //first reverse the smoothedGain. If we're not at one of the extremes, we got a double click which we'll ignore
        jassert (juce::approximatelyEqual (smoothedGainL.getTargetValue(), static_cast<T> (1))
                 || juce::approximatelyEqual (smoothedGainL.getTargetValue(), static_cast<T> (0)));
        if (juce::approximatelyEqual (smoothedGainL.getTargetValue(), static_cast<T> (1)))
        {
            smoothedGainL.setTargetValue (0.);
            smoothedGainR.setTargetValue (0.);
        }
        else
        {
            smoothedGainL.setTargetValue (1.);
            smoothedGainR.setTargetValue (1.);
        }

        prevEffect = curEffect;
        curEffect = effect;
        
    }

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGainL.isSmoothing())
            return EffectType::transitioning;

        return curEffect;
    }

#if ENABLE_GAIN_LOGGING
    void setDebugLogEntry (DebugLogEntry *_debugLogEntry)
    {
        debugLogEntry = _debugLogEntry;
    }
#endif

    void process (const juce::AudioBuffer<T>& previousEffectBuffer,
                  const juce::AudioBuffer<T>& nextEffectBuffer,
                  juce::AudioBuffer<T>&       outputBuffer)
    {
        jassert (previousEffectBuffer.getNumChannels() == nextEffectBuffer.getNumChannels() && nextEffectBuffer.getNumChannels() == outputBuffer.getNumChannels());
        jassert (previousEffectBuffer.getNumSamples() == nextEffectBuffer.getNumSamples() && nextEffectBuffer.getNumSamples() >= outputBuffer.getNumSamples());

        const auto numChannels      = outputBuffer.getNumChannels();
        const auto numSamples       = outputBuffer.getNumSamples();
        const bool needToInverse = juce::approximatelyEqual (smoothedGainL.getTargetValue(), static_cast<T> (1));

        T nextGain{};
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);
            auto*       outData  = outputBuffer.getWritePointer (channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                if (channel == 0)
                    nextGain = smoothedGainL.getNextValue();
                else if (channel == 1)
                    nextGain = smoothedGainR.getNextValue();
                else
                    jassertfalse;

#if ENABLE_GAIN_LOGGING
//                gains[sample] = nextGain;
                if (channel == 0 && sample == 0 && debugLogEntry)
                    debugLogEntry->firstGain = nextGain;
#endif

                const auto gain = needToInverse ? (1 - nextGain) : nextGain;
                outData[sample] = prevData[sample] * gain + nextData[sample] * (1 - gain);
            }
#if ENABLE_GAIN_LOGGING
            if (channel == 0 && debugLogEntry)
            {
                debugLogEntry->lastGain = nextGain;
//                DBG ("NEW BLOCK");
//                for (int i = 0; i < gains.size(); ++i)
//                    DBG (gains[i]);
            }
#endif
        }
    }

    EffectType prevEffect = EffectType::none;
    EffectType curEffect = EffectType::verb;

  private:
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainL;
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainR;
#if ENABLE_GAIN_LOGGING
    DebugLogEntry* debugLogEntry{nullptr};
#endif

    //TODO: this needs to be stored and retrieved from the state
    //EffectType curEffect = EffectType::verb;

    std::vector<float> gains{};
};
