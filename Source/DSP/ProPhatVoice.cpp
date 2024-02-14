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

#include "ProPhatVoice.h"
#include "../UI/ButtonGroupComponent.h"
#include "../Utility/Macros.h"

template class ProPhatVoice<float>;
template class ProPhatVoice<double>;

template <std::floating_point T>
ProPhatVoice<T>::ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int vId, std::set<int>* activeVoiceSet)
    : state (processorState)
    , oscillators (state)
    , voiceId (vId)
    , voicesBeingKilled (activeVoiceSet)
{
    addParamListenersToState ();

    processorChain.get<(int)ProcessorId::masterGainIndex>().setGainLinear (static_cast<T> (Constants::defaultOscLevel));
    setFilterCutoffInternal (Constants::defaultFilterCutoff);
    setFilterResonanceInternal (Constants::defaultFilterResonance);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (Constants::defaultLfoFreq);
}

template <std::floating_point T>
void ProPhatVoice<T>::prepare (const juce::dsp::ProcessSpec& spec)
{
    curPreparedSamples = spec.maximumBlockSize;
    oscillators.prepare (spec);

    overlap = std::make_unique<juce::AudioBuffer<T>> (spec.numChannels, Constants::killRampSamples);
    overlap->clear();

    processorChain.prepare (spec);

    ampADSR.setSampleRate (spec.sampleRate);
    ampADSR.setParameters (ampParams);

    filterEnvADSR.setSampleRate (spec.sampleRate);
    filterEnvADSR.setParameters (filterEnvParams);

    lfo.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
}

template <std::floating_point T>
void ProPhatVoice<T>::addParamListenersToState ()
{
    using namespace ProPhatParameterIds;

    //add our synth as listener to all parameters so we can do automations
    state.addParameterListener (filterCutoffID.getParamID (), this);
    state.addParameterListener (filterResonanceID.getParamID (), this);
    state.addParameterListener (filterEnvAttackID.getParamID (), this);
    state.addParameterListener (filterEnvDecayID.getParamID (), this);
    state.addParameterListener (filterEnvSustainID.getParamID (), this);
    state.addParameterListener (filterEnvReleaseID.getParamID (), this);

    state.addParameterListener (ampAttackID.getParamID (), this);
    state.addParameterListener (ampDecayID.getParamID (), this);
    state.addParameterListener (ampSustainID.getParamID (), this);
    state.addParameterListener (ampReleaseID.getParamID (), this);

    state.addParameterListener (lfoShapeID.getParamID (), this);
    state.addParameterListener (lfoDestID.getParamID (), this);
    state.addParameterListener (lfoFreqID.getParamID (), this);
    state.addParameterListener (lfoAmountID.getParamID (), this);
}

template <std::floating_point T>
void ProPhatVoice<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatVoice::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    if (parameterID == ampAttackID.getParamID ()
             || parameterID == ampDecayID.getParamID ()
             || parameterID == ampSustainID.getParamID ()
             || parameterID == ampReleaseID.getParamID ())
        setAmpParam (parameterID, newValue);

    else if (parameterID == filterEnvAttackID.getParamID ()
             || parameterID == filterEnvDecayID.getParamID ()
             || parameterID == filterEnvSustainID.getParamID ()
             || parameterID == filterEnvReleaseID.getParamID ())
        setFilterEnvParam (parameterID, newValue);

    else if (parameterID == lfoShapeID.getParamID ())
        setLfoShape ((int) newValue);
    else if (parameterID == lfoDestID.getParamID ())
        setLfoDest ((int) newValue);
    else if (parameterID == lfoFreqID.getParamID ())
        setLfoFreq (newValue);
    else if (parameterID == lfoAmountID.getParamID ())
        setLfoAmount (newValue);

    else if (parameterID == filterCutoffID.getParamID ())
        setFilterCutoff (newValue);
    else if (parameterID == filterResonanceID.getParamID ())
        setFilterResonance (newValue);

    else
        jassertfalse;
}

