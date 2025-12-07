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

#include "PhatOscillators.h"

#include "../UI/ButtonGroupComponent.h"
#include "../Utility/Helpers.h"
#include "../Utility/Macros.h"

#if EFFECTS_PROCESSOR_PER_VOICE
#include "PhatEffectsProcessor.hpp"
#endif

struct ProPhatSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================

/**
 * @brief The main voice for our synthesizer.
*/
enum class ProcessorId
{
    filterIndex = 0,
    masterGainIndex,
};

//==============================================================================

template <std::floating_point T>
class ProPhatVoice : public juce::SynthesiserVoice, public juce::AudioProcessorValueTreeState::Listener
{
  public:
    ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int vId, std::set<int>* activeVoiceSet);

    void addParamListenersToState();
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void releaseResources();

    void setAmpParam (juce::StringRef parameterID, float newValue);
    void setFilterEnvParam (juce::StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq)
    {
        //TODO RT: none of this is atomic / thread safe
        for (auto& lfo : lfos)
            lfo.setFrequency (newFreq);
    }
    void setLfoAmount (float newAmount) { lfoAmount = newAmount; }

    void setFilterCutoff (T newValue)
    {
        curFilterCutoff = newValue;
        setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
    }

    void setFilterTiltCutoff (T newValue)
    {
        tiltCutoff = newValue;
        setFilterCutoffInternal (curFilterCutoff + tiltCutoff);
    }

    void setFilterResonance (T newAmount)
    {
        curFilterResonance = newAmount;
        setFilterResonanceInternal (curFilterResonance);
    }

    void pitchWheelMoved (int newPitchWheelValue) override { oscillators.pitchWheelMoved (newPitchWheelValue); }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<ProPhatSound*> (sound) != nullptr; }

    //Because renderNextBlock is defined as 2 different prototypes we can't just implement a
    //renderNextBlock (juce::AudioBuffer<T>& ...). Instead we need to wrap our templated function
    //inside the specific float and double functions. What's weird though is that the juce::synthesiser
    //class has similar prototypes, but just overriding both with a template works...
    void renderNextBlockTemplate (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    void renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples) override;

    void controllerMoved (int controllerNumber, int newValue) override
    {
        //1 == orba tilt. The newValue range [0-127] is converted to [curFilterCutoff, cutOffRange.end]
        if (controllerNumber == 1)
            setFilterTiltCutoff (juce::jmap (T (newValue), T (0), T (127), T (curFilterCutoff), T (Constants::cutOffRange.end)));
    }

    int getVoiceId() const { return voiceId; }

  private:
    juce::AudioProcessorValueTreeState& state;

    int voiceId;

    T lfoCutOffContributionHz { 0 };

    void setFilterCutoffInternal (T curCutOff)
    {
        const auto limitedCutOff { juce::jlimit (T (Constants::cutOffRange.start), T (Constants::cutOffRange.end), curCutOff) };
        filterAndGainProcessorChain.template get<(int) ProcessorId::filterIndex>().setCutoffFrequencyHz (limitedCutOff);
    }

    void setFilterResonanceInternal (T curResonance)
    {
        const auto limitedResonance { juce::jlimit (T (0), T (1), curResonance) };
        filterAndGainProcessorChain.template get<(int) ProcessorId::filterIndex>().setResonance (limitedResonance);
    }

    /** Calculate LFO values. Called on the audio thread. */
    inline void updateLfo();
    void        processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void        processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void        assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);
    void        applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples);

    PhatOscillators<T> oscillators;

    std::unique_ptr<juce::AudioBuffer<T>> overlap;
    int                                   overlapIndex = -1;
    //TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool           currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<T>, juce::dsp::Gain<T>> filterAndGainProcessorChain;
    //TODO: use a slider for this
    static constexpr auto envelopeAmount { 2 };
#if EFFECTS_PROCESSOR_PER_VOICE
    EffectsProcessor<T> effectsProcessor;
