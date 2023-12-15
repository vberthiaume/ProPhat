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

#include "SineWaveVoice.h"
#include "Helpers.h"

class sBMP4Synthesiser : public Synthesiser, public AudioProcessorValueTreeState::Listener
{
public:
    sBMP4Synthesiser();

    void prepare (const dsp::ProcessSpec& spec) noexcept;

    void parameterChanged (const String& parameterID, float newValue) override;

    using VoiceOperation = std::function<void (sBMP4Voice*, float)>;
    void applyToAllVoices (VoiceOperation operation, float newValue);

    void setEffectParam (StringRef parameterID, float newValue);

    void setMasterGain (float gain)
    {
        fxChain.get<masterGainIndex>().setGainLinear (gain);
    }

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override;

private:
    void renderVoices (AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;

    enum
    {
        reverbIndex,
        masterGainIndex
    };

    //@TODO: make this into a bit mask thing?
    std::set<int> voicesBeingKilled{};

    dsp::ProcessorChain<dsp::Reverb, dsp::Gain<float>> fxChain;

    dsp::Reverb::Parameters reverbParams;

    dsp::ProcessSpec curSpecs{};
};
