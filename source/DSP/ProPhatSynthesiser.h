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

#if ! EFFECTS_PROCESSOR_PER_VOICE
#include "PhatEffectsProcessor.hpp"
#endif
#include "LockFreeSynthesiser.h"
#include "ProPhatVoice.h"
#include "../Utility/Helpers.h"

/** The main Synthesiser for the plugin. It uses Constants::numVoices voices (of type ProPhatVoice),
*   and one ProPhatSound, which applies to all midi notes. It responds to paramater changes in the
*   state via juce::AudioProcessorValueTreeState::Listener().
*/
template <std::floating_point T>
class ProPhatSynthesiser : public LockFreeSynthesiser, public juce::AudioProcessorValueTreeState::Listener
{
  public:
    ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState);

    void addParamListenersToState();

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;

    void releaseResources ();

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void setMasterGain (float gain);

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override;

  private:
    void renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples) override;

    //TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled;

#if ! EFFECTS_PROCESSOR_PER_VOICE
    EffectsProcessor<T> effectsProcessor;
#endif

    //TODO: probably don't need the wrapper on this
    std::unique_ptr<EffectProcessorWrapper<juce::dsp::Gain<T>, T>> gainWrapper;

    juce::AudioProcessorValueTreeState& state;

    juce::dsp::ProcessSpec curSpecs;
};

//=====================================================================================================================

template <std::floating_point T>
ProPhatSynthesiser<T>::ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState)
: state (processorState)
{
    for (auto i = 0; i < Constants::numVoices; ++i)
        addVoice (new ProPhatVoice<T> (state, i, &voicesBeingKilled));

    addSound (new ProPhatSound());

    addParamListenersToState();

    gainWrapper = std::make_unique<EffectProcessorWrapper<juce::dsp::Gain<T>, T>>();
    gainWrapper->processor.setRampDurationSeconds (0.1);
    setMasterGain (Constants::defaultMasterGain);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::addParamListenersToState()
{
    using namespace ProPhatParameterIds;

#if ! EFFECTS_PROCESSOR_PER_VOICE
    state.addParameterListener (reverbParam1ID.getParamID(), this);
    state.addParameterListener (reverbParam2ID.getParamID(), this);

    state.addParameterListener (chorusParam1ID.getParamID(), this);
    state.addParameterListener (chorusParam2ID.getParamID(), this);

    state.addParameterListener (phaserParam1ID.getParamID(), this);
    state.addParameterListener (phaserParam2ID.getParamID(), this);

    state.addParameterListener (effectSelectedID.getParamID(), this);
#endif

    state.addParameterListener (masterGainID.getParamID(), this);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    if (Helpers::areSameSpecs (curSpecs, spec))
        return;

    curSpecs = spec;

    setCurrentPlaybackSampleRate (spec.sampleRate);

    for (auto* v : voices)
        dynamic_cast<ProPhatVoice<T>*> (v)->prepare (spec);

#if ! EFFECTS_PROCESSOR_PER_VOICE
    effectsProcessor.prepare (spec);
#endif

    gainWrapper->prepare (spec);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::releaseResources()
{
    for (auto* v : voices)
        dynamic_cast<ProPhatVoice<T>*> (v)->releaseResources();
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatSynthesiser::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == masterGainID.getParamID ())
        setMasterGain (newValue);

#if ! EFFECTS_PROCESSOR_PER_VOICE
    else if (parameterID == reverbParam1ID.getParamID() || parameterID == reverbParam2ID.getParamID()
        || parameterID == chorusParam1ID.getParamID() || parameterID == chorusParam2ID.getParamID()
        || parameterID == phaserParam1ID.getParamID() || parameterID == phaserParam2ID.getParamID())
        effectsProcessor.setEffectParam (parameterID, newValue);
    else if (parameterID == effectSelectedID.getParamID())
    {
        const auto effect = [val = static_cast<int> (newValue)]() -> EffectType
        {
            switch (val)
            {
                case 0: return EffectType::none;
                case 1: return EffectType::verb;
                case 2: return EffectType::chorus;
                case 3: return EffectType::phaser;
                default: jassertfalse; return EffectType::none;
            }
        }();

        effectsProcessor.changeEffect (effect);
    }
#endif
    else
        jassertfalse;
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::setMasterGain (float gain)
{
    gainWrapper->processor.setGainLinear (static_cast<T> (gain));
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    //don't start new voices in current buffer call if we have filled all voices already.
    //voicesBeingKilled should be reset after each renderNextBlock call
    if (voicesBeingKilled.size() >= Constants::numVoices)
        return;

    LockFreeSynthesiser::noteOn (midiChannel, midiNoteNumber, velocity);

#if LOG_EVERYTHING_AFTER_TRANSITION
    effectsProcessor.setIsPlaying (true);
#endif
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto audioBlock { juce::dsp::AudioBlock<T> (outputAudio).getSubBlock ((size_t) startSample, (size_t) numSamples) };
    const auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

#if ! EFFECTS_PROCESSOR_PER_VOICE
    effectsProcessor.process (context);
#endif

    gainWrapper->process (context);
}
