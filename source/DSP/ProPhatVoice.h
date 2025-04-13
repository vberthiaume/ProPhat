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

struct ProPhatSound : public juce::SynthesiserSound
{
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================

/**
 * @brief The main voice for our synthesizer.
*/
template <std::floating_point T>
class ProPhatVoice : public juce::SynthesiserVoice
                   , public juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class ProcessorId
    {
        filterIndex = 0,
        masterGainIndex,
    };

    ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int voiceId, std::set<int>* activeVoiceSet);

    void addParamListenersToState ();
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);

    void setAmpParam (juce::StringRef parameterID, float newValue);
    void setFilterEnvParam (juce::StringRef parameterID, float newValue);

    void setLfoShape (int shape);
    void setLfoDest (int dest);
    void setLfoFreq (float newFreq) { lfo.setFrequency (newFreq); }
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

    int getVoiceId() { return voiceId; }

private:
    juce::AudioProcessorValueTreeState& state;

    int voiceId;

    T lfoCutOffContributionHz { 0 };

    void setFilterCutoffInternal (T curCutOff)
    {
        const auto limitedCutOff { juce::jlimit (T (Constants::cutOffRange.start), T (Constants::cutOffRange.end), curCutOff) };
        processorChain.template get<(int) ProcessorId::filterIndex> ().setCutoffFrequencyHz (limitedCutOff);
    }

    void setFilterResonanceInternal (T curResonance)
    {
        const auto limitedResonance { juce::jlimit (T (0), T (1), curResonance) };
        processorChain.template get<(int) ProcessorId::filterIndex> ().setResonance (limitedResonance);
    }

    /** Calculate LFO values. Called on the audio thread. */
    inline void updateLfo();
    void processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize);
    void assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples, juce::String dbgPrefix);
    void applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples);

    PhatOscillators<T> oscillators;

    std::unique_ptr<juce::AudioBuffer<T>> overlap;
    int overlapIndex = -1;
    //TODO replace this currentlyKillingVoice bool with a check in the bitfield that voicesBeingKilled will become
    bool currentlyKillingVoice = false;
    std::set<int>* voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<T>, juce::dsp::Gain<T>> processorChain;
    //TODO: use a slider for this
    static constexpr auto envelopeAmount { 2 };

    juce::ADSR ampADSR, filterADSR;
    juce::ADSR::Parameters ampParams { Constants::defaultAmpA, Constants::defaultAmpD, Constants::defaultAmpS, Constants::defaultAmpR };
    juce::ADSR::Parameters filterEnvParams { ampParams };
    bool currentlyReleasingNote = false, justDoneReleaseEnvelope = false;

    T curFilterCutoff { Constants::defaultFilterCutoff };
    T curFilterResonance { Constants::defaultFilterResonance };

    //lfo stuff
    static constexpr auto lfoUpdateRate = 100;
    int lfoUpdateCounter = lfoUpdateRate;
    juce::dsp::Oscillator<T> lfo;

    /** TODO RT: implement this pattern for all things that need to be try-locked. Can I abstract/wrap this into an object?
        class WavetableSynthesizer
        {
        public:
            void audioCallback()
            {
                if (std::unique_lock<spin_lock> tryLock (mutex, std::try_to_lock); tryLock.owns_lock())
                {
                    // Do something with wavetable
                }
                else
                {
                    // Do something else as wavetable is not available
                }
            }

            void updateWavetable ()
            {
                // Create new Wavetable
                auto newWavetable = std::make_unique<Wavetable>();
                {
                    std::lock_guard<spin_lock> lock (mutex);
                    std::swap (wavetable, newWavetable);
                }

                // Delete old wavetable here to lock for least time possible
            }

        private:
            spin_lock mutex;
            std::unique_ptr<Wavetable> wavetable;
        };
        }
    */
    std::mutex lfoMutex;
    T lfoAmount = static_cast<T> (Constants::defaultLfoAmount);
    LfoDest lfoDest;

    //for the random lfo
    juce::Random rng;
    T randomValue = 0.f;
    bool valueWasBig = false;

    bool rampingUp = false;
    int rampUpSamplesLeft = 0;

    T tiltCutoff { 0.f };

    int curPreparedSamples = 0;
};

//===========================================================================================================

