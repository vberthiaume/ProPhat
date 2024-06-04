/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "ProPhatVoice.h"
#include "../Utility/Helpers.h"

template <std::floating_point T>
class PhatReverb
{
public:
    //==============================================================================
    PhatReverb ()
    {
        setParameters (Parameters ());
        setSampleRate (44100.0);
    }

    //==============================================================================
    /** Holds the parameters being used by a Reverb object. */
    struct Parameters
    {
        float roomSize = 0.5f;     /**< Room size, 0 to 1.0, where 1.0 is big, 0 is small. */
        float damping = 0.5f;     /**< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped. */
        float wetLevel = 0.33f;    /**< Wet level, 0 to 1.0 */
        float dryLevel = 0.4f;     /**< Dry level, 0 to 1.0 */
        float width = 1.0f;     /**< Reverb width, 0 to 1.0, where 1.0 is very wide. */
        float freezeMode = 0.0f;     /**< Freeze mode - values < 0.5 are "normal" mode, values > 0.5
                                          put the reverb into a continuous feedback loop. */
    };

    //==============================================================================
    /** Returns the reverb's current parameters. */
    const Parameters& getParameters () const noexcept { return parameters; }

    /** Applies a new set of parameters to the reverb.
        Note that this doesn't attempt to lock the reverb, so if you call this in parallel with
        the process method, you may get artifacts.
    */
    void setParameters (const Parameters& newParams)
    {
        const float wetScaleFactor = 3.0f;
        const float dryScaleFactor = 2.0f;

        const float wet = newParams.wetLevel * wetScaleFactor;
        dryGain.setTargetValue (newParams.dryLevel * dryScaleFactor);
        wetGain1.setTargetValue (0.5f * wet * (1.0f + newParams.width));
        wetGain2.setTargetValue (0.5f * wet * (1.0f - newParams.width));

        gain = isFrozen (newParams.freezeMode) ? 0.0f : 0.015f;
        parameters = newParams;
        updateDamping ();
    }

    //==============================================================================
    /** Sets the sample rate that will be used for the reverb.
        You must call this before the process methods, in order to tell it the correct sample rate.
    */
    void setSampleRate (const double sampleRate)
    {
        jassert (sampleRate > 0);

        static const short combTunings[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // (at 44100Hz)
        static const short allPassTunings[] = { 556, 441, 341, 225 };
        const int stereoSpread = 23;
        const int intSampleRate = (int) sampleRate;

        for (int i = 0; i < numCombs; ++i)
        {
            comb[0][i].setSize ((intSampleRate * combTunings[i]) / 44100);
            comb[1][i].setSize ((intSampleRate * (combTunings[i] + stereoSpread)) / 44100);
        }

        for (int i = 0; i < numAllPasses; ++i)
        {
            allPass[0][i].setSize ((intSampleRate * allPassTunings[i]) / 44100);
            allPass[1][i].setSize ((intSampleRate * (allPassTunings[i] + stereoSpread)) / 44100);
        }

        const double smoothTime = 0.01;
        damping.reset (sampleRate, smoothTime);
        feedback.reset (sampleRate, smoothTime);
        dryGain.reset (sampleRate, smoothTime);
        wetGain1.reset (sampleRate, smoothTime);
        wetGain2.reset (sampleRate, smoothTime);
    }

    /** Clears the reverb's buffers. */
    void reset ()
    {
        for (int j = 0; j < numChannels; ++j)
        {
            for (int i = 0; i < numCombs; ++i)
                comb[j][i].clear ();

            for (int i = 0; i < numAllPasses; ++i)
                allPass[j][i].clear ();
        }
    }

    //==============================================================================
    /** Applies the reverb to two stereo channels of audio data. */
    //template <std::floating_point T>
    void processStereo (T* const left, T* const right, const int numSamples) noexcept
    {
        JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6011)
        jassert (left != nullptr && right != nullptr);

        for (int i = 0; i < numSamples; ++i)
        {
            // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
            const T input = (left[i] + right[i]) * gain;
            T outL = 0, outR = 0;

            const T damp = damping.getNextValue ();
            const T feedbck = feedback.getNextValue ();

            for (int j = 0; j < numCombs; ++j)  // accumulate the comb filters in parallel
            {
                outL += comb[0][j].process (input, damp, feedbck);
                outR += comb[1][j].process (input, damp, feedbck);
            }

            for (int j = 0; j < numAllPasses; ++j)  // run the allpass filters in series
            {
                outL = allPass[0][j].process (outL);
                outR = allPass[1][j].process (outR);
            }

            const T dry = dryGain.getNextValue ();
            const T wet1 = wetGain1.getNextValue ();
            const T wet2 = wetGain2.getNextValue ();

            left[i] = outL * wet1 + outR * wet2 + left[i] * dry;
            right[i] = outR * wet1 + outL * wet2 + right[i] * dry;
        }
        JUCE_END_IGNORE_WARNINGS_MSVC
    }

