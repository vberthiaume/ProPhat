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
        //TODO: I should compare these waves on the scope with the waves from the prophet
        oscs[OscShape::none].initialise ([] (T /*x*/) { return T (0); });
        oscs[OscShape::saw].initialise  ([] (T x) { return juce::jmap (x, T (-juce::MathConstants<T>::pi), T (juce::MathConstants<T>::pi), T (-1), T (1)); }, 2);

        oscs[OscShape::sawTri].initialise ([] (T x)
                                           {
                                               T y = juce::jmap (x, T (-juce::MathConstants<T>::pi), T (juce::MathConstants<T>::pi), T (-1), T (1)) / 2;

                                               if (x < 0)
                                                   return y += juce::jmap (x, T (-juce::MathConstants<T>::pi), T (0), T (-1), T (1)) / 2;
                                               else
                                                   return y += juce::jmap (x, T (0), T (juce::MathConstants<T>::pi), T (1), T (-1)) / 2;
                                           }, 128);

        oscs[OscShape::triangle].initialise ([] (T x)
                                             {
                                                 if (x < 0)
                                                     return juce::jmap (x, T (-juce::MathConstants<T>::pi), T (0), T (-1), T (1));
                                                 else
                                                     return juce::jmap (x, T (0), T (juce::MathConstants<T>::pi), T (1), T (-1));
                                             }, 128);

        oscs[OscShape::pulse].initialise ([] (T x) { if (x < 0) return T (-1); else return T (1); }, 16);
        oscs[OscShape::noise].initialise ([this] (T /*x*/) { return distribution (generator); });

        setOscShape (OscShape::saw);
        setGain (Constants::defaultOscLevel);
    }

    void setFrequency (T newValue, bool force = false)
    {
        jassert (newValue > 0);

        curOsc.load()->setFrequency (newValue, force);
    }

    void setOscShape (OscShape::Values newShape)
    {
        //this is to make sure we preserve the same gain after we re-init, right?
        bool wasActive = isActive;
        isActive = true;

        if (newShape == OscShape::none)
            isActive = false;

        switch (newShape)
        {
            case OscShape::none:     curOsc.store (&oscs[OscShape::none]);      break;
            case OscShape::saw:      curOsc.store (&oscs[OscShape::saw]);       break;
            case OscShape::sawTri:   curOsc.store (&oscs[OscShape::sawTri]);    break;
            case OscShape::triangle: curOsc.store (&oscs[OscShape::triangle]);  break;
            case OscShape::pulse:    curOsc.store (&oscs[OscShape::pulse]);     break;
            case OscShape::noise:    curOsc.store (&oscs[OscShape::noise]);     break;
            default: jassertfalse;
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
    void setGain (T newGain)
    {
        if (! isActive)
            newGain = 0;
        else
            lastActiveGain = newGain;

        gain.setGainLinear (newGain);
    }

    T getGain () { return lastActiveGain; }

    void reset () noexcept
    {
        curOsc.load ()->reset();
        gain.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        curOsc.load ()->process (context);
        gain.process (context);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& osc : oscs)
            osc.prepare (spec);

        gain.prepare (spec);
    }

private:

    std::array<juce::dsp::Oscillator<T>, OscShape::actualTotal> oscs;
    std::atomic<juce::dsp::Oscillator<T>*> curOsc { nullptr };

    bool isActive = true;

    T lastActiveGain {};
    juce::dsp::Gain<T> gain;

    std::uniform_real_distribution<T> distribution;
    std::default_random_engine generator;
};

//====================================================================================================
