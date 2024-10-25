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
#include "PhatVerb.h"
#include "ProPhatVoice.h"

enum class EffectType
{
    verb = 0,
    chorus,
    transitioning
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
        if (curEffect == EffectType::verb)
            curEffect = EffectType::chorus;
        else if (curEffect == EffectType::chorus)
            curEffect = EffectType::verb;
        else
            jassertfalse;

        if (curEffect == EffectType::verb)
            smoothedGain.setTargetValue (1.);
        else
            smoothedGain.setTargetValue (0.);

        //this does not resolve the click we get on the first transition
        //if (curEffect == EffectType::verb)
        //    smoothedGain.setTargetValue (0.);
        //else
        //    smoothedGain.setTargetValue (1.);
    };

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGain.isSmoothing())
            return EffectType::transitioning;

        return curEffect;
    }

    /** so this needs to be a previous and next buffer, and the smoothing needs to always be 1 -> 0.
    *   I don' think this class should know anything about the processors... actually it should just get
    *   references to the processors, or even just their processed outputs?
    */
    void process (const juce::AudioBuffer<T>& leftBuffer,
        const juce::AudioBuffer<T>& rightBuffer,
        juce::AudioBuffer<T>& outputBuffer)
    {
        jassert (leftBuffer.getNumChannels() == rightBuffer.getNumChannels() && rightBuffer.getNumChannels() == outputBuffer.getNumChannels());
        jassert (leftBuffer.getNumSamples() == rightBuffer.getNumSamples() && rightBuffer.getNumSamples() == outputBuffer.getNumSamples());

        const auto channels = outputBuffer.getNumChannels();
        const auto samples = outputBuffer.getNumSamples();

        for (int channel = 0; channel < channels; ++channel)
        {
            for (int sample = 0; sample < samples; ++sample)
            {
                // obtain the input samples from their respective buffers
                const auto left = leftBuffer.getSample (channel, sample);
                const auto right = rightBuffer.getSample (channel, sample);

                // get the next gain value in the smoothed ramp towards target
                const auto gain = smoothedGain.getNextValue();
                DBG(gain);

                // calculate the output sample as a mix of left and right
                auto output = left * gain + right * (1.0 - gain);

                // store the output sample value
                outputBuffer.setSample (channel, sample, static_cast<T> (output));
            }
        }
    }

private:
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> smoothedGain;

    //TODO: this needs to be stored and retreived from the state
    EffectType curEffect = EffectType::verb;

    //changing the default curEffect to chorus DOES fix the click on first transition. Weird man
    //EffectType curEffect = EffectType::chorus;
};

//==================================================

template <std::floating_point T>
class EffectsProcessor
{
public:
    EffectsProcessor()
    {
        verbWrapper = std::make_unique<PhatProcessorWrapper<PhatVerbProcessor<T>, T>>();
        verbWrapper->processor.setParameters (reverbParams);

        chorusWrapper = std::make_unique<PhatProcessorWrapper<juce::dsp::Chorus<T>, T>>();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        // pre-allocate!
        fade_buffer1.setSize (spec.numChannels, spec.maximumBlockSize);
        fade_buffer2.setSize (spec.numChannels, spec.maximumBlockSize);

        verbWrapper->prepare (spec);
        chorusWrapper->prepare (spec);
        effectCrossFader.prepare (spec);
    }

    template <std::floating_point T>
    void setEffectParam (juce::StringRef parameterID, T newValue)
    {
        const auto curEffect { effectCrossFader.getCurrentEffectType() };
        if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID())
        {
            if (curEffect == EffectType::verb)
            {
                reverbParams.roomSize = newValue;
                verbWrapper->processor.setParameters (reverbParams);
            }
            else if (curEffect == EffectType::chorus)
            {
                chorusWrapper->processor.setRate (static_cast<T> (99.9) * newValue);
            }
        }
        else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID())
        {
            if (curEffect == EffectType::verb)
            {
                reverbParams.wetLevel = newValue;
                verbWrapper->processor.setParameters (reverbParams);
            }
            else if (curEffect == EffectType::chorus)
            {
                chorusWrapper->processor.setDepth (newValue);
                chorusWrapper->processor.setMix (newValue);
            }
        }
        else
            jassertfalse; //unknown effect parameter!
    }

    void changeEffect()
    {
        effectCrossFader.changeEffect();
    };

    void process (juce::AudioBuffer<T>& buffer, int startSample, int numSamples)
    {
        //TODO: surround with trylock or something
        const auto currentEffectType { effectCrossFader.getCurrentEffectType() };

        if (currentEffectType == EffectType::transitioning)
        {
            //copy the OG buffer into the individual processor ones
            fade_buffer1 = buffer;
            fade_buffer2 = buffer;

            //make the individual blocks and process
            auto audioBlock1 { juce::dsp::AudioBlock<T> (fade_buffer1).getSubBlock ((size_t) startSample, (size_t) numSamples) };
            auto context1 { juce::dsp::ProcessContextReplacing<T> (audioBlock1) };
            verbWrapper->process (context1);

            auto audioBlock2 { juce::dsp::AudioBlock<T> (fade_buffer2).getSubBlock ((size_t) startSample, (size_t) numSamples) };
            auto context2 { juce::dsp::ProcessContextReplacing<T> (audioBlock2) };
            chorusWrapper->process (context2);

            //crossfade the 2 effects
            effectCrossFader.process (fade_buffer1, fade_buffer2, buffer);

            return;
        }

        auto audioBlock { juce::dsp::AudioBlock<T> (buffer).getSubBlock ((size_t) startSample, (size_t) numSamples) };
        auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

        if (currentEffectType == EffectType::verb)
            verbWrapper->process (context);
        else if (currentEffectType == EffectType::chorus)
            chorusWrapper->process (context);
        else
            jassertfalse; //unknown effect!!
    }

private:
    std::unique_ptr<PhatProcessorWrapper<juce::dsp::Chorus<T>, T>> chorusWrapper;

    std::unique_ptr<PhatProcessorWrapper<PhatVerbProcessor<T>, T>> verbWrapper;
    PhatVerbParameters reverbParams {
        //manually setting all these because we need to set the default room size and wet level to 0 if we want to be able to retrieve
        //these values from a saved state. If they are saved as 0 in the state, the event callback will not be propagated because
        //the change isn't forced-pushed
        0.0f, //< Room size, 0 to 1.0, where 1.0 is big, 0 is small.
        0.5f, //< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped.
        0.0f, //< Wet level, 0 to 1.0
        0.4f, //< Dry level, 0 to 1.0
        1.0f, //< Reverb width, 0 to 1.0, where 1.0 is very wide.
        0.0f //< Freeze mode - values < 0.5 are "normal" mode, values > 0.5 put the reverb into a continuous feedback loop.
    };

    juce::AudioBuffer<T> fade_buffer1, fade_buffer2;
    EffectsCrossfadeProcessor<T> effectCrossFader;
};