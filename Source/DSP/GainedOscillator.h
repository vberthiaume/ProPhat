/*
  ==============================================================================

   Copyright (c) 2023 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once
#include "../Utility/Helpers.h"
#include <random>

template <typename Type>
class GainedOscillator
{
public:
    GainedOscillator () :
        distribution ((Type) -1, (Type) 1)
    {
        setOscShape (OscShape::saw);
        setGain (Constants::defaultOscLevel);
    }

    void setFrequency (Type newValue, bool force = false)
    {
        jassert (newValue > 0);

        auto& osc = processorChain.template get<oscIndex> ();
        osc.setFrequency (newValue, force);
    }

    void setOscShape (int newShape)
    {
        auto& osc = processorChain.template get<oscIndex> ();

        bool wasActive = isActive;
        isActive = true;

        switch (newShape)
        {
            case OscShape::none:
                isActive = false;
                break;

            case OscShape::saw:
            {
                std::lock_guard<std::mutex> lock (processMutex);
                osc.initialise ([] (Type x)
                {
                    //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                    return juce::jmap (x, Type (-juce::MathConstants<double>::pi), Type (juce::MathConstants<double>::pi), Type (-1), Type (1));
                }, 2);
            }
            break;

            case OscShape::sawTri:
            {
                std::lock_guard<std::mutex> lock (processMutex);
                osc.initialise ([] (Type x)
                {
                    Type y = juce::jmap (x, Type (-juce::MathConstants<double>::pi), Type (juce::MathConstants<double>::pi), Type (-1), Type (1)) / 2;

                    if (x < 0)
                        return y += juce::jmap (x, Type (-juce::MathConstants<double>::pi), Type (0), Type (-1), Type (1)) / 2;
                    else
                        return y += juce::jmap (x, Type (0), Type (juce::MathConstants<double>::pi), Type (1), Type (-1)) / 2;

                }, 128);
            }
            break;

            case OscShape::triangle:
            {
                std::lock_guard<std::mutex> lock (processMutex);
                osc.initialise ([] (Type x)
                {
                    if (x < 0)
                        return juce::jmap (x, Type (-juce::MathConstants<double>::pi), Type (0), Type (-1), Type (1));
                    else
                        return juce::jmap (x, Type (0), Type (juce::MathConstants<double>::pi), Type (1), Type (-1));

                }, 128);
            }
            break;

            case OscShape::pulse:
            {
                std::lock_guard<std::mutex> lock (processMutex);
                osc.initialise ([] (Type x)
                {
                    if (x < 0)
                        return Type (-1);
                    else
                        return Type (1);
                }, 128);
            }
            break;

            case OscShape::noise:
            {
                std::lock_guard<std::mutex> lock (processMutex);

                osc.initialise ([&] (Type /*x*/)
                {
                    return distribution (generator);
                });
            }
            break;

            default:
                jassertfalse;
                break;
        }

        if (wasActive != isActive)
        {
            if (isActive)
                setGain (lastActiveGain);
            else
                setGain (0);
        }
    }

    /**
     * @brief Sets the gain for the oscillator in the processorChain.
        This ends up calling juce::dsp::Gain::setGainLinear(), which will ramp the change.
        The newGain is also cached in lastActiveGain, so we can recall that value if the
        oscillator is deactivated and reactivated.
    */
    void setGain (Type newGain)
    {
        if (! isActive)
            newGain = 0;
        else
            lastActiveGain = newGain;

        auto& gain = processorChain.template get<gainIndex> ();
        gain.setGainLinear (newGain);
    }

    Type getGain () { return lastActiveGain; }

    void reset () noexcept { processorChain.reset (); }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        //TODO: lock in audio thread!!!
        std::lock_guard<std::mutex> lock (processMutex);

        processorChain.process (context);
    }

    void prepare (const juce::dsp::ProcessSpec& spec) { processorChain.prepare (spec); }

private:
    enum
    {
        oscIndex,
        gainIndex
    };

    std::mutex processMutex;

    bool isActive = true;

    Type lastActiveGain {};

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> processorChain;

    std::uniform_real_distribution<Type> distribution;
    std::default_random_engine generator;
};
