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
        smoothedGain.reset (spec.sampleRate, .1);
    }

    void changeEffect(EffectType effect)
    {
        //first reverse the smoothedGain. If we're not at one of the extremes, we got a double click which we'll ignore
        jassert (juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1))
                 || juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (0)));
        if (juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1)))
            smoothedGain.setTargetValue (0.);
        else
            smoothedGain.setTargetValue (1.);

        prevEffect = curEffect;
        curEffect = effect;
    }

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGain.isSmoothing())
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
        const bool needToInverse = juce::approximatelyEqual (smoothedGain.getTargetValue(), static_cast<T> (1));

#if ENABLE_GAIN_LOGGING
        char* writePtr = debugLogEntry->message;
        size_t remaining = sizeof(debugLogEntry->message);
        int written = 0;
#endif

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);
            auto*       outData  = outputBuffer.getWritePointer (channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                //TODO VB: how do I know this smoothedGain ramp really goes from 1 to 0 over the total samples??
                //I need to print it
                const auto nextGain {smoothedGain.getNextValue()};
                const auto gain = needToInverse ? (1 - nextGain) : nextGain;
                outData[sample] = prevData[sample] * gain + nextData[sample] * (1 - gain);

#if ENABLE_GAIN_LOGGING
                //if (channel == 0)
                //  gainRamp << nextGain << ", ";
                if (channel == 0 && remaining > 0)
                {
                    written = std::snprintf(writePtr, remaining, "%.3f, ", nextGain);
                    if (written < 0 || static_cast<size_t>(written) >= remaining)
                    {
                        jassertfalse;
                        break; // Stop if buffer full or error
                    }
                    writePtr += written;
                    remaining -= written;
                }
#endif
            }
        }
#if ENABLE_GAIN_LOGGING
        DBG (debugLogEntry->message);
        //NOW HERE: this gainRamp breaks the debuglog, not too sure why
//        //also if I use ducoup here, once I call this once, it'll get called again and again randomly even when not playing???
//        //it actually does not get called multiple times, it's just that the entry isn't cleared. I think at least
//        if (debugLogEntry)
////            debugLogEntry->message = "ducoup";
//            debugLogEntry->message = gainRamp.str();
//        // else
//        //     jassertfalse;
#endif
    }

    EffectType prevEffect = EffectType::none;
    EffectType curEffect = EffectType::verb;

  private:
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGain;
#if ENABLE_GAIN_LOGGING
    DebugLogEntry* debugLogEntry{nullptr};
#endif

    //TODO: this needs to be stored and retrieved from the state
    //EffectType curEffect = EffectType::verb;
};
