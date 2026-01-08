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

#include "ProPhatProcessor.h"
#include "../UI/ProPhatEditor.h"

#define SIMD_BRO JUCE_WINDOWS
#if SIMD_BRO

#include <vector>
#include <iterator> // for std::ostream_iterator
#include <algorithm> // for std::ranges::copy depending on lib support

//this is only available on intel machines
#include <immintrin.h>

//when we

using Vector = std::vector<float>;
Vector simdAdd (const Vector& a, const Vector& b)
{
    jassert (a.size() == b.size());
    Vector resultVector (a.size());

    //there's 8 floats that fit in an avx register (256 / 32 = 8)
    constexpr auto FLOATS_IN_AVX_REGISTER = 8u;

    //Calculate the actual number of float samples we can vectorize. We're dividing and multiplying by the same value,
    //so this is using int rounding. The leftover samples from the buffer will have to be dealt with separately
    const auto vectorizableSamples = (a.size() / FLOATS_IN_AVX_REGISTER) * FLOATS_IN_AVX_REGISTER;

    //perform the simd addition
    auto i = 0u;
    for (; i < vectorizableSamples; i += FLOATS_IN_AVX_REGISTER)    //here we're looping in chunks of whatever the registers can take
    {
        // load data from the buffer to the simd registers
        // the ai suggested this
        // auto aRegister = _mm256_loadu_ps (&a[i]);
        // auto bRegister = _mm256_loadu_ps (&b[i]);

        // but he's got this instead, which I'm sure is equivalent but I have no way of testing for now
        // The 256 means we're using 256 bit registers,
        // u is for unaligned access, because we assume our data has unaligned data
        // ps is for single precision (float and not double)
        auto aRegister = _mm256_loadu_ps (a.data() + i);
        auto bRegister = _mm256_loadu_ps (b.data() + i);

        // add the registers
        auto intermediateSum = _mm256_add_ps (aRegister, bRegister);

        // store the result back to the buffer
        _mm256_storeu_ps (resultVector.data() + i, intermediateSum);
    }
    
    //then deal with the samples that we couldn't vectorize
    for (; i < a.size(); ++i)
        resultVector[i] = a[i] + b[i];

    return resultVector;
}

#endif

ProPhatProcessor::ProPhatProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , state { constructState () }
    , proPhatSynthFloat (state)
    , proPhatSynthDouble (state)
#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
#if SIMD_BRO
    Vector a(17, 1.f);
    auto result = simdAdd (a, a);
    //print the result vector using ranges
    std::ranges::copy(result, std::ostream_iterator<char>(std::cout, " "));
#endif
}

void ProPhatProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (isUsingDoublePrecision())
        proPhatSynthDouble.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    else
        proPhatSynthFloat.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
}

void ProPhatProcessor::releaseResources()
{
    if (isUsingDoublePrecision())
        proPhatSynthDouble.releaseResources();
    else
        proPhatSynthFloat.releaseResources();
}

