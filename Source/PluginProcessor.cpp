
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sBMP4AudioProcessorIDs;
using namespace sBMP4AudioProcessorNames;
using namespace sBMP4AudioProcessorChoices;

//==============================================================================
sBMP4AudioProcessor::sBMP4AudioProcessor() :
    AudioProcessor (BusesProperties().withInput  ("Input", AudioChannelSet::stereo(), true)
                                     .withOutput ("Output", AudioChannelSet::stereo(), true)),
    state (*this, nullptr, "state",
    {
        std::make_unique<AudioParameterInt>     (osc1FreqID, osc1FreqSliderDesc, midiNoteRange.getRange().getStart(),
                                                                                 midiNoteRange.getRange().getEnd(), defaultOscMidiNote),
        std::make_unique<AudioParameterInt>     (osc2FreqID, osc2FreqSliderDesc, midiNoteRange.getRange().getStart(),
                                                                                 midiNoteRange.getRange().getEnd(), defaultOscMidiNote),

#if 0
        std::make_unique<AudioParameterChoice>  (osc1ShapeID,  osc1ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3, oscShape4}, 1),
        std::make_unique<AudioParameterChoice>  (osc2ShapeID,  osc2ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3, oscShape4}, 1),
#else
        std::make_unique<AudioParameterChoice>  (osc1ShapeID,  osc1ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3}, 0),
        std::make_unique<AudioParameterChoice>  (osc2ShapeID,  osc2ShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3}, 0),
#endif

        std::make_unique<AudioParameterFloat>   (filterCutoffID, filterCutoffSliderDesc, hzRange, 1000.0f),
        std::make_unique<AudioParameterFloat>   (filterResonanceID, filterResonanceSliderDesc, sliderRange, .5f),
        
        std::make_unique<AudioParameterFloat>   (ampAttackID, ampAttackSliderDesc, ampRange, 0.1f),
        std::make_unique<AudioParameterFloat>   (ampDecayID, ampDecaySliderDesc, ampRange, 0.1f),
        std::make_unique<AudioParameterFloat>   (ampSustainID, ampSustainSliderDesc, sliderRange, 1.0f),
        std::make_unique<AudioParameterFloat>   (ampReleaseID, ampReleaseSliderDesc, ampRange, 0.25f),
        
        std::make_unique<AudioParameterFloat>   (lfoFreqID,   lfoFreqSliderDesc, lfoRange, 3.f),
        std::make_unique<AudioParameterChoice>  (lfoShapeID, lfoShapeDesc,  StringArray {lfoShape0, lfoShape1, /*lfoShape2, */lfoShape3, lfoShape4}, 0),
        std::make_unique<AudioParameterChoice>  (lfoDestID, lfoDestDesc,  StringArray {lfoDest0, lfoDest1, lfoDest2, lfoDest3}, 0),
        std::make_unique<AudioParameterFloat>   (lfoAmountID, lfoAmountSliderDesc, sliderRange, 0.0f),

        std::make_unique<AudioParameterFloat>   (effectParam1ID, effectParam1Desc, sliderRange, 0.0f)
    })

#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
#if STANDARD_LISTENER
    state.state.addListener (this);
#else
    state.addParameterListener (osc1FreqID, &synth);
    state.addParameterListener (osc2FreqID, &synth);

    state.addParameterListener (osc1ShapeID, &synth);
    state.addParameterListener (osc2ShapeID, &synth);

    state.addParameterListener (ampAttackID, &synth);
    state.addParameterListener (ampDecayID, &synth);
    state.addParameterListener (ampSustainID, &synth);
    state.addParameterListener (ampReleaseID, &synth);

    state.addParameterListener (lfoShapeID, &synth);
    state.addParameterListener (lfoDestID, &synth);
    state.addParameterListener (lfoFreqID, &synth);
    state.addParameterListener (lfoAmountID, &synth);

    state.addParameterListener (filterCutoffID, &synth);
    state.addParameterListener (filterResonanceID, &synth);
#endif

    //@TODO Helpers::getFloatMidiNoteInHertz does NOT approximate well MidiMessage::getMidiNoteInHertz for higher numbers
    //for (double i = .0; i < 127; i += .3)
    //    DBG (String (i) + "\t" + String (MidiMessage::getMidiNoteInHertz ((int) i)) + "\t" + String (Helpers::getFloatMidiNoteInHertz (i)));
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