template <std::floating_point T>
void ProPhatVoice<T>::setAmpParam (juce::StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == ProPhatParameterIds::ampAttackID.getParamID())
        ampParams.attack = newValue;
    else if (parameterID == ProPhatParameterIds::ampDecayID.getParamID())
        ampParams.decay = newValue;
    else if (parameterID == ProPhatParameterIds::ampSustainID.getParamID())
        ampParams.sustain = newValue;
    else if (parameterID == ProPhatParameterIds::ampReleaseID.getParamID())
        ampParams.release = newValue;

    ampADSR.setParameters (ampParams);
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterEnvParam (juce::StringRef parameterID, float newValue)
{
    if (newValue <= 0)
    {
        jassertfalse;
        newValue = std::numeric_limits<float>::epsilon();
    }

    if (parameterID == ProPhatParameterIds::filterEnvAttackID.getParamID())
        filterEnvParams.attack = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvDecayID.getParamID())
        filterEnvParams.decay = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvSustainID.getParamID())
        filterEnvParams.sustain = newValue;
    else if (parameterID == ProPhatParameterIds::filterEnvReleaseID.getParamID())
        filterEnvParams.release = newValue;

    filterEnvADSR.setParameters (filterEnvParams);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscilate between [-1, 1]
template <std::floating_point T>
void ProPhatVoice<T>::setLfoShape (int shape)
{
    switch (shape)
    {
        case LfoShape::triangle:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](T x) { return (std::sin (x) + 1) / 2; }, 128);
        }
            break;

        case LfoShape::saw:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](T x)
            {
                //this is a sawtooth wave; as x goes from -pi to pi, y goes from -1 to 1
                return juce::jmap (x, -juce::MathConstants<T>::pi, juce::MathConstants<T>::pi, T { 0 }, T { 1 });
            }, 2);
        }
            break;

        //@TODO add this once we have more room in the UI for lfo destinations
        /*
        case LfoShape::revSaw:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](float x)
            {
                return (float) juce::jmap (x, -juce::MathConstants<T>::pi, juce::MathConstants<T>::pi, 1.f, 0.f);
            }, 2);
        }
            break;
        */

        case LfoShape::square:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([](T x)
            {
                if (x < 0)
                    return T { 0 };
                else
                    return T { 1 };
            });
        }
            break;

        case LfoShape::random:
        {
            std::lock_guard<std::mutex> lock (lfoMutex);
            lfo.initialise ([this](T x)
            {
                if (x <= 0.f && valueWasBig)
                {
                    randomValue = rng.nextFloat()/* * 2 - 1*/;
                    valueWasBig = false;
                }
                else if (x > 0.f && ! valueWasBig)
                {
                    randomValue = rng.nextFloat()/* * 2 - 1*/;
                    valueWasBig = true;
                }

                return randomValue;
            });
        }
            break;

        default:
            jassertfalse;
            break;
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::setLfoDest (int dest)
{
    //reset everything
    oscillators.resetLfoOscNoteOffsets ();

    //change the destination
    lfoDest.curSelection = dest;
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterCutoff (float newValue)
{
    curFilterCutoff = newValue;
    setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterTiltCutoff (float newValue)
{
    tiltCutoff = newValue;
    setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterResonance (float newAmount)
{
    curFilterResonance = newAmount;
    setFilterResonanceInternal (curFilterResonance);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscillate between [-1, 1]
template <std::floating_point T>
void ProPhatVoice<T>::updateLfo()
{
    //apply filter envelope
    //TODO make this into a slider
    const auto envelopeAmount = 2;

    T lfoOut;
    {
        //TODO: LOCK IN AUDIO THREAD
        std::lock_guard<std::mutex> lock (lfoMutex);
        lfoOut = lfo.processSample (T (0)) * lfoAmount;
    }

    //TODO get this switch out of here, this is awful for performances
    switch (lfoDest.curSelection)
    {
        case LfoDest::osc1Freq:
            //lfoOsc1NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            //oscillators.updateOscFrequencies();
            oscillators.setLfoOsc1NoteOffset (Constants::lfoNoteRange.convertFrom0to1 (static_cast<float> (lfoOut)));
            break;

        case LfoDest::osc2Freq:
            /*lfoOsc2NoteOffset = lfoNoteRange.convertFrom0to1 (lfoOut);
            oscillators.updateOscFrequencies();*/
            oscillators.setLfoOsc2NoteOffset (Constants::lfoNoteRange.convertFrom0to1 (static_cast<float> (lfoOut)));
            break;

        case LfoDest::filterCutOff:
        {
            const auto lfoCutOffContributionHz { juce::jmap (static_cast<float> (lfoOut), 0.0f, 1.0f, 10.0f, 10000.0f) };
            const auto curCutOff { (curFilterCutoff + tiltCutoff) * (1 + envelopeAmount * filterEnvelope) + lfoCutOffContributionHz };
            setFilterCutoffInternal (curCutOff);
        }
        break;

        case LfoDest::filterResonance:
            setFilterResonanceInternal (curFilterResonance * (1 + envelopeAmount * static_cast<float> (lfoOut)));
            break;

        default:
            break;
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterCutoffInternal (float curCutOff)
{
    const auto limitedCutOff { juce::jlimit (Constants::cutOffRange.start, Constants::cutOffRange.end, curCutOff) };
    processorChain.get<(int) ProcessorId::filterIndex> ().setCutoffFrequencyHz (limitedCutOff);
}

template <std::floating_point T>
void ProPhatVoice<T>::setFilterResonanceInternal (float curResonance)
{
    const auto limitedResonance { juce::jlimit (0.f, 1.f, curResonance) };
    processorChain.get<(int) ProcessorId::filterIndex> ().setResonance (limitedResonance);
}

template <std::floating_point T>
void ProPhatVoice<T>::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG start: " + juce::String (voiceId));
#endif

    ampADSR.setParameters (ampParams);
    ampADSR.reset();
    ampADSR.noteOn();

    filterEnvADSR.setParameters (filterEnvParams);
    filterEnvADSR.reset();
    filterEnvADSR.noteOn();

    oscillators.updateOscFrequencies (midiNoteNumber, velocity, currentPitchWheelPosition);

    rampingUp = true;
    rampUpSamplesLeft = Constants::rampUpSamples;

    oscillators.updateOscLevels();
}

template <std::floating_point T>
void ProPhatVoice<T>::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        currentlyReleasingNote = true;
        ampADSR.noteOff();
        filterEnvADSR.noteOff();

#if DEBUG_VOICES
        DBG ("\tDEBUG tailoff voice: " + juce::String (voiceId));
#endif
    }
    else
    {
        if (getSampleRate() != 0.f && ! justDoneReleaseEnvelope)
        {
            rampingUp = false;

            overlap->clear();
            voicesBeingKilled->insert (voiceId);
            currentlyKillingVoice = true;
            renderNextBlock (*overlap, 0, Constants::killRampSamples);
            overlapIndex = 0;
        }

        justDoneReleaseEnvelope = false;
        clearCurrentNote();

#if DEBUG_VOICES
        DBG ("\tDEBUG kill voice: " + juce::String (voiceId));
#endif
    }
}

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate<float> (outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<float>::renderNextBlock (juce::AudioBuffer<double>&, int, int)
{
    //trying to render doubles with a float voice!
    jassertfalse;
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples)
{
    renderNextBlockTemplate<double>(outputBuffer, startSample, numSamples);
}

template <>
void ProPhatVoice<double>::renderNextBlock (juce::AudioBuffer<float>&, int, int)
{
    //trying to render floats with a double voice!
    jassertfalse;
}

template <std::floating_point T>
void ProPhatVoice<T>::controllerMoved (int controllerNumber, int newValue)
{
    //DBG ("controllerNumber: " + juce::String (controllerNumber) + ", newValue: " + juce::String (newValue));

    //1 == orba tilt. The newValue range [0-127] is converted to [curFilterCutoff, cutOffRange.end]
    if (controllerNumber == 1)
        setFilterTiltCutoff (juce::jmap (static_cast<float> (newValue), 0.f, 127.f, curFilterCutoff, Constants::cutOffRange.end));
}
