#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

static const NormalisableRange<float> sliderRange = {-12.0f, 12.0f};

namespace sBMP4AudioProcessorIDs
{
    const String oscillatorsComboID = "oscillatorsCombo";
    const String oscillatorsSliderID = "oscillatorsSlider";
    const String oscillatorsButtonID = "oscillatorsButton";

    const String filterSliderID = "filterSlider";
    const String filterButtonID = "filterButton";

    const String envelopeSliderID = "envelopeSlider";
    const String envelopeButtonID = "envelopeButton";

    const String lfoComboID = "lfoCombo";

    const String lfoSliderID = "lfoSlider";

    const String oscillatorsSectionID = "oscillatorsSection";
    const String filterSectionID = "filterSection";
    const String envelopeSectionID = "envelopeSection";
    const String lfoSectionID = "lfoSection";
}

namespace sBMP4AudioProcessorNames
{
    const String oscillatorsComboDescription = "Oscillators Model";
    const String oscillatorsSliderDescription = "Oscillators Gain";
    const String oscillatorsButtonDescription = "Oscillators ON";

    const String filterSliderDescription = "Filter Gain";
    const String filterButtonDescription = "Filter ON";

    const String envelopeSliderDescription = "Envelope Gain";
    const String envelopeButtonDescription = "Envelope ON";

    const String lfoComboDescription = "Wave Type";

    const String lfoSliderDescription = "LFO Frequency";

    const String oscillatorsSectionDescription = "Oscillators";
    const String filterSectionDescription = "Filter";
    const String envelopeSectionDescription = "Envelope";
    const String lfoSectionDescription = "LFO";
}

namespace sBMP4AudioProcessorChoices
{
    const String oscillatorsChoice0 = "Sine";
    const String oscillatorsChoice1 = "Other";

    const String lfoChoices0 = "Sine";
    const String lfoChoices1 = "Pulse";
}

//==============================================================================

class sBMP4AudioProcessor  : public AudioProcessor, public ValueTree::Listener
{
public:

    //==============================================================================
    sBMP4AudioProcessor();
    ~sBMP4AudioProcessor();

    void valueTreePropertyChanged (juce::ValueTree &/*v*/, const juce::Identifier &/*id*/) override
    {
#if 0

#endif
    }

    float getSliderLinearGain (StringRef id)
    {
        return Decibels::decibelsToGain (sliderRange.convertFrom0to1 (state.getParameter (id)->getValue()));
    }

    void reset() override
    {
        lrFilter.reset();
    }

    bool getIsButtonEnabled (StringRef id) { return state.getParameter (id)->getValue(); }

    String getSelectedChoice (StringRef id) { return ((AudioParameterChoice*) state.getParameter (id))->getCurrentValueAsText(); }

    void valueTreeChildAdded (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenAdded*/) override {}
    void valueTreeChildRemoved (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override {}
    void valueTreeChildOrderChanged (juce::ValueTree &/*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged (juce::ValueTree &/*treeWhoseParentHasChanged*/) override {}

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override;

    void process (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    void processLeftRight (AudioBuffer<float>& ogBuffer);

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState state;

private:
    //==============================================================================
    int numSamples = -1;

    dsp::IIR::Filter<float> lrFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessor)
};
