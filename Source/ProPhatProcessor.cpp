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

#include "ProPhatProcessor.h"
#include "UI/ProPhatEditor.h"
#include <limits>

using namespace ProPhatAudioProcessorIDs;
using namespace ProPhatAudioProcessorNames;
using namespace ProPhatAudioProcessorChoices;
using namespace Constants;

ProPhatProcessor::ProPhatProcessor()
    : juce::AudioProcessor (BusesProperties().withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                                             .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , state { constructState () }
#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
    addParamListenersToState ();
}

juce::AudioProcessorValueTreeState ProPhatProcessor::constructState ()
{
    //TODO: add undo manager!
    return { *this, nullptr, "state",
    {
        std::make_unique<juce::AudioParameterInt>    (osc1FreqID, osc1FreqDesc, midiNoteRange.getRange ().getStart (), midiNoteRange.getRange ().getEnd (), defaultOscMidiNote),
        std::make_unique<juce::AudioParameterInt>    (osc2FreqID, osc2FreqDesc, midiNoteRange.getRange ().getStart (), midiNoteRange.getRange ().getEnd (), defaultOscMidiNote),

        std::make_unique<juce::AudioParameterFloat>  (osc1TuningID, osc1TuningDesc, tuningSliderRange, (float) defaultOscTuning),
        std::make_unique<juce::AudioParameterFloat>  (osc2TuningID, osc2TuningDesc, tuningSliderRange, (float) defaultOscTuning),

        std::make_unique<juce::AudioParameterFloat>  (oscSubID, oscSubOctDesc, sliderRange, (float) defaultSubOsc),
        std::make_unique<juce::AudioParameterFloat>  (oscMixID, oscMixDesc, sliderRange, (float) defaultOscMix),
        std::make_unique<juce::AudioParameterFloat>  (oscNoiseID, oscNoiseDesc, sliderRange, (float) defaultOscNoise),
        std::make_unique<juce::AudioParameterFloat>  (oscSlopID, oscSlopDesc, slopSliderRange, (float) defaultOscSlop),

        std::make_unique<juce::AudioParameterChoice> (osc1ShapeID, osc1ShapeDesc,  juce::StringArray { oscShape0, oscShape1, oscShape2, oscShape3, oscShape4 }, defaultOscShape),
        std::make_unique<juce::AudioParameterChoice> (osc2ShapeID, osc2ShapeDesc,  juce::StringArray { oscShape0, oscShape1, oscShape2, oscShape3, oscShape4 }, defaultOscShape),

        std::make_unique<juce::AudioParameterFloat>  (filterCutoffID, filterCutoffSliderDesc, cutOffRange, defaultFilterCutoff),
        std::make_unique<juce::AudioParameterFloat>  (filterResonanceID, filterResonanceSliderDesc, sliderRange, defaultFilterResonance),

        std::make_unique<juce::AudioParameterFloat>  (ampAttackID, ampAttackSliderDesc, attackRange, defaultAmpA),
        std::make_unique<juce::AudioParameterFloat>  (ampDecayID, ampDecaySliderDesc, decayRange, defaultAmpD),
        std::make_unique<juce::AudioParameterFloat>  (ampSustainID, ampSustainSliderDesc, sustainRange, defaultAmpS),
        std::make_unique<juce::AudioParameterFloat>  (ampReleaseID, ampReleaseSliderDesc, releaseRange, defaultAmpR),

        std::make_unique<juce::AudioParameterFloat>  (filterEnvAttackID, ampAttackSliderDesc, attackRange, defaultAmpA),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvDecayID, ampDecaySliderDesc, decayRange, defaultAmpD),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvSustainID, ampSustainSliderDesc, sustainRange, defaultAmpS),
        std::make_unique<juce::AudioParameterFloat>  (filterEnvReleaseID, ampReleaseSliderDesc, releaseRange, defaultAmpR),

        std::make_unique<juce::AudioParameterFloat>  (lfoFreqID, lfoFreqSliderDesc, lfoRange, defaultLfoFreq),
        std::make_unique<juce::AudioParameterChoice> (lfoShapeID, lfoShapeDesc, juce::StringArray { lfoShape0, lfoShape1, /*lfoShape2,*/ lfoShape3, lfoShape4 }, defaultLfoShape),
        std::make_unique<juce::AudioParameterChoice> (lfoDestID, lfoDestDesc, juce::StringArray { lfoDest0, lfoDest1, lfoDest2, lfoDest3 }, defaultLfoDest),
        std::make_unique<juce::AudioParameterFloat>  (lfoAmountID, lfoAmountSliderDesc, sliderRange, defaultLfoAmount),

        std::make_unique<juce::AudioParameterFloat>  (effectParam1ID, effectParam1Desc, sliderRange, defaultEffectParam1),
        std::make_unique<juce::AudioParameterFloat>  (effectParam2ID, effectParam2Desc, sliderRange, defaultEffectParam2),

        std::make_unique<juce::AudioParameterFloat>  (masterGainID, masterGainDesc, sliderRange, defaultMasterGain)
    }};
}

