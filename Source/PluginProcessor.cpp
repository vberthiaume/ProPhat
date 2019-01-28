
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
        std::make_unique<AudioParameterBool> (oscWavetableID, oscWavetableButtonDesc, false, oscWavetableButtonDesc),
        std::make_unique<AudioParameterChoice> (oscShapeID,  oscShapeDesc,  StringArray {oscShape0, oscShape1, oscShape2, oscShape3}, 0),
        std::make_unique<AudioParameterFloat> (osc1FreqID, oscFreqSliderDesc, hzRange, 0.0f),

        std::make_unique<AudioParameterFloat> (filterCutoffID, filterCutoffSliderDesc, hzRange, 1000.0f),
        
        std::make_unique<AudioParameterFloat> (ampAttackID, ampAttackSliderDesc, sliderRange, 0.0f),
        std::make_unique<AudioParameterFloat> (ampDecayID, ampDecaySliderDesc, sliderRange, 0.0f),
        std::make_unique<AudioParameterFloat> (ampSustainID, ampSustainSliderDesc, sliderRange, 0.0f),
        std::make_unique<AudioParameterFloat> (ampReleaseID, ampReleaseSliderDesc, sliderRange, 0.0f),
        
        std::make_unique<AudioParameterFloat> (lfoFreqID, lfoSliderDesc, lfoRange, 0.0f),

        std::make_unique<AudioParameterFloat> (effectParam1ID, effectParam1Desc, sliderRange, 0.0f)
    })

#if CPU_USAGE
    , perfCounter ("ProcessBlock")
#endif
{
#if STANDARD_LISTENER
    state.state.addListener (this);
#else
    state.addParameterListener (oscWavetableID, this);
#endif
    
    createWavetable();
}

sBMP4AudioProcessor::~sBMP4AudioProcessor()
{
}

void sBMP4AudioProcessor::setUseWavetables (bool useThem)
{
    synth.clearVoices();
    synth.clearSounds();

    usingWavetables = useThem;

    for (auto i = 0; i < numVoices; ++i)
        if (usingWavetables)
            synth.addVoice (new SineWaveTableVoice (sineTable));
        else
            synth.addVoice (new SineWaveVoice());

    synth.addSound (new SineWaveSound());
}

//==============================================================================
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

void sBMP4AudioProcessor::reset()
{
    //lrFilter.reset();
    ladderFilter.reset();
}

//==============================================================================
void sBMP4AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;

    auto channels = static_cast<uint32> (jmin (getMainBusNumInputChannels(), getMainBusNumOutputChannels()));

    dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), channels };

    //lrFilter.prepare (spec);
    //lrFilter.coefficients = dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1000.f, .3f, Decibels::decibelsToGain (-6.f));

    ladderFilter.prepare (spec);
    ladderFilter.setMode (juce::dsp::LadderFilter<float>::Mode::LPF12);

    synth.setCurrentPlaybackSampleRate (sampleRate);
    reverb.setSampleRate (sampleRate);

    setUseWavetables (usingWavetables);
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
    //we're not dealing with any inputs here, so clear the buffer
    buffer.clear();

    if (needToSwitchWavetableStatus)
    {
        setUseWavetables (! usingWavetables);
        needToSwitchWavetableStatus = false;
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    processFilter (buffer);

    processReverb (buffer);


#if CPU_USAGE
    perfCounter.stop();
#endif
}

void sBMP4AudioProcessor::processFilter (AudioBuffer<float>& buffer)
{
    /*auto cutOff = state.getParameter (filterCutoffID)->getValue();*/
    auto cutOff = state.getParameter (filterCutoffID)->convertFrom0to1 (state.getParameter (filterCutoffID)->getValue());

    if (cutOff == 0.f)
        return;

    ladderFilter.setCutoffFrequencyHz (cutOff);

    for (int i = 0; i < buffer.getNumChannels(); ++i)
    {
        auto audioBlock = dsp::AudioBlock<float> (buffer).getSingleChannelBlock (i);

        //TODO This class can only process mono signals. Use the ProcessorDuplicator class
        // to apply this filter on a multi-channel audio stream.
        //lrFilter.process (dsp::ProcessContextReplacing<float> (audioBlock));
        ladderFilter.process (dsp::ProcessContextReplacing<float> (audioBlock));
    }
}

void sBMP4AudioProcessor::processReverb (AudioBuffer<float>& buffer)
{
    Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = state.getParameter (effectParam1ID)->getValue();
    reverb.setParameters (reverbParameters);

    if (getMainBusNumOutputChannels() == 1)
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
    else if (getMainBusNumOutputChannels() == 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
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
