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

//I don't think this is making any difference
#define ENABLE_CLEAR_EFFECT 0

template <std::floating_point T>
class EffectsProcessor
{
  public:
    EffectsProcessor()
    {
        //init our log
        DebugLog tempData;
        memset (&tempData, 0, sizeof (tempData));

        //init the file that'll receive the log
        auto mmFile = juce::File (kSharedMemoryMapFilepath);
        mmFile.replaceWithData (&tempData, sizeof (tempData));

        m_pLogDebugMapping = std::make_unique<juce::MemoryMappedFile> (mmFile, juce::MemoryMappedFile::readWrite, false);
        m_pLogDebug        = static_cast<DebugLog*> (m_pLogDebugMapping->getData());
        if (m_pLogDebug)
        {
            m_pLogDebug->logHead = 0;
            memset (m_pLogDebug->log, 0, sizeof (m_pLogDebug->log));
        }
        else
        {
            jassertfalse; // failed to create the log
        }

        verbWrapper = std::make_unique<EffectProcessorWrapper<PhatVerbProcessor<T>, T>>();
        verbWrapper->processor.setParameters (reverbParams);

        chorusWrapper = std::make_unique<EffectProcessorWrapper<juce::dsp::Chorus<T>, T>>();
        phaserWrapper = std::make_unique<EffectProcessorWrapper<juce::dsp::Phaser<T>, T>>();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
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
            else if (curEffect != EffectType::none)
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
            else if (curEffect != EffectType::none)
            {
                jassertfalse;
            }
        }
        else
            jassertfalse; //unknown effect parameter!
    }

    void changeEffect(EffectType effect)
    {
        effectCrossFader.changeEffect(effect);
    }

    void process (juce::AudioBuffer<T>& buffer, int startSample, int numSamples)
    {
#if ENABLE_CLEAR_EFFECT
         needToClearEffect = true;
#endif

        //TODO: surround with trylock or something, although not here because we don't have a proper fallback
        const auto currentEffectType { effectCrossFader.getCurrentEffectType() };

        DebugLogEntry& debugLogEntry = m_pLogDebug->log[m_pLogDebug->logHead];
        if (enableLogging)
        {
            debugLogEntry.startTime = juce::Time::currentTimeMillis();
            debugLogEntry.curEffect = static_cast<int> (effectCrossFader.getCurrentEffectType());
        }

        if (currentEffectType == EffectType::transitioning)
        {
            //copy the OG buffer into the individual processor ones
            fade_buffer1 = buffer;
            auto block1 {juce::dsp::AudioBlock<T> (fade_buffer1)};
            auto context1 { juce::dsp::ProcessContextReplacing<T> (block1) };

            fade_buffer2 = buffer;
            auto block2 {juce::dsp::AudioBlock<T> (fade_buffer2)};
            auto context2 { juce::dsp::ProcessContextReplacing<T> (block2) };

            //do the crossfad between the previous and next effects
            const auto prevEffect = effectCrossFader.prevEffect;
            const auto nextEffect = effectCrossFader.curEffect;
            switch (prevEffect)
            {
                case EffectType::none:
                    fade_buffer1.clear();
                    break;
                case EffectType::verb:
                    verbWrapper->process (context1);
                    break;
                case EffectType::chorus:
                    chorusWrapper->process (context1);
                    break;
                case EffectType::phaser:
                    phaserWrapper->process (context1);
                    break;
                case EffectType::transitioning:
                default:
                    jassertfalse;
                    break;
            }

            switch (nextEffect)
            {
                case EffectType::none:
                    fade_buffer2.clear();
                    break;
                case EffectType::verb:
                    verbWrapper->process (context2);
                    break;
                case EffectType::chorus:
                    chorusWrapper->process (context2);
                    break;
                case EffectType::phaser:
                    phaserWrapper->process (context2);
                    break;
                case EffectType::transitioning:
                default:
                    jassertfalse;
                    break;
            }

            //crossfade the 2 effects
            effectCrossFader.process (fade_buffer1, fade_buffer2, buffer);
        }
        else
        {
            auto audioBlock { juce::dsp::AudioBlock<T> (buffer).getSubBlock ((size_t) startSample, (size_t) numSamples) };
            auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

            if (currentEffectType == EffectType::verb)
            {
                verbWrapper->process (context);
#if ENABLE_CLEAR_EFFECT
                if (needToClearEffect)
                {
                    phaserWrapper->reset();
                    chorusWrapper->reset();
                    needToClearEffect = false;
                }
#endif
            }
            else if (currentEffectType == EffectType::chorus)
            {
                chorusWrapper->process (context);
#if ENABLE_CLEAR_EFFECT
                if (needToClearEffect)
                {
                    verbWrapper->reset();
                    phaserWrapper->reset();
                    needToClearEffect = false;
                }
#endif
            }
            else if (currentEffectType == EffectType::phaser)
            {
                phaserWrapper->process (context);
#if ENABLE_CLEAR_EFFECT
                if (needToClearEffect)
                {
                    chorusWrapper->reset();
                    verbWrapper->reset();
                    needToClearEffect = false;
                }
#endif
            }
            else if (currentEffectType != EffectType::none)
                jassertfalse; //unknown effect!!
        }

        if (enableLogging)
        {
            //probably there's a way to increase the head at the beginning of the function,
            //and then I'd be able to do the early return instead of this big else section
            debugLogEntry.endTime = juce::Time::currentTimeMillis();
            m_pLogDebug->logHead = (m_pLogDebug->logHead + 1) & (kMaxDebugEntries - 1);
            //the AI suggested this, probably unrelated
            // memset (&debugLogEntry, 0, sizeof (DebugLogEntry));
        }
    }

private:
#if ENABLE_CLEAR_EFFECT
    bool needToClearEffect {false};
#endif
    std::unique_ptr<juce::MemoryMappedFile> m_pLogDebugMapping{};
    DebugLog* m_pLogDebug{};
    bool enableLogging {true};

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