#endif

    juce::ADSR             ampADSR, filterADSR;
    juce::ADSR::Parameters ampParams { Constants::defaultAmpA, Constants::defaultAmpD, Constants::defaultAmpS, Constants::defaultAmpR };
    juce::ADSR::Parameters filterEnvParams { ampParams };
    bool currentlyReleasingNote = false, justDoneReleaseEnvelope = false;   //written and read on audio thread only

    T curFilterCutoff { Constants::defaultFilterCutoff };
    T curFilterResonance { Constants::defaultFilterResonance };

    //lfo stuff
    static constexpr auto    lfoUpdateRate    = 100;
    int                      lfoUpdateCounter = lfoUpdateRate;

    //struct RandomLfoFunc
    //{
    //    float lastPhase = 0.0f;
    //    float lastValue = 0.0f;

    //    float operator()(float phase)
    //    {
    //        // Detect wrap-around (phase goes from 0 → 2π)
    //        if (phase < lastPhase)
    //        {
    //            lastValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    //        }

    //        lastPhase = phase;
    //        return lastValue;
    //    }
    //};


    std::array<juce::dsp::Oscillator<T>, LfoShape::totalSelectable> lfos;
    std::atomic<juce::dsp::Oscillator<T>*> curLfo {nullptr};

    std::mutex lfoMutex;

    //TODO RT: I think this (and all similar parameters set in the UI and read in the audio thread) sould be atomic
    T       lfoAmount = static_cast<T> (Constants::defaultLfoAmount);
    LfoDest lfoDest;

    //for the random lfo
    juce::Random rng;
    T            randomValue = 0.f;
    bool         valueWasBig = false;

    bool rampingUp         = false;
    int  rampUpSamplesLeft = 0;

    T tiltCutoff { 0.f };

    int curPreparedSamples = 0;
};

//===========================================================================================================