    /** Applies the reverb to a single mono channel of audio data. */
    template <std::floating_point T>
    void processMono (T* const samples, const int numSamples) noexcept
    {
        JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6011)
            jassert (samples != nullptr);

        for (int i = 0; i < numSamples; ++i)
        {
            const T input = samples[i] * gain;
            T output = 0;

            const T damp = damping.getNextValue ();
            const T feedbck = feedback.getNextValue ();

            for (int j = 0; j < numCombs; ++j)  // accumulate the comb filters in parallel
                output += comb[0][j].process (input, damp, feedbck);

            for (int j = 0; j < numAllPasses; ++j)  // run the allpass filters in series
                output = allPass[0][j].process (output);

            const T dry = dryGain.getNextValue ();
            const T wet1 = wetGain1.getNextValue ();

            samples[i] = output * wet1 + samples[i] * dry;
        }
        JUCE_END_IGNORE_WARNINGS_MSVC
    }

private:
    //==============================================================================
    static bool isFrozen (const float freezeMode) noexcept { return freezeMode >= 0.5f; }

    void updateDamping () noexcept
    {
        const float roomScaleFactor = 0.28f;
        const float roomOffset = 0.7f;
        const float dampScaleFactor = 0.4f;

        if (isFrozen (parameters.freezeMode))
            setDamping (0.0f, 1.0f);
        else
            setDamping (parameters.damping * dampScaleFactor,
                        parameters.roomSize * roomScaleFactor + roomOffset);
    }

    void setDamping (const float dampingToUse, const float roomSizeToUse) noexcept
    {
        damping.setTargetValue (dampingToUse);
        feedback.setTargetValue (roomSizeToUse);
    }

    //==============================================================================
    template <std::floating_point T>
    class CombFilter
    {
    public:
        CombFilter () noexcept {}

        void setSize (const int size)
        {
            if (size != bufferSize)
            {
                bufferIndex = 0;
                buffer.malloc (size);
                bufferSize = size;
            }

            clear ();
        }

        void clear () noexcept
        {
            last = 0;
            buffer.clear ((size_t) bufferSize);
        }

        T process (const T input, const T damp, const T feedbackLevel) noexcept
        {
            const T output = buffer[bufferIndex];
            last = (output * (1.0f - damp)) + (last * damp);
            JUCE_UNDENORMALISE (last);

            T temp = input + (last * feedbackLevel);
            JUCE_UNDENORMALISE (temp);
            buffer[bufferIndex] = temp;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return output;
        }

    private:
        juce::HeapBlock<T> buffer;
        int bufferSize = 0, bufferIndex = 0;
        T last = 0.0f;

        JUCE_DECLARE_NON_COPYABLE (CombFilter)
    };

    //==============================================================================
    template <std::floating_point T>
    class AllPassFilter
    {
    public:
        AllPassFilter () noexcept {}

        void setSize (const int size)
        {
            if (size != bufferSize)
            {
                bufferIndex = 0;
                buffer.malloc (size);
                bufferSize = size;
            }

            clear ();
        }

        void clear () noexcept
        {
            buffer.clear ((size_t) bufferSize);
        }

        T process (const T input) noexcept
        {
            const T bufferedValue = buffer[bufferIndex];
            T temp = input + (bufferedValue * 0.5f);
            JUCE_UNDENORMALISE (temp);
            buffer[bufferIndex] = temp;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return bufferedValue - input;
        }

    private:
        juce::HeapBlock<T> buffer;
        int bufferSize = 0, bufferIndex = 0;

        JUCE_DECLARE_NON_COPYABLE (AllPassFilter)
    };

    //==============================================================================
    enum { numCombs = 8, numAllPasses = 4, numChannels = 2 };

    Parameters parameters;
    float gain;

    CombFilter<T> comb[numChannels][numCombs];
    AllPassFilter<T> allPass[numChannels][numAllPasses];

    juce::SmoothedValue<T> damping, feedback, dryGain, wetGain1, wetGain2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhatReverb)
};

//==========================================================

template <std::floating_point T>
class PhatReverbWrapper
{
public:
    //==============================================================================
    /** Creates an uninitialised Reverb processor. Call prepare() before first use. */
    PhatReverbWrapper () = default;

    //==============================================================================
    using Parameters = PhatReverb<T>::Parameters;

    /** Returns the reverb's current parameters. */
    const Parameters& getParameters () const noexcept { return reverb.getParameters (); }