void ProPhatProcessor::addParamListenersToState ()
{
    //NOW HERE: WHAT DOES THIS ACTUALLY DO?
    state.addParameterListener (osc1FreqID.getParamID (), &ProPhatSynth);
    state.addParameterListener (osc2FreqID.getParamID (), &ProPhatSynth);

    state.addParameterListener (osc1TuningID.getParamID (), &ProPhatSynth);
    state.addParameterListener (osc2TuningID.getParamID (), &ProPhatSynth);

    state.addParameterListener (osc1ShapeID.getParamID (), &ProPhatSynth);
    state.addParameterListener (osc2ShapeID.getParamID (), &ProPhatSynth);

    state.addParameterListener (oscSubID.getParamID (), &ProPhatSynth);
    state.addParameterListener (oscMixID.getParamID (), &ProPhatSynth);
    state.addParameterListener (oscNoiseID.getParamID (), &ProPhatSynth);
    state.addParameterListener (oscSlopID.getParamID (), &ProPhatSynth);

    state.addParameterListener (filterCutoffID.getParamID (), &ProPhatSynth);
    state.addParameterListener (filterResonanceID.getParamID (), &ProPhatSynth);
    state.addParameterListener (filterEnvAttackID.getParamID (), &ProPhatSynth);
    state.addParameterListener (filterEnvDecayID.getParamID (), &ProPhatSynth);
    state.addParameterListener (filterEnvSustainID.getParamID (), &ProPhatSynth);
    state.addParameterListener (filterEnvReleaseID.getParamID (), &ProPhatSynth);

    state.addParameterListener (ampAttackID.getParamID (), &ProPhatSynth);
    state.addParameterListener (ampDecayID.getParamID (), &ProPhatSynth);
    state.addParameterListener (ampSustainID.getParamID (), &ProPhatSynth);
    state.addParameterListener (ampReleaseID.getParamID (), &ProPhatSynth);

    state.addParameterListener (lfoShapeID.getParamID (), &ProPhatSynth);
    state.addParameterListener (lfoDestID.getParamID (), &ProPhatSynth);
    state.addParameterListener (lfoFreqID.getParamID (), &ProPhatSynth);
    state.addParameterListener (lfoAmountID.getParamID (), &ProPhatSynth);

    state.addParameterListener (effectParam1ID.getParamID (), &ProPhatSynth);
    state.addParameterListener (effectParam2ID.getParamID (), &ProPhatSynth);

    state.addParameterListener (masterGainID.getParamID (), &ProPhatSynth);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProPhatProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
           || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void ProPhatProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    jassert (! isUsingDoublePrecision());
    process (buffer, midiMessages);
}

void ProPhatProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    jassert (isUsingDoublePrecision());
    process (buffer, midiMessages);
}

template <typename T>
void ProPhatProcessor::process (juce::AudioBuffer<T>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

#if CPU_USAGE
    perfCounter.start();
#endif

    //we're not dealing with any inputs here, so clear the buffer
    buffer.clear();

    //render the block
    ProPhatSynth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

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