template <std::floating_point T>
ProPhatVoice<T>::ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int vId, std::set<int>* activeVoiceSet)
: state (processorState), voiceId (vId), oscillators (state), voicesBeingKilled (activeVoiceSet)
{
    addParamListenersToState();

    filterAndGainProcessorChain.template get<(int) ProcessorId::masterGainIndex>().setGainLinear (static_cast<T> (Constants::defaultOscLevel));

    setFilterCutoffInternal (Constants::defaultFilterCutoff);
    setFilterResonanceInternal (Constants::defaultFilterResonance);

    lfoDest.curSelection = (int) defaultLfoDest;

    lfos[LfoShape::triangle].initialise ([] (T x) { return (std::sin (x) + 1) / 2; }, 128);
    lfos[LfoShape::saw].initialise ([] (T x)      { return juce::jmap (x, -juce::MathConstants<T>::pi, juce::MathConstants<T>::pi, T (0), T (1)); }, 2);
    //lfos[LfoShape::revSaw].initialise ([] (T x)   { return (float) juce::jmap (x, -juce::MathConstants<T>::pi, juce::MathConstants<T>::pi, 1.f, 0.f); }, 2);
    lfos[LfoShape::square].initialise ([] (T x)   { return x < 0 ? T (0) : T (1); });

    // TODO: i really don't think this will be captured correctly in a table. i probably need to run this once and hardcode the table values
    // As it is, it most likely involves non-RT safe system calls too
    lfos[LfoShape::randomLfo].initialise ([this](T x)
                                          {
                                              if (x <= 0.f && valueWasBig)
                                              {
                                                  randomValue = rng.nextFloat ()/* * 2 - 1*/;
                                                  valueWasBig = false;
                                              }
                                              else if (x > 0.f && ! valueWasBig)
                                              {
                                                  randomValue = rng.nextFloat ()/* * 2 - 1*/;
                                                  valueWasBig = true;
                                              }

                                              return randomValue; });

    setLfoShape (LfoShape::triangle);
    for (auto& lfo : lfos)
    {
        jassert (lfo.isInitialised());
        lfo.setFrequency (Constants::defaultLfoFreq);
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::renderNextBlockTemplate (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
{
    if (! currentlyKillingVoice && ! isVoiceActive())
        return;

    //reserve an audio block of size numSamples. Auvaltool has a tendency to _not_ call prepare before rendering
    //with new buffer sizes, so just making sure we're not taking more samples than the audio block was prepared with.
    jassert (numSamples <= curPreparedSamples);
    numSamples = juce::jmin (numSamples, curPreparedSamples);
    auto currentAudioBlock { oscillators.prepareRender (numSamples) };

    for (int pos = 0; pos < numSamples;)
    {
        const auto subBlockSize = juce::jmin (numSamples - pos, lfoUpdateCounter);

        //render the oscillators over the subBlockSize
        juce::dsp::AudioBlock<T> oscBlock { oscillators.process (pos, subBlockSize) };

        //apply filter and gain
        juce::dsp::ProcessContextReplacing<T> oscContext (oscBlock);
        filterAndGainProcessorChain.process (oscContext);

#if EFFECTS_PROCESSOR_PER_VOICE
        effectsProcessor.process (oscContext);
#endif

        //apply the envelopes. We calculate and apply the amp envelope on a sample basis,
        //but for the filter env we increment it on a sample basis but only apply it
        //once per buffer, just like the LFO -- see below.
        auto filterEnvelope { 0.f };
        {
            const auto numChannels { oscBlock.getNumChannels() };
            for (auto i = 0; i < subBlockSize; ++i)
            {
                //TODO: we only need the last of these right, not sure this needs to be in this loop
                //calculate and store filter envelope
                filterEnvelope = filterADSR.getNextSample();

                //TODO: if there's an efficient way to render the ampEnv here we could use SIMD for the multiplication below
                //calculate and apply amp envelope
                const auto ampEnv = ampADSR.getNextSample();
                for (size_t c = 0; c < numChannels; ++c)
                    oscBlock.getChannelPointer (c)[i] *= ampEnv;
            }

            if (currentlyReleasingNote && ! ampADSR.isActive())
            {
                currentlyReleasingNote  = false;
                justDoneReleaseEnvelope = true;
                stopNote (0.f, false);
            }
        }

        if (rampingUp)
            processRampUp (oscBlock, (int) subBlockSize);

        //for now this is only happening when we run out of voices
        //overlapIndex will be >= 0 if we're in the process of adding a kill overlap buffer to the oscBlock
        if (overlapIndex > -1)
            processKillOverlap (oscBlock, (int) subBlockSize);

        //update our lfos at the end of the block
        lfoUpdateCounter -= subBlockSize;
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo();
        }

        //apply our filter envelope once per buffer
        const auto curCutOff { (curFilterCutoff + tiltCutoff) * (1 + envelopeAmount * filterEnvelope) + lfoCutOffContributionHz };
        setFilterCutoffInternal (curCutOff);

        //increment our position
        pos += subBlockSize;
    }

    //add everything to the output buffer
    juce::dsp::AudioBlock<T> (outputBuffer).getSubBlock ((size_t) startSample, (size_t) numSamples).add (currentAudioBlock);

    if (currentlyKillingVoice)
        applyKillRamp (outputBuffer, startSample, numSamples);
#if DEBUG_VOICES
    // else
    //     assertForDiscontinuities (outputBuffer, startSample, numSamples, {});
#endif
}

template <std::floating_point T>
void ProPhatVoice<T>::prepare (const juce::dsp::ProcessSpec& spec)
{
    curPreparedSamples = (int) spec.maximumBlockSize;
    oscillators.prepare (spec);

    overlap = std::make_unique<juce::AudioBuffer<T>> (spec.numChannels, Constants::killRampSamples);
    overlap->clear();

    filterAndGainProcessorChain.prepare (spec);

    ampADSR.setSampleRate (spec.sampleRate);
    ampADSR.setParameters (ampParams);
    //Helpers::printADSR ("prepare", ampADSR.getParameters());

    filterADSR.setSampleRate (spec.sampleRate);
    filterADSR.setParameters (filterEnvParams);

    for (auto& lfo : lfos)
        lfo.prepare ({ spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels });

#if EFFECTS_PROCESSOR_PER_VOICE
    effectsProcessor.prepare (spec);
#endif
}

template <std::floating_point T>
void ProPhatVoice<T>::releaseResources()
{
}

template <std::floating_point T>
void ProPhatVoice<T>::addParamListenersToState()
{
    using namespace ProPhatParameterIds;

    //add our synth as listener to all parameters so we can do automations
    state.addParameterListener (filterCutoffID.getParamID(), this);
    state.addParameterListener (filterResonanceID.getParamID(), this);
    state.addParameterListener (filterEnvAttackID.getParamID(), this);
    state.addParameterListener (filterEnvDecayID.getParamID(), this);
    state.addParameterListener (filterEnvSustainID.getParamID(), this);
    state.addParameterListener (filterEnvReleaseID.getParamID(), this);

    state.addParameterListener (ampAttackID.getParamID(), this);
    state.addParameterListener (ampDecayID.getParamID(), this);
    state.addParameterListener (ampSustainID.getParamID(), this);
    state.addParameterListener (ampReleaseID.getParamID(), this);

    state.addParameterListener (lfoShapeID.getParamID(), this);
    state.addParameterListener (lfoDestID.getParamID(), this);
    state.addParameterListener (lfoFreqID.getParamID(), this);
    state.addParameterListener (lfoAmountID.getParamID(), this);

#if EFFECTS_PROCESSOR_PER_VOICE
    state.addParameterListener (reverbParam1ID.getParamID(), this);
    state.addParameterListener (reverbParam2ID.getParamID(), this);

    state.addParameterListener (chorusParam1ID.getParamID(), this);
    state.addParameterListener (chorusParam2ID.getParamID(), this);

    state.addParameterListener (phaserParam1ID.getParamID(), this);
    state.addParameterListener (phaserParam2ID.getParamID(), this);

    state.addParameterListener (effectSelectedID.getParamID(), this);
#endif
}

template <std::floating_point T>
void ProPhatVoice<T>::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace ProPhatParameterIds;

    //DBG ("ProPhatVoice::parameterChanged (" + parameterID + ", " + juce::String (newValue));

    //TODO RT: I _think_ it's fine to just set the envelopes directly but I should make sure it's true
    if (parameterID == ampAttackID.getParamID()
        || parameterID == ampDecayID.getParamID()
        || parameterID == ampSustainID.getParamID()
        || parameterID == ampReleaseID.getParamID())
        setAmpParam (parameterID, newValue);

    else if (parameterID == filterEnvAttackID.getParamID()
             || parameterID == filterEnvDecayID.getParamID()
             || parameterID == filterEnvSustainID.getParamID()
             || parameterID == filterEnvReleaseID.getParamID())
        setFilterEnvParam (parameterID, newValue);

    else if (parameterID == lfoShapeID.getParamID())
        setLfoShape ((int) newValue);
    else if (parameterID == lfoDestID.getParamID())
        setLfoDest ((int) newValue);

    //TODO RT: need to go through these and figure out what needs to be atomic, set async on the audio thread, or ramped or whatever it is
    else if (parameterID == lfoAmountID.getParamID())
        setLfoAmount (newValue);
    else if (parameterID == lfoFreqID.getParamID())
        setLfoFreq (newValue);
    else if (parameterID == filterCutoffID.getParamID())
        setFilterCutoff (newValue);
    else if (parameterID == filterResonanceID.getParamID())
        setFilterResonance (newValue);

#if EFFECTS_PROCESSOR_PER_VOICE
    else if (parameterID == reverbParam1ID.getParamID() || parameterID == reverbParam2ID.getParamID()
             || parameterID == chorusParam1ID.getParamID() || parameterID == chorusParam2ID.getParamID()
             || parameterID == phaserParam1ID.getParamID() || parameterID == phaserParam2ID.getParamID())
        effectsProcessor.setEffectParam (parameterID, newValue);
    else if (parameterID == effectSelectedID.getParamID())
    {
        EffectType effect { EffectType::none };
        //TODO: switch?
        const auto newInt { static_cast<int> (newValue) };
        if (newInt == 0)
            effect = EffectType::none;
        else if (newInt == 1)
            effect = EffectType::verb;
        else if (newInt == 2)
            effect = EffectType::chorus;
        else if (newInt == 3)
            effect = EffectType::phaser;
        else
            jassertfalse;

        if (isVoiceActive())
            effectsProcessor.changeEffect (effect);
    }
#endif

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

    filterADSR.setParameters (filterEnvParams);
}

//@TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscilate between [-1, 1]
template <std::floating_point T>
void ProPhatVoice<T>::setLfoShape (int shape)
{
    //TODO: this somehow made things sound worse when looping in reaper
    //reset everything
    //oscillators.resetLfoOscNoteOffsets();

    //TODO: shape is somehow always 0 with reaper automations??
    //change the shape
    switch (shape)
    {
        case LfoShape::triangle:    curLfo.store (&lfos[LfoShape::triangle]); break;
        case LfoShape::saw:         curLfo.store (&lfos[LfoShape::saw]); break;
        //case LfoShape::revSaw:    curLfo.store (&lfos[LfoShape::revSaw]); break;
        case LfoShape::square:      curLfo.store (&lfos[LfoShape::square]); break;
        case LfoShape::randomLfo:   curLfo.store (&lfos[LfoShape::randomLfo]); break;
        default: jassertfalse;
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::setLfoDest (int dest)
{
    //reset everything
    oscillators.resetLfoOscNoteOffsets();

    //change the destination
    lfoDest.curSelection = dest;
}

//TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscillate between [-1, 1]
template <std::floating_point T>
void ProPhatVoice<T>::updateLfo()
{
    //TODO: so err, wat? we're only using the first sample of the lfo at every buffer size? So the lfo speed is tied to the buffer size?
    const auto lfoOut { curLfo.load()->processSample (static_cast<T> (0)) * lfoAmount };

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
            lfoCutOffContributionHz = juce::jmap (lfoOut, T (0), T (1), T (10), T (10000));
            break;

        case LfoDest::filterResonance:
            setFilterResonanceInternal (curFilterResonance * (1 + envelopeAmount * static_cast<float> (lfoOut)));
            break;

        default:
            break;
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ProPhatVoice::startNote() with voiceId : " + juce::String (voiceId));
#endif

    ampADSR.setParameters (ampParams);
    ampADSR.reset();
    ampADSR.noteOn();

    filterADSR.setParameters (filterEnvParams);
    filterADSR.reset();
    filterADSR.noteOn();

    oscillators.updateOscFrequencies (midiNoteNumber, velocity, currentPitchWheelPosition);

    rampingUp         = true;
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
        filterADSR.noteOff();

#if DEBUG_VOICES
        DBG ("\tDEBUG ProPhatVoice<T>::stopNote with tailoff for voiceId: " << juce::String (voiceId));
#endif
    }
    else
    {
        //if we have a valid sample rate and we haven't had the chance to apply the release envelope
        if (! juce::approximatelyEqual (getSampleRate(), 0.) && ! justDoneReleaseEnvelope)
        {
#if DEBUG_VOICES
            DBG ("\tProPhatVoice<T>::stopNote() starting to kill voice: " + juce::String (voiceId));
#endif
            rampingUp = false;

            //get ready to kill the voice
            overlap->clear();
            voicesBeingKilled->insert (voiceId);
            currentlyKillingVoice = true;

            //render the voice kill into the overlap buffer
            renderNextBlockTemplate (*overlap, 0, Constants::killRampSamples);
            //this index is how we keep track of progress in applying the overlap buffer to the next output buffer
            overlapIndex = 0;
        }

        justDoneReleaseEnvelope = false;
        clearCurrentNote();

#if DEBUG_VOICES
        DBG ("\tDEBUG kill voice: " + juce::String (voiceId));
#endif
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ProPhatVoice<T>::processRampUp() " + juce::String (Constants::rampUpSamples - rampUpSamplesLeft));
#endif
    const auto curRampUpLenght = juce::jmin (curBlockSize, rampUpSamplesLeft);
    const auto prevRampUpValue = static_cast<T> ((Constants::rampUpSamples - rampUpSamplesLeft)) / static_cast<T> (Constants::rampUpSamples);
    const auto nextRampUpValue = static_cast<T> (prevRampUpValue + curRampUpLenght / Constants::rampUpSamples);
    const auto incr            = static_cast<T> ((nextRampUpValue - prevRampUpValue) / static_cast<T> (curRampUpLenght));

    jassert (nextRampUpValue >= T (0) && nextRampUpValue <= T (1.0001));

    const auto numChannels { static_cast<int> (block.getNumChannels()) };
    for (int c = 0; c < numChannels; ++c)
    {
        for (int i = 0; i < curRampUpLenght; ++i)
        {
            const auto value = block.getSample (c, i);
            const auto ramp  = prevRampUpValue + static_cast<T> (i) * incr;
            block.setSample (c, i, value * ramp);
        }
    }

    rampUpSamplesLeft -= curRampUpLenght;

    if (rampUpSamplesLeft <= 0)
    {
        rampingUp = false;
#if DEBUG_VOICES
        DBG ("\tDEBUG ProPhatVoice<T>::processRampUp() DONE");
#endif
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ProPhatVoice::processKillOverlap() " + juce::String (overlapIndex));
#endif

    [[maybe_unused]] const T min { -1 };
    [[maybe_unused]] const T max { 1 };

    auto curSamples = juce::jmin (Constants::killRampSamples - overlapIndex, (int) curBlockSize);

    const auto numChannels { static_cast<int> (block.getNumChannels()) };
    for (int c = 0; c < numChannels; ++c)
    {
        for (int i = 0; i < curSamples; ++i)
        {
            const T prev { block.getSample (c, i) };
            const T overl { overlap->getSample (c, overlapIndex + i) };
            const T total { prev + overl };
            jassert (total >= min && total <= max);

            block.setSample (c, i, total);

#if PRINT_ALL_SAMPLES
            if (c == 0)
                DBG ("\tADD\t" + juce::String (prev) + "\t" + juce::String (overl) + "\t" + juce::String (total));
#endif
        }
    }

    overlapIndex += curSamples;

    if (overlapIndex >= Constants::killRampSamples)
    {
        overlapIndex = -1;
        voicesBeingKilled->erase (voiceId);
#if DEBUG_VOICES
        DBG ("\tDEBUG ProPhatVoice::processKillOverlap() DONE");
#endif
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ProPhatVoice<T>::applyKillRamp on samples " << juce::String (startSample) << "-" << juce::String (startSample + numSamples));
#endif
    outputBuffer.applyGainRamp (startSample, numSamples, 1.f, 0.f);
    currentlyKillingVoice = false;

#if DEBUG_VOICES
    assertForDiscontinuities (outputBuffer, startSample, numSamples, "\tBUILDING KILLRAMP\t");
    DBG ("\tDEBUG ProPhatVoice<T>::applyKillRamp done");
#endif
}

template <std::floating_point T>
void ProPhatVoice<T>::assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, [[maybe_unused]] juce::String dbgPrefix)
{
    if (startSample == outputBuffer.getNumSamples() - 1)
        return;

    auto prev     = outputBuffer.getSample (0, startSample);
    auto prevDiff = abs (outputBuffer.getSample (0, startSample + 1) - prev);

    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
    {
        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            //@TODO need some kind of compression to avoid values above 1.f...
            jassert (abs (outputBuffer.getSample (c, i)) < 1.5f);

            if (c == 0)
            {
#if PRINT_ALL_SAMPLES
                DBG (dbgPrefix + juce::String (outputBuffer.getSample (0, i)));
#endif
                auto cur = outputBuffer.getSample (0, i);
                jassert (abs (cur - prev) < .2f);

                auto curDiff = abs (cur - prev);
                jassert (curDiff - prevDiff < .2f);

                prev     = cur;
                prevDiff = curDiff;
            }
        }
    }
}
