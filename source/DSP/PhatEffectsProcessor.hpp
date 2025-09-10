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

#include "../modules/DebugLog/Source/DebugLog.hpp"
#include "PhatEffectsCrossfadeProcessor.hpp"
#include "PhatVerb.h"

//if I don't clear the effects, their buffers will still contain the last processed content
//but I don't think this changes anything for glitches
#define ENABLE_CLEAR_EFFECT 1
#define LOG_EVERYTHING_AFTER_TRANSITION 0

template <std::floating_point T>
class EffectsProcessor
{
  public:
    EffectsProcessor()
    {
#if ENABLE_DEBUG_LOG
        //TODO: presumably this can be DRYed into some function and called from the main processor
        DebugLog tempData;
        memset (&tempData, 0, sizeof (tempData));

        // Initialize the file that will receive the log
//        if (! memoryMappedFile.existsAsFile() || memoryMappedFile.getSize() != sizeof (DebugLog))
        {
            bool success = memoryMappedFile.deleteFile(); // Start clean if it already exists
            success      = memoryMappedFile.replaceWithData (&tempData, sizeof (tempData));
            jassert (success); // Fail loudly if replaceWithData failed
        }
        jassert (memoryMappedFile.getSize() == sizeof (DebugLog)); // Confirm correct size

        // Create the memory mapped file
        m_pLogDebugMapping = std::make_unique<juce::MemoryMappedFile> (memoryMappedFile, juce::MemoryMappedFile::readWrite, false);
        jassert (m_pLogDebugMapping != nullptr);

        m_pLogDebug = static_cast<DebugLog*> (m_pLogDebugMapping->getData());
        if (m_pLogDebug)
        {
            m_pLogDebug->logHead = 0;
            memset (m_pLogDebug->log, 0, sizeof (m_pLogDebug->log));
        }
        else
        {
            jassertfalse; // Failed to map the log memory!
        }
#endif

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

#if LOG_EVERYTHING_AFTER_TRANSITION
    bool isPlaying         = false;
    bool transitionStarted = false;

    void setIsPlaying (bool pIsPlaying)
    {
        isPlaying = pIsPlaying;
    }
#endif

    void setEffectParam (juce::StringRef parameterID, T newValue)
    {
        if (parameterID == ProPhatParameterIds::reverbParam1ID.getParamID())
        {
            reverbParams.roomSize = static_cast<float> (newValue);
            verbWrapper->processor.setParameters (reverbParams);
        }
        else if (parameterID == ProPhatParameterIds::chorusParam1ID.getParamID())
        {
            chorusWrapper->processor.setRate (static_cast<T> (99.9) * newValue);
        }
        else if (parameterID == ProPhatParameterIds::phaserParam1ID.getParamID())
        {
            phaserWrapper->processor.setRate (static_cast<T> (99.9) * newValue);
        }
        else if (parameterID == ProPhatParameterIds::reverbParam2ID.getParamID())
        {
            reverbParams.wetLevel = newValue;
            verbWrapper->processor.setParameters (reverbParams);
        }
        else if (parameterID == ProPhatParameterIds::chorusParam2ID.getParamID())
        {
            chorusWrapper->processor.setDepth (newValue);
            chorusWrapper->processor.setMix (newValue);
        }
        else if (parameterID == ProPhatParameterIds::phaserParam2ID.getParamID())
        {
            phaserWrapper->processor.setDepth (newValue);
            phaserWrapper->processor.setMix (newValue);
        }
        else
            jassertfalse; //unknown effect parameter!
    }

    void changeEffect (EffectType effect)
    {
        //NO (audible) GLITCH if I comment this out
        effectCrossFader.changeEffect (effect);

#if LOG_EVERYTHING_AFTER_TRANSITION
        if (isPlaying)
            transitionStarted = true;
#endif
    }

void process (const juce::dsp::ProcessContextReplacing<T>& context)
{
#if ENABLE_CLEAR_EFFECT
    needToClearEffect = true;
#endif

    const auto inputBlock { context.getInputBlock () };
    const auto numSamples { static_cast<int> (inputBlock.getNumSamples ()) };
    //TODO: surround with trylock or something, although not here because we don't have a proper fallback
    const auto currentEffectType { effectCrossFader.getCurrentEffectType () };

#if ENABLE_DEBUG_LOG
    DebugLogEntry& debugLogEntry = m_pLogDebug->log[m_pLogDebug->logHead];
    if (m_pLogDebug != nullptr)
    {
        //TODO VB: couldn't I just make a new debug entry?
        //IMPORTANT! Need to clear all the debug entry fields here so that we overwrite the previous one properly!
        debugLogEntry.timeSinceLastCall = juce::Time::currentTimeMillis () - cachedProcessCallTime;
        cachedProcessCallTime = juce::Time::currentTimeMillis ();
        debugLogEntry.curEffect = static_cast<int> (effectCrossFader.getCurrentEffectType ());
#if ENABLE_GAIN_LOGGING
        debugLogEntry.firstGain = {};
        debugLogEntry.lastGain = {};
#endif
    }
#endif

    if (currentEffectType == EffectType::transitioning)
    {
#if ENABLE_GAIN_LOGGING
        effectCrossFader.setDebugLogEntry (&debugLogEntry);
#endif
        jassert (fade_buffer1.getNumSamples() >= numSamples && fade_buffer2.getNumSamples() >= numSamples);

        //copy the OG buffer into the individual processor ones
        for (auto c = 0; c < inputBlock.getNumChannels (); ++c)
        {
            //TODO VB: look into copyFromWithRamp!!! we could probably use this to do the crossfade
            fade_buffer1.copyFrom (c, 0, inputBlock.getChannelPointer (c), numSamples);
            fade_buffer2.copyFrom (c, 0, inputBlock.getChannelPointer (c), numSamples);
        }

    //THE GLITCH HAS TO BE SOMEWHERE IN HERE
        auto block1 { juce::dsp::AudioBlock<T> (fade_buffer1).getSubBlock ((size_t) 0, (size_t) numSamples) };
        auto context1 { juce::dsp::ProcessContextReplacing<T> (block1) };

        auto block2 { juce::dsp::AudioBlock<T> (fade_buffer2).getSubBlock ((size_t) 0, (size_t) numSamples) };
        auto context2 { juce::dsp::ProcessContextReplacing<T> (block2) };

        //do the crossfade between the previous and next effects
        const auto prevEffect = effectCrossFader.prevEffect;
        const auto nextEffect = effectCrossFader.curEffect;

        switch (prevEffect)
        {
        case EffectType::none:
            fade_buffer1.clear ();
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
            fade_buffer2.clear ();
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
        effectCrossFader.process (fade_buffer1, fade_buffer2, context);
    }
    else
    {
        if (currentEffectType == EffectType::verb)
        {
            verbWrapper->process (context);
#if ENABLE_CLEAR_EFFECT
            if (needToClearEffect)
            {
                phaserWrapper->reset ();
                chorusWrapper->reset ();
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
                verbWrapper->reset ();
                phaserWrapper->reset ();
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
                chorusWrapper->reset ();
                verbWrapper->reset ();
                needToClearEffect = false;
            }
#endif
        }
        else if (currentEffectType != EffectType::none)
            jassertfalse; //unknown effect!!
    }

#if LOG_EVERYTHING_AFTER_TRANSITION
    if (transitionStarted)
    {
        int asdf = 0;
        for (int i = 0; i < numSamples; ++i)
            DBG (inputBlock.getChannelPointer (0)[i]);
        ++asdf;
    }
#endif

#if ENABLE_DEBUG_LOG
    debugLogEntry.processCallDuration = juce::Time::currentTimeMillis () - cachedProcessCallTime;
    m_pLogDebug->logHead = (m_pLogDebug->logHead + 1) & (kMaxDebugEntries - 1);
#endif
}

  private:
#if ENABLE_CLEAR_EFFECT
    bool needToClearEffect { false };
#endif
#if ENABLE_DEBUG_LOG
    juce::int64                             cachedProcessCallTime { juce::Time::currentTimeMillis() };
    std::unique_ptr<juce::MemoryMappedFile> m_pLogDebugMapping {};
    DebugLog*                               m_pLogDebug {};
#endif

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

    juce::AudioBuffer<T>         fade_buffer1, fade_buffer2;
    EffectsCrossfadeProcessor<T> effectCrossFader;
};
