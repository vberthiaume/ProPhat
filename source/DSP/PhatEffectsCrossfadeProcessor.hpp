#pragma once

#include "../Utility/Helpers.h"
#include "../modules/DebugLog/Source/DebugLog.hpp"
#include <sstream>

#define LOG_EVERYTHING_DURING_CROSSFADE 0

constexpr auto crossfadeDurationSeconds = .1;

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
        smoothedGainL.reset (spec.sampleRate, static_cast<T> (crossfadeDurationSeconds));
        smoothedGainR.reset (spec.sampleRate, static_cast<T> (crossfadeDurationSeconds));

        gainLog.resize (spec.maximumBlockSize);
        prevDataLog.resize (spec.maximumBlockSize);
        nextDataLog.resize (spec.maximumBlockSize);
        outDataLog.resize (spec.maximumBlockSize);
    }

    void changeEffect (EffectType effect)
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
        curEffect  = effect;
    }

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGainL.isSmoothing())
            return EffectType::transitioning;

        return curEffect;
    }

#if ENABLE_GAIN_LOGGING
    void setDebugLogEntry (DebugLogEntry* _debugLogEntry)
    {
        debugLogEntry = _debugLogEntry;
    }
#endif

    void process (const juce::AudioBuffer<T>&                  previousEffectBuffer,
                  const juce::AudioBuffer<T>&                  nextEffectBuffer,
                  const juce::dsp::ProcessContextReplacing<T>& context)
    {
        const auto inputBlock { context.getInputBlock() };
        jassert (previousEffectBuffer.getNumChannels() >= nextEffectBuffer.getNumChannels()
                 && nextEffectBuffer.getNumChannels() >= inputBlock.getNumChannels());
        jassert (previousEffectBuffer.getNumSamples () >= nextEffectBuffer.getNumSamples ()
                 && nextEffectBuffer.getNumSamples () >= inputBlock.getNumSamples ());


#if LOG_EVERYTHING_DURING_CROSSFADE
        DBG ("prevData");
        for (int i = 0; i < inputBlock.getNumSamples (); ++i)
            DBG (previousEffectBuffer.getReadPointer (0)[i]);

        //DBG ("nextData");
        //for (int i = 0; i < inputBlock.getNumSamples (); ++i)
        //    DBG (nextEffectBuffer.getReadPointer (0)[i]);

        DBG ("context");
        for (int i = 0; i < inputBlock.getNumSamples (); ++i)
            DBG (context.getOutputBlock ().getChannelPointer (0)[i]);

        DBG ("done");
#endif

        const bool needToInverse = juce::approximatelyEqual (smoothedGainL.getTargetValue (), static_cast<T> (1));

        T curGain {};
        for (int channel = 0; channel < inputBlock.getNumChannels (); ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);
            auto*       outData  = context.getOutputBlock().getChannelPointer (channel);

            for (int sample = 0; sample < inputBlock.getNumSamples(); ++sample)
            {
                //TODO VB: this could be an IIFE to get curGain
                //figure out curGain value based on the current channel and whether we're running the smoothedGains in reverse
                T nextGain {};
                if (channel == 0)
                    nextGain = smoothedGainL.getNextValue ();
                else if (channel == 1)
                    nextGain = smoothedGainR.getNextValue ();
                else
                    jassertfalse;
                curGain = needToInverse ? (1 - nextGain) : nextGain;

                outData[sample] = prevData[sample] * curGain + nextData[sample] * (1 - curGain);

#if ENABLE_GAIN_LOGGING
                if (channel == 0 && sample == 0 && debugLogEntry)
                    debugLogEntry->firstGain = static_cast<float> (outData[sample]);

                gainLog[sample] = curGain;
                prevDataLog[sample] = prevData[sample];
                nextDataLog[sample] = nextData[sample];
                outDataLog[sample] = outData[sample];
#endif
            }
        }
    }

    EffectType prevEffect = EffectType::none;
    EffectType curEffect  = EffectType::verb;

  private:
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainL;
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainR;
#if ENABLE_GAIN_LOGGING
    DebugLogEntry* debugLogEntry { nullptr };
#endif

    //TODO: this needs to be stored and retrieved from the state
    //EffectType curEffect = EffectType::verb;

    std::vector<T> gainLog {};
    std::vector<T> prevDataLog {};
    std::vector<T> nextDataLog {};
    std::vector<T> outDataLog {};
};
