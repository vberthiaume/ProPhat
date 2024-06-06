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

#pragma once

#include "../Utility/Macros.h"
#include "ProPhatSynthesiser.h"

/** The main AudioProcessor for the plugin.
*   All we do in here is basically set up the state and init the ProPhatSynth.
*/
class ProPhatProcessor : public juce::AudioProcessor
{
public:
    ProPhatProcessor();

    bool supportsDoublePrecisionProcessing () const override { return true; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void reset () override {}
    void releaseResources () override {}

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    template <std::floating_point T>
    void process (juce::AudioBuffer<T>& buffer, juce::MidiBuffer& midiMessages);

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int /*index*/) override { }
    const juce::String getProgramName (int /*index*/) override { return {}; }
    void changeProgramName (int /*index*/, const juce::String& /*newName*/) override { }

    void getStateInformation (juce::MemoryBlock& destData) override;

    /** This is called at startup and will cause callbacks to
    *   ProPhatSynthesiser::parameterChanged() for all stored parameters.
    */
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;

#if CPU_USAGE
    PerformanceCounter perfCounter;
#endif

    struct MidiMessageListener
    {
        virtual void receivedMidiMessage (juce::MidiBuffer& midiMessages) = 0;
        virtual ~MidiMessageListener() = default;
    };

    juce::ListenerList<MidiMessageListener> midiListeners;

private:
    ProPhatSynthesiser<float> proPhatSynthFloat;
    ProPhatSynthesiser<double> proPhatSynthDouble;

    juce::AudioProcessorValueTreeState constructState ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProPhatProcessor)
};
