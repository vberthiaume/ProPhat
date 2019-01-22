#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SineWaveVoice.h"
#include "Helpers.h"

static const NormalisableRange<float> sliderRange = {-12.0f, 12.0f};

//#ifndef CPU_USAGE
//    #define CPU_USAGE 1
//#endif

#ifndef STANDARD_LISTENER
    #define STANDARD_LISTENER 0
#endif

namespace sBMP4AudioProcessorIDs
{
    const String oscEnableButtonID = "oscEnableButton";
    const String oscComboID = "oscCombo";
    const String oscSliderID = "oscSlider";
    const String oscWavetableButtonID = "oscWaveTableButton";

    const String filterEnableButtonID = "filterEnableButton";
    const String filterSliderID = "filterSlider";

    const String envelopeEnableButtonID = "envelopeEnableButton";
    const String envelopeSliderID = "envelopeSlider";

    const String lfoComboID = "lfoCombo";
    const String lfoSliderID = "lfoSlider";

    const String roomSizeID = "roomSize";
}

namespace sBMP4AudioProcessorNames
{
    const String oscEnableButtonDesc = "Oscillators ON";
    const String oscComboDesc = "Oscillators Model";
    const String oscSliderDesc = "Oscillators Gain";
    const String oscWavetableButtonDesc = "Use WaveTable";

    const String filterEnableButtonDesc = "Filter ON";
    const String filterSliderDesc = "Filter Gain";

    const String envelopeEnableButtonDesc = "Envelope ON";
    const String envelopeSliderDesc = "Envelope Gain";

    const String lfoComboDesc = "Wave Type";
    const String lfoSliderDesc = "LFO Frequency";

    const String oscGroupDesc = "Oscillators";
    const String filterGroupDesc = "Filter";
    const String envelopeGroupDesc = "Envelope";
    const String lfoGroupDesc = "LFO";
    const String effectGroupDesc = "Effect";

    const String roomSizeDesc = "Room Size";
}

namespace sBMP4AudioProcessorChoices
{
    const String oscChoice0 = "Sine";
    const String oscChoice1 = "Other";

    const String lfoChoices0 = "Sine";
    const String lfoChoices1 = "Pulse";
}

//==============================================================================

class sBMP4AudioProcessor : public AudioProcessor
#if STANDARD_LISTENER
    , public ValueTree::Listener
#else
    , public AudioProcessorValueTreeState::Listener
#endif
{
public:

    //==============================================================================
    sBMP4AudioProcessor();
    ~sBMP4AudioProcessor();

#if STANDARD_LISTENER
    void valueTreePropertyChanged (juce::ValueTree &v, const juce::Identifier &/*id*/) override
    {
        if (v.getParent() != state.state)
            return;

        auto paramName = v.getProperty ("id").toString();
        auto paramValue = v.getProperty ("value").toString();
        auto paramValue2 = (float) state.getParameterAsValue (paramName).getValue();

        DBG (paramName + "now has the value: " + paramValue + ", which is the same as this: " + String (paramValue2));
    }

    void valueTreeChildAdded (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenAdded*/) override {}
    void valueTreeChildRemoved (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override {}
    void valueTreeChildOrderChanged (juce::ValueTree &/*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged (juce::ValueTree &/*treeWhoseParentHasChanged*/) override {}
#else
    void parameterChanged (const String& parameterID, float newValue) override
    {
        if (parameterID == sBMP4AudioProcessorIDs::oscWavetableButtonID && usingWavetables != (bool) newValue)
            needToSwitchWavetableStatus = true;
    }
#endif
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
    const String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

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

#if CPU_USAGE
    PerformanceCounter perfCounter;
#endif

private:

    void setUseWavetables (bool useThem);

    void createWavetable()
    {
        //initialize the table
        sineTable.setSize (1, tableSize + 1);
        sineTable.clear();
        double* samples = sineTable.getWritePointer (0);

        //decide on the harmonics
        int harmonics[] = {1}; //{1, 3, 5, 6, 7, 9, 13, 15};
        double harmonicWeights[] = {1.0}; //{0.5f, 0.1f, 0.05f, 0.125f, 0.09f, 0.005f, 0.002f, 0.001f};
        jassert (numElementsInArray (harmonics) == numElementsInArray (harmonicWeights));

        for (auto harmonic = 0; harmonic < numElementsInArray (harmonics); ++harmonic)
        {
            //we divide 2pi by the table size. Like this, the table will cover one full cycle
            auto angleDelta = MathConstants<double>::twoPi / (double) (tableSize - 1) * harmonics[harmonic];
            auto currentAngle = 0.0;

            for (size_t i = 0; i < tableSize; ++i)
            {
                auto sample = std::sin (currentAngle);
                samples[i] += sample * harmonicWeights[harmonic];
                currentAngle += angleDelta;
            }
        }

        samples[tableSize] = samples[0];
    }

    //==============================================================================
    double lastSampleRate = {};

    Synthesiser synth;
    Reverb reverb;

    const unsigned int tableSize = 2 << 15; // 2^15 == 32768 slots; 32768 * 4 = 131kB
    int numberOfOscillators = 16;
    AudioBuffer<double> sineTable;
    bool usingWavetables = false, needToSwitchWavetableStatus = false;

    dsp::IIR::Filter<float> lrFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessor)
};