template<std::floating_point T>
void ProPhatVoice<T>::renderNextBlockTemplate (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
{
    if (! currentlyKillingVoice && ! isVoiceActive ())
        return;

    //reserve an audio block of size numSamples. Auvaltool has a tendency to _not_ call prepare before rendering
    //with new buffer sizes, so just making sure we're not taking more samples than the audio block was prepared with.
    numSamples = juce::jmin (numSamples, curPreparedSamples);
    auto currentAudioBlock { oscillators.prepareRender (numSamples) };

    for (int pos = 0; pos < numSamples;)
    {
        const auto subBlockSize = juce::jmin (numSamples - pos, lfoUpdateCounter);

        //render the oscillators
        auto oscBlock { oscillators.process (pos, subBlockSize) };

        //render our effects
        juce::dsp::ProcessContextReplacing<T> oscContext (oscBlock);
        processorChain.process (oscContext);

        //apply the enveloppes. We calculate and apply the amp envelope on a sample basis,
        //but for the filter env we increment it on a sample basis but only apply it
        //once per buffer, just like the LFO -- see below.
        auto filterEnvelope { 0.f };
        {
            const auto numChannels { oscBlock.getNumChannels () };
            for (auto i = 0; i < subBlockSize; ++i)
            {
                //calculate and atore filter envelope
                filterEnvelope = filterADSR.getNextSample ();

                //calculate and apply amp envelope
                const auto ampEnv = ampADSR.getNextSample ();
                for (size_t c = 0; c < numChannels; ++c)
                    oscBlock.getChannelPointer (c)[i] *= ampEnv;
            }

            if (currentlyReleasingNote && ! ampADSR.isActive ())
            {
                currentlyReleasingNote = false;
                justDoneReleaseEnvelope = true;
                stopNote (0.f, false);

#if DEBUG_VOICES
                DBG ("\tDEBUG ENVELOPPE DONE");
#endif
            }
        }

        if (rampingUp)
            processRampUp (oscBlock, (int) subBlockSize);

        if (overlapIndex > -1)
            processKillOverlap (oscBlock, (int) subBlockSize);

        //update our lfos at the end of the block
        lfoUpdateCounter -= subBlockSize;
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;
            updateLfo ();
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
    else
        assertForDiscontinuities (outputBuffer, startSample, numSamples, {});
#endif
}

template <std::floating_point T>
ProPhatVoice<T>::ProPhatVoice (juce::AudioProcessorValueTreeState& processorState, int vId, std::set<int>* activeVoiceSet)
: state (processorState)
, voiceId (vId)
, oscillators (state)
, voicesBeingKilled (activeVoiceSet)
{
    addParamListenersToState ();

    processorChain.template get<(int)ProcessorId::masterGainIndex>().setGainLinear (static_cast<T> (Constants::defaultOscLevel));

    setFilterCutoffInternal (Constants::defaultFilterCutoff);
    setFilterResonanceInternal (Constants::defaultFilterResonance);

    lfoDest.curSelection = (int) defaultLfoDest;

    setLfoShape (LfoShape::triangle);
    lfo.setFrequency (Constants::defaultLfoFreq);
}

template <std::floating_point T>
void ProPhatVoice<T>::prepare (const juce::dsp::ProcessSpec& spec)
{
    curPreparedSamples = (int) spec.maximumBlockSize;
    oscillators.prepare (spec);

    overlap = std::make_unique<juce::AudioBuffer<T>> (spec.numChannels, Constants::killRampSamples);
    overlap->clear();

    processorChain.prepare (spec);

    ampADSR.setSampleRate (spec.sampleRate);
    ampADSR.setParameters (ampParams);
    //Helpers::printADSR ("prepare", ampADSR.getParameters());

    filterADSR.setSampleRate (spec.sampleRate);
    filterADSR.setParameters (filterEnvParams);

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

    //TODO RT: I _think_ it's fine to just set the envelopes directly but I should make sure it's true
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

    //TODO RT: need to use try_locks for both of these
    else if (parameterID == lfoShapeID.getParamID ())
        setLfoShape ((int) newValue);
    else if (parameterID == lfoDestID.getParamID ())
        setLfoDest ((int) newValue);  

    //TODO RT: I think because all of these end up setting smoothed values, it's fine to set them directly
    else if (parameterID == lfoAmountID.getParamID())
        setLfoAmount (newValue);
    else if (parameterID == lfoFreqID.getParamID())
        setLfoFreq (newValue);
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

    filterADSR.setParameters (filterEnvParams);
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

            //TODO add this once we have more room in the UI for lfo destinations
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
    oscillators.resetLfoOscNoteOffsets();

    //TODO RT: this should also be behind a lock/try_lock
    //change the destination
    lfoDest.curSelection = dest;
}

//TODO For now, all lfos oscillate between [0, 1], even though the random one (and only that one) should oscillate between [-1, 1]
template <std::floating_point T>
void ProPhatVoice<T>::updateLfo()
{
    T lfoOut;
    {
        std::unique_lock<std::mutex> tryLock (lfoMutex, std::defer_lock);
        if (! tryLock.try_lock())
        {
            //TODO RT: I need to fade out or something when we can't acquire the lock
            return;
        }

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
            lfoCutOffContributionHz  = juce::jmap (lfoOut, T (0), T (1), T (10), T (10000));
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
    DBG ("\tDEBUG start: " + juce::String (voiceId));
#endif

    ampADSR.setParameters (ampParams);
    ampADSR.reset();
    ampADSR.noteOn();

    filterADSR.setParameters (filterEnvParams);
    filterADSR.reset();
    filterADSR.noteOn();

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
        filterADSR.noteOff();

#if DEBUG_VOICES
        DBG ("\tDEBUG tailoff voice: " + juce::String (voiceId));
#endif
    }
    else
    {
        if (! juce::approximatelyEqual (getSampleRate(), 0.) && ! justDoneReleaseEnvelope)
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

template <std::floating_point T>
void ProPhatVoice<T>::processRampUp (juce::dsp::AudioBlock<T>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG RAMP UP " + juce::String (rampUpSamples - rampUpSamplesLeft));
#endif
    const auto curRampUpLenght = juce::jmin (curBlockSize, rampUpSamplesLeft);
    const auto prevRampUpValue = static_cast<T> ((Constants::rampUpSamples - rampUpSamplesLeft) / Constants::rampUpSamples);
    const auto nextRampUpValue = static_cast<T> (prevRampUpValue + curRampUpLenght / Constants::rampUpSamples);
    const auto incr            = static_cast<T> ((nextRampUpValue - prevRampUpValue) / curRampUpLenght);

    jassert (nextRampUpValue >= T(0) && nextRampUpValue <= T(1.0001));

    for (size_t c = 0; c < block.getNumChannels (); ++c)
    {
        for (int i = 0; i < curRampUpLenght; ++i)
        {
            auto value = block.getSample (c, i);
            auto ramp = prevRampUpValue + i * incr;
            block.setSample (c, i, value * ramp);
        }
    }

    rampUpSamplesLeft -= curRampUpLenght;

    if (rampUpSamplesLeft <= 0)
    {
        rampingUp = false;
#if DEBUG_VOICES
        DBG ("\tDEBUG RAMP UP DONE");
#endif
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::processKillOverlap (juce::dsp::AudioBlock<T>& block, int curBlockSize)
{
#if DEBUG_VOICES
    DBG ("\tDEBUG ADD OVERLAP" + juce::String (overlapIndex));
#endif

    const T min { -1 };
    const T max { 1 };

    auto curSamples = juce::jmin (Constants::killRampSamples - overlapIndex, (int) curBlockSize);

    for (size_t c = 0; c < block.getNumChannels (); ++c)
    {
        for (int i = 0; i < curSamples; ++i)
        {
            T prev  { block.getSample (c, i)};
            T overl { overlap->getSample (c, overlapIndex + i)};
            T total { juce::jlimit (min, max, prev + overl) };

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
        DBG ("\tDEBUG OVERLAP DONE");
#endif
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::assertForDiscontinuities (juce::AudioBuffer<T>& outputBuffer, int startSample,
                                                int numSamples, [[maybe_unused]]juce::String dbgPrefix)
{
    auto prev = outputBuffer.getSample (0, startSample);
    auto prevDiff = abs (outputBuffer.getSample (0, startSample + 1) - prev);

    for (int c = 0; c < outputBuffer.getNumChannels (); ++c)
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
                jassert (curDiff - prevDiff < .08f);

                prev = cur;
                prevDiff = curDiff;
            }
        }
    }
}

template <std::floating_point T>
void ProPhatVoice<T>::applyKillRamp (juce::AudioBuffer<T>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.applyGainRamp (startSample, numSamples, 1.f, 0.f);
    currentlyKillingVoice = false;

#if DEBUG_VOICES
    DBG ("\tDEBUG START KILLRAMP");
    assertForDiscontinuities (outputBuffer, startSample, numSamples, "\tBUILDING KILLRAMP\t");
    DBG ("\tDEBUG stop KILLRAMP");
#endif
}