    /** Applies a new set of parameters to the reverb.
        Note that this doesn't attempt to lock the reverb, so if you call this in parallel with
        the process method, you may get artifacts.
    */
    void setParameters (const Parameters& newParams) { reverb.setParameters (newParams); }

    /** Returns true if the reverb is enabled. */
    bool isEnabled () const noexcept { return enabled; }

    /** Enables/disables the reverb. */
    void setEnabled (bool newValue) noexcept { enabled = newValue; }

    //==============================================================================
    /** Initialises the reverb. */
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        reverb.setSampleRate (spec.sampleRate);
    }

    /** Resets the reverb's internal state. */
    void reset () noexcept
    {
        reverb.reset ();
    }

    //==============================================================================
    /** Applies the reverb to a mono or stereo buffer. */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock ();
        auto& outputBlock = context.getOutputBlock ();
        const auto numInChannels = inputBlock.getNumChannels ();
        const auto numOutChannels = outputBlock.getNumChannels ();
        const auto numSamples = outputBlock.getNumSamples ();

        jassert (inputBlock.getNumSamples () == numSamples);

        outputBlock.copyFrom (inputBlock);

        if (! enabled || context.isBypassed)
            return;

        if (numInChannels == 1 && numOutChannels == 1)
        {
            reverb.processMono (outputBlock.getChannelPointer (0), (int) numSamples);
        }
        else if (numInChannels == 2 && numOutChannels == 2)
        {
            reverb.processStereo (outputBlock.getChannelPointer (0),
                                  outputBlock.getChannelPointer (1),
                                  (int) numSamples);
        }
        else
        {
            jassertfalse;   // invalid channel configuration
        }
    }

private:
    //==============================================================================
    PhatReverb<T> reverb;
    bool enabled = true;
};

//==========================================================

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

    void setMasterGain (float gain) { fxChain.template get<masterGainIndex>().setGainLinear (static_cast<T> (gain)); }

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override;

private:
    void setEffectParam (juce::StringRef parameterID, float newValue);

    void renderVoicesTemplate (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples);
    void renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;
    void renderVoices (juce::AudioBuffer<double>& outputAudio, int startSample, int numSamples) override;

    enum
    {
        reverbIndex = 0,
        masterGainIndex,
    };

    //TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled;

    juce::dsp::ProcessorChain<PhatReverbWrapper<T>, juce::dsp::Gain<T>> fxChain;
    PhatReverbWrapper<T>::Parameters reverbParams
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

template <std::floating_point T>
void ProPhatSynthesiser<T>::renderVoicesTemplate (juce::AudioBuffer<T>& outputAudio, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (outputAudio, startSample, numSamples);

    auto audioBlock { juce::dsp::AudioBlock<T> (outputAudio).getSubBlock((size_t)startSample, (size_t)numSamples) };
    const auto context { juce::dsp::ProcessContextReplacing<T> (audioBlock) };
    fxChain.process (context);
}


template <std::floating_point T>
ProPhatSynthesiser<T>::ProPhatSynthesiser (juce::AudioProcessorValueTreeState& processorState)
: state (processorState)
{
    for (auto i = 0; i < Constants::numVoices; ++i)
        addVoice (new ProPhatVoice<T> (state, i, &voicesBeingKilled));

    addSound (new ProPhatSound ());

    addParamListenersToState ();

    setMasterGain (Constants::defaultMasterGain);
    fxChain.template get<masterGainIndex> ().setRampDurationSeconds (0.1);

    //we need to manually override the default reverb params to make sure 0 values are set if needed
    fxChain.template get<reverbIndex> ().setParameters (reverbParams);
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

    fxChain.prepare (spec);
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
void ProPhatSynthesiser<T>::setEffectParam ([[maybe_unused]] juce::StringRef parameterID, [[maybe_unused]] float newValue)
{
    if (parameterID == ProPhatParameterIds::effectParam1ID.getParamID ())
        reverbParams.roomSize = newValue;
    else if (parameterID == ProPhatParameterIds::effectParam2ID.getParamID ())
        reverbParams.wetLevel = newValue;
    else
        jassertfalse;   //unknown effect parameter!


    fxChain.template get<reverbIndex> ().setParameters (reverbParams);
}

template <std::floating_point T>
void ProPhatSynthesiser<T>::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    {
        //TODO lock in the audio thread??
        const juce::ScopedLock sl (lock);

        //don't start new voices in current buffer call if we have filled all voices already.
        //voicesBeingKilled should be reset after each renderNextBlock call
        if (voicesBeingKilled.size () >= Constants::numVoices)
            return;
    }

    Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}


