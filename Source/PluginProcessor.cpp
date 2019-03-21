/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   sBMP4 IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <limits>

using namespace sBMP4AudioProcessorIDs;
using namespace sBMP4AudioProcessorNames;
using namespace sBMP4AudioProcessorChoices;
using namespace Constants;

//==============================================================================
sBMP4AudioProcessor::sBMP4AudioProcessor() :
    AudioProcessor (BusesProperties().withInput  ("Input", AudioChannelSet::stereo(), true)
                                     .withOutput ("Output", AudioChannelSet::stereo(), true)),
    state (*this, nullptr, "state",
    {
        std::make_unique<AudioParameterInt>     (osc1FreqID, osc1FreqDesc, midiNoteRange.getRange().getStart(),
                                                                           midiNoteRange.getRange().getEnd(), defaultOscMidiNote),
        std::make_unique<AudioParameterInt>     (osc2FreqID, osc2FreqDesc, midiNoteRange.getRange().getStart(),
                                                                           midiNoteRange.getRange().getEnd(), defaultOscMidiNote),

        std::make_unique<AudioParameterFloat>   (osc1TuningID, osc1TuningDesc, centeredSliderRange, (float) defaultOscTuning),
        std::make_unique<AudioParameterFloat>   (osc2TuningID, osc2TuningDesc, centeredSliderRange, (float) defaultOscTuning),

        std::make_unique<AudioParameterFloat>   (oscSubID, oscSubOctDesc, sliderRange, (float) defaultSubOsc),
        std::make_unique<AudioParameterFloat>   (oscMixID, oscMixDesc, sliderRange, (float) defaultOscMix),

        std::make_unique<AudioParameterChoice>  (osc1ShapeID,  osc1ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3, oscShape4}, defaultOscShape),
        std::make_unique<AudioParameterChoice>  (osc2ShapeID,  osc2ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3, oscShape4}, defaultOscShape),

        std::make_unique<AudioParameterFloat>   (filterCutoffID, filterCutoffSliderDesc, cutOffRange, defaultFilterCutoff),
        std::make_unique<AudioParameterFloat>   (filterResonanceID, filterResonanceSliderDesc, sliderRange, defaultFilterResonance),

        std::make_unique<AudioParameterFloat>   (ampAttackID, ampAttackSliderDesc, attackRange, defaultAmpA),
        std::make_unique<AudioParameterFloat>   (ampDecayID, ampDecaySliderDesc, decayRange, defaultAmpD),
        std::make_unique<AudioParameterFloat>   (ampSustainID, ampSustainSliderDesc, sustainRange, defaultAmpS),
        std::make_unique<AudioParameterFloat>   (ampReleaseID, ampReleaseSliderDesc, releaseRange, defaultAmpR),

        std::make_unique<AudioParameterFloat>   (filterEnvAttackID, ampAttackSliderDesc, attackRange, defaultAmpA),
        std::make_unique<AudioParameterFloat>   (filterEnvDecayID, ampDecaySliderDesc, decayRange, defaultAmpD),
        std::make_unique<AudioParameterFloat>   (filterEnvSustainID, ampSustainSliderDesc, sustainRange, defaultAmpS),
        std::make_unique<AudioParameterFloat>   (filterEnvReleaseID, ampReleaseSliderDesc, releaseRange, defaultAmpR),

        std::make_unique<AudioParameterFloat>   (lfoFreqID,   lfoFreqSliderDesc, lfoRange, defaultLfoFreq),
        std::make_unique<AudioParameterChoice>  (lfoShapeID, lfoShapeDesc,  StringArray {lfoShape0, lfoShape1, /*lfoShape2,*/ lfoShape3, lfoShape4}, defaultLfoShape),
        std::make_unique<AudioParameterChoice>  (lfoDestID, lfoDestDesc,  StringArray {lfoDest0, lfoDest1, lfoDest2, lfoDest3}, defaultLfoDest),
        std::make_unique<AudioParameterFloat>   (lfoAmountID, lfoAmountSliderDesc, sliderRange, defaultLfoAmount),

        std::make_unique<AudioParameterFloat>   (effectParam1ID, effectParam1Desc, sliderRange, defaultEffectParam1),
        std::make_unique<AudioParameterFloat>   (effectParam2ID, effectParam2Desc, sliderRange, defaultEffectParam2)
    })

#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
    state.addParameterListener (osc1FreqID, &synth);
    state.addParameterListener (osc2FreqID, &synth);

    state.addParameterListener (osc1TuningID, &synth);
    state.addParameterListener (osc2TuningID, &synth);

    state.addParameterListener (osc1ShapeID, &synth);
    state.addParameterListener (osc2ShapeID, &synth);

    state.addParameterListener (oscSubID, &synth);
    state.addParameterListener (oscMixID, &synth);

    state.addParameterListener (filterCutoffID, &synth);
    state.addParameterListener (filterResonanceID, &synth);
    state.addParameterListener (filterEnvAttackID, &synth);
    state.addParameterListener (filterEnvDecayID, &synth);
    state.addParameterListener (filterEnvSustainID, &synth);
    state.addParameterListener (filterEnvReleaseID, &synth);

    state.addParameterListener (ampAttackID, &synth);
    state.addParameterListener (ampDecayID, &synth);
    state.addParameterListener (ampSustainID, &synth);
    state.addParameterListener (ampReleaseID, &synth);

    state.addParameterListener (lfoShapeID, &synth);
    state.addParameterListener (lfoDestID, &synth);
    state.addParameterListener (lfoFreqID, &synth);
    state.addParameterListener (lfoAmountID, &synth);

    state.addParameterListener (effectParam1ID, &synth);
    state.addParameterListener (effectParam2ID, &synth);
}

sBMP4AudioProcessor::~sBMP4AudioProcessor()
{
}

//==============================================================================

void sBMP4AudioProcessor::reset()
{
}

//==============================================================================
void sBMP4AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;

    synth.prepare ({sampleRate, (uint32) samplesPerBlock, 2});
}

void sBMP4AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool sBMP4AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}
#endif

//============================================================================= =
void sBMP4AudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    jassert (! isUsingDoublePrecision());
    process (buffer, midiMessages);
}

void sBMP4AudioProcessor::processBlock (AudioBuffer<double>& /*ogBuffer*/, MidiBuffer& /*midiMessages*/)
{
    jassertfalse;
    jassert (isUsingDoublePrecision());
    //process (ogBuffer, midiMessages);
}

void sBMP4AudioProcessor::process (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

#if CPU_USAGE
    perfCounter.start();
#endif
    //we're not dealing with any inputs here, so clear the buffer
    buffer.clear();

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

#if CPU_USAGE
    perfCounter.stop();
#endif
}

//==============================================================================
void sBMP4AudioProcessor::getStateInformation (MemoryBlock& destData)
{
    std::unique_ptr<XmlElement> xmlState (state.copyState().createXml());

    if (xmlState.get() != nullptr)
        copyXmlToBinary (*xmlState, destData);
}

void sBMP4AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        state.replaceState (ValueTree::fromXml (*xmlState));
}

//==============================================================================
AudioProcessorEditor* sBMP4AudioProcessor::createEditor()
{
    return new sBMP4AudioProcessorEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sBMP4AudioProcessor();
}
