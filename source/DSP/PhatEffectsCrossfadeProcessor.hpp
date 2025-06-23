#pragma once

#include "../Utility/Helpers.h"
#include "../modules/DebugLog/Source/DebugLog.hpp"
#include <sstream>

//changing this does NOT affect the perception of glitches at note-offs
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
        //changing this creates a long tail but still an initial glitch
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

#if EFFECTS_PROCESSOR_PER_VOICE
    void process (const juce::AudioBuffer<T>& previousEffectBuffer,
                  const juce::AudioBuffer<T>& nextEffectBuffer,
                  juce::dsp::AudioBlock<T>    outputBlock)
    {
        jassert (previousEffectBuffer.getNumChannels() == nextEffectBuffer.getNumChannels() && nextEffectBuffer.getNumChannels() == outputBlock.getNumChannels());
        //TODO VB: should probably assert that all buffers have at least numSamples?

        const auto numChannels   = outputBlock.getNumChannels();
        const bool needToInverse = juce::approximatelyEqual (smoothedGainL.getTargetValue(), static_cast<T> (1));

        T curGain {};
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);

            for (int sample = 0; sample < outputBlock.getNumSamples(); ++sample)
            {
                //TODO VB: this could be an IIFE to get curGain
                //figure out curGain value based on the current channel and whether we're running the smoothedGains in reverse
                T nextGain {};
                if (channel == 0)
                    nextGain = smoothedGainL.getNextValue();
                else if (channel == 1)
                    nextGain = smoothedGainR.getNextValue();
                else
                    jassertfalse;
                curGain = needToInverse ? (1 - nextGain) : nextGain;

                //cross fade prevData and nextData into outData. I guess any of these can be clipping
                outputBlock.setSample (channel, sample, prevData[sample] * curGain + nextData[sample] * (1 - curGain));
            }
        }
    }
#else
    void process (const juce::AudioBuffer<T>& previousEffectBuffer,
                  const juce::AudioBuffer<T>& nextEffectBuffer,
                  juce::AudioBuffer<T>&       outputBuffer,
                  int                         startSample,
                  int                         numSamples)
    {
        jassert (previousEffectBuffer.getNumChannels () == nextEffectBuffer.getNumChannels () && nextEffectBuffer.getNumChannels () == outputBuffer.getNumChannels ());
        //TODO VB: should probably assert that all buffers have at least numSamples?

        const auto numChannels = outputBuffer.getNumChannels ();
        const bool needToInverse = juce::approximatelyEqual (smoothedGainL.getTargetValue (), static_cast<T> (1));

        T curGain {};
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);
            auto* outData = outputBuffer.getWritePointer (channel);

            for (int sample = 0; sample < numSamples; ++sample)
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

                //cross fade prevData and nextData into outData. I guess any of these can be clipping
                outData[startSample + sample] = prevData[sample] * curGain + nextData[sample] * (1 - curGain);

#if ENABLE_GAIN_LOGGING
                if (channel == 0 && sample == 0 && debugLogEntry)
                    debugLogEntry->firstGain = static_cast<float> (outData[sample]);

                gainLog[sample] = curGain;
                prevDataLog[sample] = prevData[sample];
                nextDataLog[sample] = nextData[sample];
                outDataLog[sample] = outData[sample];
#endif
            }
#if 0 //ENABLE_GAIN_LOGGING
            //NOW HERE -- NEXT DATA STILL LOOKS LIKE BS
            if (channel == 0 && debugLogEntry)
            {
                debugLogEntry->lastGain = static_cast<float> (outData[numSamples - 1]);

                //                DBG ("GAIN");
                //                for (size_t i = 0; i < numSamples; ++i)
                //                    DBG (gainLog[i]);

                //                DBG ("PREV");
                //                for (size_t i = 0; i < numSamples; ++i)
                //                    DBG (prevDataLog[i]);

                DBG ("NEXT");
                for (size_t i = 0; i < numSamples; ++i)
                    DBG (nextDataLog[i]);

                //                DBG ("OUT");
                //                for (size_t i = 0; i < numSamples; ++i)
                //                    DBG (outDataLog[i]);

                DBG ("DONE ENABLE_GAIN_LOGGING");
            }
#endif
        }
    }
#endif

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
