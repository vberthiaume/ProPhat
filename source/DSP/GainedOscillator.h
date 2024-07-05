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
#include <random>

template <std::floating_point T>
class GainedOscillator
{
public:
    GainedOscillator () :
        distribution ((T) -1, (T) 1)
    {
        setOscShape (OscShape::saw);
        setGain (Constants::defaultOscLevel);
    }

    void setFrequency (T newValue, bool force = false)
    {
        jassert (newValue > 0);

        auto& osc = processorChain.template get<oscIndex> ();
        osc.setFrequency (newValue, force);
    }

    void setOscShape (int newShape);

    /**
     * @brief Sets the gain for the oscillator in the processorChain.
        This ends up calling juce::dsp::Gain::setGainLinear(), which will ramp the change.
        The newGain is also cached in lastActiveGain, so we can recall that value if the
        oscillator is deactivated and reactivated.
    */
    void setGain (T newGain)
    {
        if (! isActive)
            newGain = 0;
        else
            lastActiveGain = newGain;

        auto& gain = processorChain.template get<gainIndex> ();
        gain.setGainLinear (newGain);
    }

    T getGain () { return lastActiveGain; }

    void reset () noexcept { processorChain.reset (); }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        //TODO: lock in audio thread!!! This should/could be a try lock and the other one a lock,
        // so we only lock in the other place. But are these actuallty different threads though?
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

    T lastActiveGain {};

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<T>, juce::dsp::Gain<T>> processorChain;

    std::uniform_real_distribution<T> distribution;
    std::default_random_engine generator;
};

//====================================================================================================

template<std::floating_point T>
void GainedOscillator<T>::setOscShape (int newShape)
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
        osc.initialise ([](T x)
                        {
                            //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                            return juce::jmap (x, T (-juce::MathConstants<double>::pi), T (juce::MathConstants<double>::pi), T (-1), T (1));
                        }, 2);
    }
    break;

    case OscShape::sawTri:
    {
        std::lock_guard<std::mutex> lock (processMutex);
        osc.initialise ([](T x)
                        {
                            T y = juce::jmap (x, T (-juce::MathConstants<double>::pi), T (juce::MathConstants<double>::pi), T (-1), T (1)) / 2;

                            if (x < 0)
                                return y += juce::jmap (x, T (-juce::MathConstants<double>::pi), T (0), T (-1), T (1)) / 2;
                            else
                                return y += juce::jmap (x, T (0), T (juce::MathConstants<double>::pi), T (1), T (-1)) / 2;

                        }, 128);
    }
    break;

    case OscShape::triangle:
    {
        std::lock_guard<std::mutex> lock (processMutex);
        osc.initialise ([](T x)
                        {
                            if (x < 0)
                                return juce::jmap (x, T (-juce::MathConstants<double>::pi), T (0), T (-1), T (1));
                            else
                                return juce::jmap (x, T (0), T (juce::MathConstants<double>::pi), T (1), T (-1));

                        }, 128);
    }
    break;

    case OscShape::pulse:
    {
        std::lock_guard<std::mutex> lock (processMutex);
        osc.initialise ([](T x)
                        {
                            if (x < 0)
                                return T (-1);
                            else
                                return T (1);
                        }, 128);
    }
    break;

    case OscShape::noise:
    {
        std::lock_guard<std::mutex> lock (processMutex);

        osc.initialise ([&](T /*x*/)
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
