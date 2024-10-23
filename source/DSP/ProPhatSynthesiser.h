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

#include "ProPhatVoice.h"
#include "PhatVerb.h"
#include "../Utility/Helpers.h"

template <std::floating_point T>
class Crossfade
{
public:
    enum ActiveBuffer { leftBuffer, rightBuffer };

    Crossfade() = default;

    /**
        Resets the crossfade, setting the sample rate and ramp length.

        @param sampleRate           The current sample rate.
        @param rampLengthInSeconds  The duration of the ramp in seconds.
    */
    void reset (double sampleRate, double rampLengthInSeconds)
    {
        smoothedGain.reset (sampleRate, rampLengthInSeconds);
    }

    /**
        Sets the active buffer. I.e. which one should be written to the output.
        @param buffer   An enum value indicating which buffer to output.
    */
    void setActiveBuffer (ActiveBuffer buffer)
    {
        if (buffer == leftBuffer)
            setGain (1.0);
        else
            setGain (0.0);
    }

    /**
        Applies the crossfade.

        Output buffer can be the same buffer as either of the inputs.

        All buffers should have the same number of channels and samples as each
        other, but if not, then the minimum number of channels/samples will be
        used.

        @param leftBuffer   The left input buffer to read from.
        @param rightBuffer  The right input buffer to read from.
        @param outputBuffer The buffer in which to store the result of the crossfade.
    */
    void process (const juce::AudioBuffer<T>& leftBuffer,
                  const juce::AudioBuffer<T>& rightBuffer,
                  juce::AudioBuffer<T>& outputBuffer)
    {
        // find the lowest number of channels available across all buffers
        const auto channels = std::min ({ leftBuffer.getNumChannels(),
                                          rightBuffer.getNumChannels(),
                                          outputBuffer.getNumChannels() });

        // find the lowest number of samples available across all buffers
        const auto samples = std::min ({ leftBuffer.getNumSamples(),
                                         rightBuffer.getNumSamples(),
                                         outputBuffer.getNumSamples() });

        for (int channel = 0; channel < channels; ++channel)
        {
            for (int sample = 0; sample < samples; ++sample)
            {
                // obtain the input samples from their respective buffers
                const auto left = leftBuffer.getSample (channel, sample);
                const auto right = rightBuffer.getSample (channel, sample);

                // get the next gain value in the smoothed ramp towards target
                const auto gain = smoothedGain.getNextValue();

                // calculate the output sample as a mix of left and right
                auto output = left * gain + right * (1.0 - gain);

                // store the output sample value
                outputBuffer.setSample (channel, sample, static_cast<T> (output));
            }
        }
    }

private:
    /**
        Can be used to set a custom gain level to combine the two buffers.
        @param gain     The gain level of the left buffer.
    */
    void setGain (double gain)
    {
        smoothedGain.setTargetValue (std::clamp (gain, 0.0, 1.0));
    }

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> smoothedGain;
};

//==================================================

template <std::floating_point T>
struct Crossfade_Processor
{
    PhatProcessorBase<T> processor1;
    PhatProcessorBase<T> processor2;
    std::vector<float> fade_buffer;

    Crossfade<T> effectCrossFader;

    void prepare (size_t max_buffer_size) { fade_buffer.reserve (max_buffer_size); } // pre-allocate!

    //NOW HERE: I'm not sure if jatin's code here is meant to be working on a copy of the audio or the audio itself
    void process (juce::AudioBuffer<T>&buffer, int startSample, int numSamples)
    {
        auto audioBlock { juce::dsp::AudioBlock<T> (buffer).getSubBlock ((size_t) startSample, (size_t) numSamples) };
        const auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

        if (use_processor1)
        {
            processor1.process (buffer);
            return;
        }
        if (use_processor2)
        {
            processor2.process (buffer);
            return;
        }

        fade_buffer.resize (buffer.size());
        std::copy (buffer.begin(), buffer.end(), fade_buffer.begin());

        processor1.process (fade_buffer);
        processor2.process (buffer);
        effectCrossFader.process (buffer, fade_buffer);

        return;
    }
};


//========================================================

