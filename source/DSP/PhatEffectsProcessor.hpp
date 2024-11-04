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
        //TODO: probagate back the failure to change the effect
        //first reverse the smoothedGain. If we're not at one of the extremes, we got a double click which we'll ignore
        if (smoothedGain.getTargetValue() == 1.)
            smoothedGain.setTargetValue (0.);
        else if (smoothedGain.getTargetValue() == 0.)
            smoothedGain.setTargetValue (1.);
        else
        {
            jassertfalse;
            return;
        }

        if (curEffect == EffectType::verb)
            curEffect = EffectType::chorus;
        else if (curEffect == EffectType::chorus)
            curEffect = EffectType::verb;
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
                  juce::AudioBuffer<T>& outputBuffer)
    {
        jassert (previousEffectBuffer.getNumChannels() == nextEffectBuffer.getNumChannels() && nextEffectBuffer.getNumChannels() == outputBuffer.getNumChannels());
        jassert (previousEffectBuffer.getNumSamples() == nextEffectBuffer.getNumSamples() && nextEffectBuffer.getNumSamples() == outputBuffer.getNumSamples());

        const auto channels = outputBuffer.getNumChannels();
        const auto samples = outputBuffer.getNumSamples();
        //TODO: compare floating points bro?
        const auto needToInverse = smoothedGain.getTargetValue() == 1;

        for (int channel = 0; channel < channels; ++channel)
        {
            for (int sample = 0; sample < samples; ++sample)
            {
                //TODO: get this outta the loop lmao
                if (needToInverse)
                {
                    //get individual samples
                    const auto prev = previousEffectBuffer.getSample (channel, sample);
                    const auto next = nextEffectBuffer.getSample (channel, sample);

                    //mix and send to output
                    const auto gain = 1 - smoothedGain.getNextValue();
                    auto output = prev * gain + next * (1.0 - gain);
                    outputBuffer.setSample (channel, sample, static_cast<T> (output));
                }
                else
                {
                    //get individual samples
                    const auto prev = nextEffectBuffer.getSample (channel, sample);
                    const auto next = previousEffectBuffer.getSample (channel, sample);

                    //mix and send to output
                    const auto gain = smoothedGain.getNextValue();
                    auto output = prev * gain + next * (1.0 - gain);
                    outputBuffer.setSample (channel, sample, static_cast<T> (output));
                }
            }
        }
    }

private:
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> smoothedGain;

    //TODO: this needs to be stored and retreived from the state
    EffectType curEffect = EffectType::verb;
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
        fade_buffer1.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);
        fade_buffer2.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

        verbWrapper->prepare (spec);
        chorusWrapper->prepare (spec);
        effectCrossFader.prepare (spec);
    }

    void setEffectParam (juce::StringRef parameterID, T newValue)
    {
        const auto curEffect { effectCrossFader.getCurrentEffectType() };
        if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID())
        {
            if (curEffect == EffectType::verb)
            {
                reverbParams.roomSize = (float) newValue;
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
    }

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
