#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

static const NormalisableRange<float> sliderRange = {-12.0f, 12.0f};

namespace Jero2BAudioProcessorIDs
{
    const String shotgunComboID = "shotgunCombo";
    const String shotgunSliderID = "shotgunSlider";
    const String shotgunButtonID = "shotgunButton";

    const String frontBackSliderID = "frontBackSlider";
    const String frontBackButtonID = "frontBackButton";

    const String upDownSliderID = "upDownSlider";
    const String upDownButtonID = "upDownButton";

    const String bFormatComboID = "bFormatCombo";

    const String outputSliderID = "outputSlider";

    const String shotgunSectionID = "shotgunSection";
    const String frontBackSectionID = "frontBackSection";
    const String upDownSectionID = "upDownSection";
    const String outputSectionID = "outputSection";
}

namespace Jero2BAudioProcessorNames
{
    const String shotgunComboDescription = "Shotgun Model";
    const String shotgunSliderDescription = "Shotgun Gain";
    const String shotgunButtonDescription = "Shotgun ON";

    const String frontBackSliderDescription = "Front-Back Axis Gain";
    const String frontBackButtonDescription = "Front-Back Axis ON";

    const String upDownSliderDescription = "Up-Down Axis Gain";
    const String upDownButtonDescription = "Up-Down Axis ON";

    const String bFormatComboDescription = "B-Format Type";

    const String outputSliderDescription = "Output Gain";

    const String shotgunSectionDescription = "Shotgun";
    const String frontBackSectionDescription = "Front-Back";
    const String upDownSectionDescription = "Up-Down";
    const String outputSectionDescription = "Output";
}

namespace Jero2BAudioProcessorChoices
{
    const String shotgunChoice0 = "DPA 4017";
    const String shotgunChoice1 = "Other";

    const String bFormatChoices0 = "ambiX";
    const String bFormatChoices1 = "FuMa";
}

//==============================================================================

class Jero2BAudioProcessor  : public AudioProcessor, public ValueTree::Listener
{
public:

    //==============================================================================
    Jero2BAudioProcessor();
    ~Jero2BAudioProcessor();

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jero2BAudioProcessor)
};
