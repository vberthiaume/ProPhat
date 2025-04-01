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

#include "PhatEffectsCrossfadeProcessor.hpp"
#include "PhatVerb.h"
#include "ProPhatVoice.h"
template <std::floating_point T>
class EffectsProcessor
{
public:
    EffectsProcessor()
    {
        verbWrapper = std::make_unique<EffectProcessorWrapper<PhatVerbProcessor<T>, T>>();
        verbWrapper->processor.setParameters (reverbParams);

        chorusWrapper = std::make_unique<EffectProcessorWrapper<juce::dsp::Chorus<T>, T>>();
        phaserWrapper = std::make_unique<EffectProcessorWrapper<juce::dsp::Phaser<T>, T>>();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        // pre-allocate!
        fade_buffer1.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);
        fade_buffer2.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

        verbWrapper->prepare (spec);
        chorusWrapper->prepare (spec);
        phaserWrapper->prepare (spec);
        effectCrossFader.prepare (spec);
    }

    void setEffectParam (juce::StringRef parameterID, T newValue)
    {
        const auto curEffect { effectCrossFader.getCurrentEffectType() };
        if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID())
        {
            if (curEffect == EffectType::verb)
            {
                reverbParams.roomSize = static_cast<float> (newValue);
                verbWrapper->processor.setParameters (reverbParams);
            }
            else if (curEffect == EffectType::chorus)
            {
                chorusWrapper->processor.setRate (static_cast<T> (99.9) * newValue);
            }
            else if (curEffect == EffectType::phaser)
            {
                phaserWrapper->processor.setRate (static_cast<T> (99.9) * newValue);
            }
            else
            {
                jassertfalse;
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
            else if (curEffect == EffectType::phaser)
            {
                phaserWrapper->processor.setDepth (newValue);
                phaserWrapper->processor.setMix (newValue);
            }
            else
            {
                jassertfalse;
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
        needToClearEffect = true;

        //TODO: surround with trylock or something, although not here because we don't have a proper fallback
        const auto currentEffectType { effectCrossFader.getCurrentEffectType() };
        if (currentEffectType == EffectType::transitioning)
        {
            //copy the OG buffer into the individual processor ones
            fade_buffer1 = buffer;
            auto block1 {juce::dsp::AudioBlock<T> (fade_buffer1)};
            auto context1 { juce::dsp::ProcessContextReplacing<T> (block1) };

            fade_buffer2 = buffer;
            auto block2 {juce::dsp::AudioBlock<T> (fade_buffer2)};
            auto context2 { juce::dsp::ProcessContextReplacing<T> (block2) };

            //do the crossfade based on the actual current effect -- which is actually now the NEXT effect lol
            const auto nextEffect = effectCrossFader.curEffect;
            switch (nextEffect)
            {
                case EffectType::verb:
                    phaserWrapper->process (context1);
                    verbWrapper->process (context2);
                    break;
                case EffectType::chorus:
                    verbWrapper->process (context1);
                    chorusWrapper->process (context2);
                    break;
                case EffectType::phaser:
                    chorusWrapper->process (context1);
                    phaserWrapper->process (context2);
                    break;
                case EffectType::transitioning:
                default:
                    jassertfalse;
                    break;
            }

            //crossfade the 2 effects
            effectCrossFader.process (fade_buffer1, fade_buffer2, buffer);

            return;
        }

        // fade_buffer1.clear();
        // fade_buffer2.clear();

        auto audioBlock { juce::dsp::AudioBlock<T> (buffer).getSubBlock ((size_t) startSample, (size_t) numSamples) };
        auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

        if (currentEffectType == EffectType::verb)
        {
            verbWrapper->process (context);
            if (needToClearEffect)
            {
                phaserWrapper->reset();
                chorusWrapper->reset();
                needToClearEffect = false;
            }
        }
        else if (currentEffectType == EffectType::chorus)
        {
            chorusWrapper->process (context);
            if (needToClearEffect)
            {
                verbWrapper->reset();
                phaserWrapper->reset();
                needToClearEffect = false;
            }
        }
        else if (currentEffectType == EffectType::phaser)
        {
            phaserWrapper->process (context);
            if (needToClearEffect)
            {
                chorusWrapper->reset();
                verbWrapper->reset();
                needToClearEffect = false;
            }
        }
        else
            jassertfalse; //unknown effect!!
    }

private:
    bool needToClearEffect {false};
    std::unique_ptr<EffectProcessorWrapper<juce::dsp::Chorus<T>, T>> chorusWrapper;

    std::unique_ptr<EffectProcessorWrapper<juce::dsp::Phaser<T>, T>> phaserWrapper;

    std::unique_ptr<EffectProcessorWrapper<PhatVerbProcessor<T>, T>> verbWrapper;
    PhatVerbParameters<T>                                            reverbParams {
        //manually setting all these because we need to set the default room size and wet level to 0 if we want to be able to retrieve
        //these values from a saved state. If they are saved as 0 in the state, the event callback will not be propagated because
        //the change isn't forced-pushed
        static_cast<T> (0),   //< Room size, 0 to 1.0, where 1.0 is big, 0 is small.
        static_cast<T> (0.5), //< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped.
        static_cast<T> (0),   //< Wet level, 0 to 1.0
        static_cast<T> (0.4), //< Dry level, 0 to 1.0
        static_cast<T> (1),   //< Reverb width, 0 to 1.0, where 1.0 is very wide.
        static_cast<T> (0)    //< Freeze mode - values < 0.5 are "normal" mode, values > 0.5 put the reverb into a continuous feedback loop.
    };

    juce::AudioBuffer<T> fade_buffer1, fade_buffer2;
    EffectsCrossfadeProcessor<T> effectCrossFader;
};
