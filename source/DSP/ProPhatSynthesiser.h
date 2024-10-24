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
class EffectsCrossfadeProcessor
{
public:
    enum EffectType
    {
        verb = 0,
        phaser,
        transitioning
    };
    EffectType curEffect = EffectType::verb;

    EffectsCrossfadeProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        //smoothedGain.reset (sampleRate, rampLengthInSeconds);
        smoothedGain.reset (spec.maximumBlockSize);
    }

    void changeEffect()
    {
        if (curEffect == EffectType::verb)
            curEffect = EffectType::phaser;
        else if (curEffect == EffectType::phaser)
                curEffect = EffectType::verb;
        else
            jassertfalse;

        if (curEffect == EffectType::verb)
            setGain (1.0);
        else
            setGain (0.0);
    };

    EffectType getCurrentEffectType() const
    {
        //TODO: this isn't atomic. Try lock?
        if (smoothedGain.isSmoothing())
            return EffectType::transitioning;

        return curEffect;
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
#if 1
        jassert (leftBuffer.getNumChannels() == rightBuffer.getNumChannels()
                 && rightBuffer.getNumChannels() == outputBuffer.getNumChannels());
        jassert (leftBuffer.getNumSamples() == rightBuffer.getNumSamples()
                 && rightBuffer.getNumSamples() == outputBuffer.getNumSamples());

        const auto channels = outputBuffer.getNumChannels();
        const auto samples = outputBuffer.getNumSamples();

#else
        // find the lowest number of channels available across all buffers
        const auto channels = std::min ({ leftBuffer.getNumChannels(),
            rightBuffer.getNumChannels(),
            outputBuffer.getNumChannels() });

        // find the lowest number of samples available across all buffers
        const auto samples = std::min ({ leftBuffer.getNumSamples(),
            rightBuffer.getNumSamples(),
            outputBuffer.getNumSamples() });
#endif

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

        verbWrapper->prepare(spec);
        chorusWrapper->prepare (spec);
        effectCrossFader.prepare (spec);
    }

    template <std::floating_point T>
    void setEffectParam (juce::StringRef parameterID, T newValue) 
    {
        if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID())
            reverbParams.roomSize = newValue;
        else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID())
            reverbParams.wetLevel = newValue;
        else
            jassertfalse; //unknown effect parameter!

        verbWrapper->processor.setParameters (reverbParams);
    }

   void changeEffect()
    {
        effectCrossFader.changeEffect();
    };

    void process (juce::AudioBuffer<T>& buffer, int startSample, int numSamples)
    {
        //TODO: surround with trylock or something
        const auto currentEffectType { effectCrossFader.getCurrentEffectType() };

        if (currentEffectType == EffectsCrossfadeProcessor<T>::EffectType::transitioning)
        {
            //copy the OG buffer into the individual processor ones
            fade_buffer1 = buffer;
            fade_buffer2 = buffer;

            //make the individual blocks and process
            auto audioBlock1 { juce::dsp::AudioBlock<T> (fade_buffer1).getSubBlock ((size_t) startSample, (size_t) numSamples) };
            auto context1    { juce::dsp::ProcessContextReplacing<T> (audioBlock1) };
            verbWrapper->process (context1);

            auto audioBlock2 { juce::dsp::AudioBlock<T> (fade_buffer2).getSubBlock ((size_t) startSample, (size_t) numSamples) };
            auto context2    { juce::dsp::ProcessContextReplacing<T> (audioBlock2) };
            chorusWrapper->process (context2);

            //crossfade the 2 effects
            effectCrossFader.process (fade_buffer1, fade_buffer2, buffer);

            return;
        }

        auto audioBlock { juce::dsp::AudioBlock<T> (buffer).getSubBlock ((size_t) startSample, (size_t) numSamples) };
        auto context    { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

        if (currentEffectType == EffectsCrossfadeProcessor<T>::EffectType::verb)
            verbWrapper->process (context);
        else if (currentEffectType == EffectsCrossfadeProcessor<T>::EffectType::phaser)
            chorusWrapper->process (context);
        else
            jassertfalse;   //unknown effect!!
    }

private:
    std::unique_ptr<PhatProcessorWrapper<juce::dsp::Chorus<T>, T>> chorusWrapper;

    std::unique_ptr<PhatProcessorWrapper<PhatVerbProcessor<T>, T>> verbWrapper;

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
        0.0f //< Freeze mode - values < 0.5 are "normal" mode, values > 0.5 put the reverb into a continuous feedback loop.
    };


    juce::AudioBuffer<T> fade_buffer1, fade_buffer2;
    EffectsCrossfadeProcessor<T> effectCrossFader;
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
    void renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples) override;

    //TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled;

    EffectsProcessor<T> effectsProcessor;

    //TODO: probably don't need the wrapper on this
    std::unique_ptr<PhatProcessorWrapper<juce::dsp::Gain<T>, T>> gainWrapper;

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

    gainWrapper = std::make_unique<PhatProcessorWrapper<juce::dsp::Gain<T>, T>>();
    gainWrapper->processor.setRampDurationSeconds (0.1);
    setMasterGain (Constants::defaultMasterGain);
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

    effectsProcessor.prepare(spec);

    gainWrapper->prepare (spec);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatSynthesiser::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == effectParam1ID.getParamID () || parameterID == effectParam2ID.getParamID ())
        effectsProcessor.setEffectParam (parameterID, newValue);
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

//template <std::floating_point T>
//void ProPhatSynthesiser<T>::setEffectParam (juce::StringRef parameterID, float newValue)
//{
//    if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID ())
//        reverbParams.roomSize = newValue;
//    else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID ())
//        reverbParams.wetLevel = newValue;
//    else
//        jassertfalse;   //unknown effect parameter!
//
//    verbWrapper->processor.setParameters (reverbParams);
//}

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
    effectsProcessor.changeEffect();
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::renderVoices (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    //TODO: this converts the arguments internally to a context, exactly like below, so might as well use that directly as params
    effectsProcessor.process (outputAudio, startSample, numSamples);
    
    auto audioBlock { juce::dsp::AudioBlock<T> (outputAudio).getSubBlock ((size_t) startSample, (size_t) numSamples) };
    const auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };

    //for (const auto& fx : fxChain)
    //    fx->process (context);

    gainWrapper->process (context);
}