/** The main Synthesiser for the plugin. It uses Constants::numVoices voices (of type ProPhatVoice),
*   and one ProPhatSound, which applies to all midi notes. It responds to paramater changes in the
*   state via juce::AudioProcessorValueTreeState::Listener().
*/
template <std::floating_point T>
class ProPhatSynthesiser : public juce::Synthesiser
                         , public juce::AudioProcessorValueTreeState::Listener
{
public:
    ProPhatSynthesiser(juce::AudioProcessorValueTreeState& processorState);

    void addParamListenersToState ();

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void setMasterGain (float gain);

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override;

    void changeEffect();

private:
    void setEffectParam (juce::StringRef parameterID, float newValue);

    void renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples) override;

    enum
    {
        reverbIndex = 0,
        masterGainIndex,
    };

    //TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled;

    std::unique_ptr<PhatProcessorWrapper<PhatVerbProcessor<T>, T>> verbWrapper;
    std::unique_ptr<PhatProcessorWrapper<juce::dsp::Chorus<T>, T>> chorusWrapper;
    
    std::vector<PhatProcessorBase<T>*> fxChain;

    std::unique_ptr<PhatProcessorWrapper<juce::dsp::Gain<T>, T>> gainWrapper;

    PhatVerbParameters reverbParams
    {
        //manually setting all these because we need to set the default room size and wet level to 0 if we want to be able to retrieve
        //these values from a saved state. If they are saved as 0 in the state, the event callback will not be propagated because
        //the change isn't forced-pushed
        0.0f, //< Room size, 0 to 1.0, where 1.0 is big, 0 is small.
        0.5f, //< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped.
        0.0f, //< Wet level, 0 to 1.0
        0.4f, //< Dry level, 0 to 1.0
        1.0f, //< Reverb width, 0 to 1.0, where 1.0 is very wide.
        0.0f  //< Freeze mode - values < 0.5 are "normal" mode, values > 0.5 put the reverb into a continuous feedback loop.
    };

    juce::AudioProcessorValueTreeState& state;

    juce::dsp::ProcessSpec curSpecs;
};

//=====================================================================================================================

template <std::floating_point T>
ProPhatSynthesiser<T>::ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState) :
    state (processorState)
{
    for (auto i = 0; i < Constants::numVoices; ++i)
        addVoice (new ProPhatVoice<T> (state, i, &voicesBeingKilled));

    addSound (new ProPhatSound ());

    addParamListenersToState ();

    //init our effects
    verbWrapper = std::make_unique<PhatProcessorWrapper<PhatVerbProcessor<T>, T>>();
    verbWrapper->processor.setParameters (reverbParams);

    gainWrapper = std::make_unique<PhatProcessorWrapper<juce::dsp::Gain<T>, T>>();
    gainWrapper->processor.setRampDurationSeconds (0.1);
    setMasterGain (Constants::defaultMasterGain);

    chorusWrapper = std::make_unique<PhatProcessorWrapper<juce::dsp::Chorus<T>, T>>();

    //TODO: make this dynamic
    //add effects to processing chain
    fxChain.push_back (verbWrapper.get());
    fxChain.push_back (chorusWrapper.get());

    fxChain.push_back (gainWrapper.get());
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    state.addParameterListener (effectParam1ID.getParamID (), this);
    state.addParameterListener (effectParam2ID.getParamID (), this);

    state.addParameterListener (masterGainID.getParamID (), this);
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

    for (const auto& fx : fxChain)
        fx->prepare (spec);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatSynthesiser::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == effectParam1ID.getParamID () || parameterID == effectParam2ID.getParamID ())
        setEffectParam (parameterID, newValue);
    else if (parameterID == masterGainID.getParamID ())
        setMasterGain (newValue);
    else
        jassertfalse;
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::setMasterGain (float gain)
{
    gainWrapper->processor.setGainLinear (static_cast<T> (gain));
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::setEffectParam ([[maybe_unused]] juce::StringRef parameterID, [[maybe_unused]] float newValue)
{
    if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;
    else
        jassertfalse;   //unknown effect parameter!

    verbWrapper->processor.setParameters (reverbParams);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    //don't start new voices in current buffer call if we have filled all voices already.
    //voicesBeingKilled should be reset after each renderNextBlock call
    if (voicesBeingKilled.size() >= Constants::numVoices)
        return;

    Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::changeEffect()
{

}

template <std::floating_point T>
void ProPhatSynthesiser<T>::renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto audioBlock { juce::dsp::AudioBlock<T> (outputAudio).getSubBlock ((size_t) startSample, (size_t) numSamples) };
    const auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

    for (const auto& fx : fxChain)
        fx->process (context);
}
