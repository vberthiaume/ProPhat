
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
        std::make_unique<AudioParameterChoice> (shotgunComboID,  shotgunComboDescription,  StringArray {shotgunChoice0, shotgunChoice1}, 0),
        std::make_unique<AudioParameterFloat> (shotgunSliderID, shotgunSliderDescription, sliderRange, 0.0f),
        std::make_unique<AudioParameterBool> (shotgunButtonID, shotgunButtonDescription, true, shotgunButtonDescription),

        std::make_unique<AudioParameterFloat> (frontBackSliderID, frontBackSliderDescription, sliderRange, 0.0f),
        std::make_unique<AudioParameterBool> (frontBackButtonID, frontBackButtonDescription, true, frontBackButtonDescription),

        std::make_unique<AudioParameterFloat> (upDownSliderID, upDownSliderDescription, sliderRange, 0.0f),
        std::make_unique<AudioParameterBool> (upDownButtonID, upDownButtonDescription, true, upDownButtonDescription),

        std::make_unique<AudioParameterChoice> (bFormatComboID,  bFormatComboDescription,  StringArray {bFormatChoices0, bFormatChoices1}, 0),
        std::make_unique<AudioParameterFloat> (outputSliderID, outputSliderDescription, sliderRange, 0.0f)
    })
{
    state.state.addListener (this);
}

sBMP4AudioProcessor::~sBMP4AudioProcessor()
{
}

//==============================================================================
const String sBMP4AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool sBMP4AudioProcessor::acceptsMidi() const
{
    return false;

}

bool sBMP4AudioProcessor::producesMidi() const
{
    return false;
}

bool sBMP4AudioProcessor::isMidiEffect() const
{
    return false;
}

double sBMP4AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int sBMP4AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int sBMP4AudioProcessor::getCurrentProgram()
{
    return 0;
}

void sBMP4AudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String sBMP4AudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void sBMP4AudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void sBMP4AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    numSamples = samplesPerBlock;

    auto channels = static_cast<uint32> (jmin (getMainBusNumInputChannels(), getMainBusNumOutputChannels()));

    dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), channels };

    lrFilter.prepare (spec);
    lrFilter.coefficients = dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1000.f, .3f, Decibels::decibelsToGain (-6.f));
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

void sBMP4AudioProcessor::process (AudioBuffer<float>& ogBuffer, MidiBuffer& /*midiMessages*/)
{
    //clear unused output channels, if there's any (there shoudn't, but whatever)
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        ogBuffer.clear (channel, 0, numSamples);

    processLeftRight (ogBuffer);

    //apply outputGain
    const auto outputGain = getSliderLinearGain (outputSliderID);
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        ogBuffer.applyGain (channel, 0, numSamples, outputGain);
}

void sBMP4AudioProcessor::processLeftRight (AudioBuffer<float>& ogBuffer)
{
    for (int i = 0; i < ogBuffer.getNumChannels(); ++i)
    {
        auto audioBlock = dsp::AudioBlock<float> (ogBuffer).getSingleChannelBlock (i);

        // This class can only process mono signals. Use the ProcessorDuplicator class
        // to apply this filter on a multi-channel audio stream.
        lrFilter.process (dsp::ProcessContextReplacing<float> (audioBlock));
    }
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
