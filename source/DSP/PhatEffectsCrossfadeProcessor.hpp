/*
  ==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2024 Vincent Berthiaume

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#pragma once

#include "../Utility/Helpers.h"
#include "../modules/DebugLog/Source/DebugLog.hpp"
#include <sstream>

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

#if ENABLE_DEBUG_LOG
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
        jassert ((int) previousEffectBuffer.getNumChannels() >= nextEffectBuffer.getNumChannels()
                 && (int) nextEffectBuffer.getNumChannels() >= inputBlock.getNumChannels());
        jassert ((int) previousEffectBuffer.getNumSamples() >= nextEffectBuffer.getNumSamples()
                 && (int) nextEffectBuffer.getNumSamples() >= inputBlock.getNumSamples());

        const bool needToInverse = juce::approximatelyEqual (smoothedGainL.getTargetValue (), static_cast<T> (1));

        T curGain {};
        for (int channel = 0; channel < (int) inputBlock.getNumChannels (); ++channel)
        {
            const auto* prevData = previousEffectBuffer.getReadPointer (channel);
            const auto* nextData = nextEffectBuffer.getReadPointer (channel);
            auto*       outData  = context.getOutputBlock().getChannelPointer (channel);

            for (int sample = 0; sample < (int) inputBlock.getNumSamples(); ++sample)
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

#if ENABLE_DEBUG_LOG
                if (debugLogEntry && channel == 0 && sample == 0)
                    debugLogEntry->firstGain = static_cast<float> (outData[sample]);
#endif
            }
        }
    }

    EffectType prevEffect = EffectType::none;
    EffectType curEffect  = EffectType::verb;

  private:
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainL;
    juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear> smoothedGainR;
#if ENABLE_DEBUG_LOG
    DebugLogEntry* debugLogEntry { nullptr };
#endif
};