juce::AudioProcessorValueTreeState ProPhatProcessor::constructState ()
{
    using namespace Constants;
    using namespace ProPhatParameterIds;
    using namespace ProPhatAudioProcessorChoices;

    //TODO: add undo manager!
    return { *this, nullptr, "state",
    {
        std::make_unique<juce::AudioParameterInt>    (osc1FreqID, osc1FreqID.getParamID (), midiNoteRange.getRange ().getStart (), midiNoteRange.getRange ().getEnd (), defaultOscMidiNote),
        std::make_unique<juce::AudioParameterInt>    (osc2FreqID, osc2FreqID.getParamID (), midiNoteRange.getRange ().getStart (), midiNoteRange.getRange ().getEnd (), defaultOscMidiNote),

        std::make_unique<juce::AudioParameterFloat>  (osc1TuningID, osc1TuningID.getParamID (), tuningSliderRange, (float) defaultOscTuning),
        std::make_unique<juce::AudioParameterFloat>  (osc2TuningID, osc2TuningID.getParamID (), tuningSliderRange, (float) defaultOscTuning),

        std::make_unique<juce::AudioParameterFloat>  (oscSubID, oscSubID.getParamID (), sliderRange, (float) defaultSubOsc),
        std::make_unique<juce::AudioParameterFloat>  (oscMixID, oscMixID.getParamID (), sliderRange, (float) defaultOscMix),
        std::make_unique<juce::AudioParameterFloat>  (oscNoiseID, oscNoiseID.getParamID (), sliderRange, (float) defaultOscNoise),
        std::make_unique<juce::AudioParameterFloat>  (oscSlopID, oscSlopID.getParamID (), slopSliderRange, (float) defaultOscSlop),

        std::make_unique<juce::AudioParameterChoice> (osc1ShapeID, osc1ShapeID.getParamID (), juce::StringArray { oscShape0, oscShape1, oscShape2, oscShape3, oscShape4 }, defaultOscShape),
        std::make_unique<juce::AudioParameterChoice> (osc2ShapeID, osc2ShapeID.getParamID (), juce::StringArray { oscShape0, oscShape1, oscShape2, oscShape3, oscShape4 }, defaultOscShape),

        std::make_unique<juce::AudioParameterFloat>  (filterCutoffID, filterCutoffID.getParamID (), cutOffRange, defaultFilterCutoff),
        std::make_unique<juce::AudioParameterFloat>  (filterResonanceID, filterResonanceID.getParamID (), sliderRange, defaultFilterResonance),

        std::make_unique<juce::AudioParameterFloat>  (ampAttackID, ampAttackID.getParamID (), attackRange, defaultAmpA),
        std::make_unique<juce::AudioParameterFloat>  (ampDecayID, ampDecayID.getParamID (), decayRange, defaultAmpD),
        std::make_unique<juce::AudioParameterFloat>  (ampSustainID, ampSustainID.getParamID (), sustainRange, defaultAmpS),
        std::make_unique<juce::AudioParameterFloat>  (ampReleaseID, ampReleaseID.getParamID (), releaseRange, defaultAmpR),

        std::make_unique<juce::AudioParameterFloat>  (filterEnvAttackID, filterEnvAttackID.getParamID (), attackRange, defaultAmpA),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvDecayID, filterEnvDecayID.getParamID (), decayRange, defaultAmpD),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvSustainID, filterEnvSustainID.getParamID (), sustainRange, defaultAmpS),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvReleaseID, filterEnvReleaseID.getParamID (), releaseRange, defaultAmpR),

        std::make_unique<juce::AudioParameterFloat>  (lfoFreqID, lfoFreqID.getParamID (), lfoRange, defaultLfoFreq),
        std::make_unique<juce::AudioParameterChoice> (lfoShapeID, lfoShapeID.getParamID (), juce::StringArray { lfoShape0, lfoShape1, /*lfoShape2,*/ lfoShape3, lfoShape4 }, defaultLfoShape),
        std::make_unique<juce::AudioParameterChoice> (lfoDestID, lfoDestID.getParamID (), juce::StringArray { lfoDest0, lfoDest1, lfoDest2, lfoDest3 }, defaultLfoDest),
        std::make_unique<juce::AudioParameterFloat>  (lfoAmountID, lfoAmountID.getParamID (), sliderRange, defaultLfoAmount),

        std::make_unique<juce::AudioParameterFloat>  (reverbParam1ID, reverbParam1ID.getParamID (), sliderRange, defaultEffectParam1),
        std::make_unique<juce::AudioParameterFloat>  (reverbParam2ID, reverbParam2ID.getParamID (), sliderRange, defaultEffectParam2),
        std::make_unique<juce::AudioParameterFloat>  (chorusParam1ID, chorusParam1ID.getParamID (), sliderRange, defaultEffectParam1),
        std::make_unique<juce::AudioParameterFloat>  (chorusParam2ID, chorusParam2ID.getParamID (), sliderRange, defaultEffectParam2),
        std::make_unique<juce::AudioParameterFloat>  (phaserParam1ID, phaserParam1ID.getParamID (), sliderRange, defaultEffectParam1),
        std::make_unique<juce::AudioParameterFloat>  (phaserParam2ID, phaserParam2ID.getParamID (), sliderRange, defaultEffectParam2),
        std::make_unique<juce::AudioParameterChoice> (effectSelectedID, effectSelectedID.getParamID (), juce::StringArray { effect0, effect1, effect2, effect3 }, defaultLfoShape),

        std::make_unique<juce::AudioParameterFloat>  (masterGainID, masterGainID.getParamID (), sliderRange, defaultMasterGain)
    }};
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProPhatProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
           || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void ProPhatProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) [[clang::nonblocking]]
{
    jassert (! isUsingDoublePrecision());
    process (buffer, midiMessages);
}

void ProPhatProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) [[clang::nonblocking]]
{
    jassert (isUsingDoublePrecision());
    process (buffer, midiMessages);
}

template <std::floating_point T>
void ProPhatProcessor::process (juce::AudioBuffer<T>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

#if CPU_USAGE
    perfCounter.start ();
#endif

    //we're not dealing with any inputs here, so clear the buffer
    buffer.clear ();

    for (const auto metadata : midiMessages)
    {
        if (metadata.getMessage ().isNoteOn ())
        {
            midiListeners.call ([&midiMessages] (MidiMessageListener& l) { l.receivedMidiMessage (midiMessages); });
            break;
        }
    }

    //render the block
    if (isUsingDoublePrecision())
        proPhatSynthDouble.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    else
        proPhatSynthFloat.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

#if CPU_USAGE
    perfCounter.stop();
#endif
}

void ProPhatProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xmlState { state.copyState ().createXml () })
        copyXmlToBinary (*xmlState, destData);
}

void ProPhatProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState { getXmlFromBinary (data, sizeInBytes) })
        state.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* ProPhatProcessor::createEditor()
{
    return new ProPhatEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProPhatProcessor();
}
