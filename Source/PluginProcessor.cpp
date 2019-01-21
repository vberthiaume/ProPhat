
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
        std::make_unique<AudioParameterBool> (oscEnableButtonID, oscEnableButtonDesc, true, oscEnableButtonDesc),
        std::make_unique<AudioParameterChoice> (oscComboID,  oscComboDesc,  StringArray {oscChoice0, oscChoice1}, 0),
        std::make_unique<AudioParameterFloat> (oscSliderID, oscSliderDesc, sliderRange, 0.0f),
        std::make_unique<AudioParameterBool> (oscWavetableButtonID, oscWavetableButtonDesc, true, oscWavetableButtonDesc),

        std::make_unique<AudioParameterBool> (filterEnableButtonID, filterEnableButtonDesc, true, filterEnableButtonDesc),
        std::make_unique<AudioParameterFloat> (filterSliderID, filterSliderDesc, sliderRange, 0.0f),
        
        std::make_unique<AudioParameterBool> (envelopeEnableButtonID, envelopeEnableButtonDesc, true, envelopeEnableButtonDesc),
        std::make_unique<AudioParameterFloat> (envelopeSliderID, envelopeSliderDesc, sliderRange, 0.0f),
        
        std::make_unique<AudioParameterChoice> (lfoComboID,  lfoComboDesc,  StringArray {lfoChoices0, lfoChoices1}, 0),
        std::make_unique<AudioParameterFloat> (lfoSliderID, lfoSliderDesc, sliderRange, 0.0f),

        std::make_unique<AudioParameterFloat> (roomSizeID, roomSizeDesc, sliderRange, 0.0f)
    })

#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
    state.state.addListener (this);

    createWavetable();

    //@todo this is the switch between wave table and sine
    for (auto i = 0; i < 4; ++i)
        //synth.addVoice (new SineWaveTableVoice (sineTable));
        synth.addVoice (new SineWaveVoice());

    synth.addSound (new SineWaveSound());
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
    lastSampleRate = sampleRate;

    auto channels = static_cast<uint32> (jmin (getMainBusNumInputChannels(), getMainBusNumOutputChannels()));

    dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), channels };

    lrFilter.prepare (spec);
    lrFilter.coefficients = dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1000.f, .3f, Decibels::decibelsToGain (-6.f));

    synth.setCurrentPlaybackSampleRate (sampleRate);
    reverb.setSampleRate (sampleRate);
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
#if CPU_USAGE
    perfCounter.start();
#endif
    //@TODO not sure there's actually a point o this?
    //buffer.clear();

    Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = state.getParameter (roomSizeID)->getValue();

    reverb.setParameters (reverbParameters);
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    if (getMainBusNumOutputChannels() == 1)
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
    else if (getMainBusNumOutputChannels() == 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());

#if CPU_USAGE
    perfCounter.stop();
#endif
}

void sBMP4AudioProcessor::processLeftRight (AudioBuffer<float>& ogBuffer)
{
    for (int i = 0; i < ogBuffer.getNumChannels(); ++i)
    {
        auto audioBlock = dsp::AudioBlock<float> (ogBuffer).getSingleChannelBlock (i);

        //TODO This class can only process mono signals. Use the ProcessorDuplicator class
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
