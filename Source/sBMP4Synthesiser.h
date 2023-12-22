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

#pragma once

#include "sBMP4Voice.h"
#include "Helpers.h"

/** The main Synthesiser for the plugin. It uses Constants::numVoices voices (of type sBMP4Voice),
*   and one sBMP4Sound, which applies to all midi notes. It responds to paramater changes in the
*   state via juce::AudioProcessorValueTreeState::Listener().
*/
class sBMP4Synthesiser : public juce::Synthesiser, public juce::AudioProcessorValueTreeState::Listener
{
public:
    sBMP4Synthesiser();

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    using VoiceOperation = std::function<void (sBMP4Voice*, float)>;
    inline void applyToAllVoices (VoiceOperation operation, float newValue)
    {
        for (auto voice : voices)
            operation (dynamic_cast<sBMP4Voice*> (voice), newValue);
    }

    void setEffectParam (juce::StringRef parameterID, float newValue);

    void setMasterGain (float gain) { fxChain.get<masterGainIndex>().setGainLinear (gain); }

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override;

private:
    void renderVoices (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;

    enum
    {
        reverbIndex = 0,
        masterGainIndex
    };

    //@TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled;

    juce::dsp::ProcessorChain<juce::dsp::Reverb, juce::dsp::Gain<float>> fxChain;

    juce::dsp::Reverb::Parameters reverbParams;

    juce::dsp::ProcessSpec curSpecs;
};
