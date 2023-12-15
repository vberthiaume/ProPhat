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

#ifndef CPU_USAGE
    #define CPU_USAGE 0
#endif

#ifndef DEBUG_VOICES
    #define DEBUG_VOICES 0
#endif

#ifndef PRINT_ALL_SAMPLES
    #define PRINT_ALL_SAMPLES 0
#endif

namespace Constants
{
constexpr auto oscShapeRadioGroupId { 1 };

constexpr auto defaultSubOsc            { 0 };
constexpr auto defaultOscMix            { 0 };
constexpr auto defaultOscNoise          { 0 };
constexpr auto defaultOscSlop           { 0 };
constexpr auto defaultOscTuning         { 0 };

constexpr auto numVoices                { 16 };
constexpr auto defaultOscMidiNote       { 48 }; //C2 on rev2
constexpr auto middleCMidiNote          { 60 }; //C3 on rev2

constexpr auto killRampSamples          { 300 };
constexpr auto rampUpSamples            { 100 };

constexpr auto defaultOscLevel          { .4f };
constexpr auto defaultMasterGain        { .8f };

constexpr auto defaultFilterCutoff      { 1000.f };
constexpr auto defaultFilterResonance   { .5f };

constexpr float defaultLfoFreq          { 3.f };
constexpr float defaultLfoAmount        { 0.f };

constexpr float defaultEffectParam1     { 0.f };
constexpr float defaultEffectParam2     { 0.f };

//envelope stuff
constexpr auto minA                     { .001f };
constexpr auto minAmp                   { .01f };

constexpr auto defaultAmpA              { minA };
constexpr auto defaultAmpD              { minAmp };
constexpr auto defaultAmpS              { 1.f };
constexpr auto defaultAmpR              { .25f };

constexpr auto sustainSkewFactor        { .5f };
constexpr auto ampSkewFactor            { .5f };
constexpr auto cutOffSkewFactor         { .5f };
constexpr auto slopSkewFactor           { .5f };

const juce::NormalisableRange<float> attackRange        { minA, 25.f, 0.f, ampSkewFactor };
const juce::NormalisableRange<float> decayRange         { minAmp, 25.f, 0.f, ampSkewFactor };
const juce::NormalisableRange<float> sustainRange       { minAmp, 1.f,  0.f, sustainSkewFactor };
const juce::NormalisableRange<float> releaseRange       { minAmp, 25.f, 0.f, ampSkewFactor };

const juce::NormalisableRange<float> dBRange            { -12.f, 12.f };
const juce::NormalisableRange<float> sliderRange        { 0.f, 1.f };
const juce::NormalisableRange<float> tuningSliderRange  { -0.5f, .5f };
const juce::NormalisableRange<float> slopSliderRange    { 0.f, 1.0f, 0.f, slopSkewFactor };

const juce::NormalisableRange<float> cutOffRange        { 0.1f, 18000.f, 0.f, cutOffSkewFactor };
const juce::NormalisableRange<float> lfoRange           { 0.1f, 10.f };
const juce::NormalisableRange<float> lfoNoteRange       { 0.f, 16.f };

//Sets the base frequency of Oscillator 1 or 2 over a 9-octave
//range from 16 Hz to 8KHz (when used with the Transpose buttons). Adjustment is in semitones.
const juce::NormalisableRange<int> midiNoteRange        { 12, 120 };   //actual midi note range is (0,127), but rev2, at least for oscilators is C0(0) to C10(120)
const juce::NormalisableRange<float> pitchWheelNoteRange{ -7.f, 7.f };
}

namespace sBMP4AudioProcessorIDs
{
const juce::ParameterID  osc1ShapeID { "osc1ShapeID", 1 };
const juce::ParameterID  osc1FreqID { "osc1FreqID", 1 };
const juce::ParameterID  osc1TuningID { "osc1TuningID", 1 };

const juce::ParameterID  osc2ShapeID { "osc2ShapeID", 1 };
const juce::ParameterID  osc2FreqID { "osc2FreqID", 1 };
const juce::ParameterID  osc2TuningID { "osc2TuningID", 1 };

const juce::ParameterID  oscSubID { "oscSubID", 1 };
const juce::ParameterID  oscMixID { "oscMixID", 1 };
const juce::ParameterID  oscNoiseID { "oscNoiseID", 1 };
const juce::ParameterID  oscSlopID { "oscSlopID", 1 };

const juce::ParameterID  filterCutoffID { "filterCutoffID", 1 };
const juce::ParameterID  filterResonanceID { "filterResonanceID", 1 };

const juce::ParameterID  ampAttackID { "ampAttackID", 1 };
const juce::ParameterID  ampDecayID { "ampDecayID", 1 };
const juce::ParameterID  ampSustainID { "ampSustainID", 1 };
const juce::ParameterID  ampReleaseID { "ampReleaseID", 1 };

const juce::ParameterID  filterEnvAttackID { "filterEnvAttackID", 1 };
const juce::ParameterID  filterEnvDecayID { "filterEnvDecayID", 1 };
const juce::ParameterID  filterEnvSustainID { "filterEnvSustainID", 1 };
const juce::ParameterID  filterEnvReleaseID { "filterEnvReleaseID", 1 };

const juce::ParameterID  lfoShapeID { "lfoShapeID", 1 };
const juce::ParameterID  lfoDestID { "lfoDestID", 1 };
const juce::ParameterID  lfoFreqID { "lfoFreqID", 1 };
const juce::ParameterID  lfoAmountID { "lfoAmountID", 1 };

const juce::ParameterID  effectID { "effectID", 1 };
const juce::ParameterID  effectParam1ID { "effectParam1ID", 1 };
const juce::ParameterID  effectParam2ID { "effectParam2ID", 1 };

const juce::ParameterID  masterGainID { "masterGainID", 1 };
}

namespace sBMP4AudioProcessorNames
{
const juce::String oscGroupDesc = "OSCILLATORS";
const juce::String osc1ShapeDesc = "SHAPE";
const juce::String osc2ShapeDesc = "SHAPE";
const juce::String osc1FreqDesc = "OSC 1 FREQ";
const juce::String osc2FreqDesc = "OSC 2 FREQ";
const juce::String osc1TuningDesc = "FINE TUNE";
const juce::String osc2TuningDesc = "FINE TUNE";

const juce::String oscSubOctDesc = "SUB OCTAVE";
const juce::String oscMixDesc = "OSC MIX";
const juce::String oscNoiseDesc = "NOISE";
const juce::String oscSlopDesc = "OSC SLOP";

const juce::String filterGroupDesc = "LOW-PASS FILTER";
const juce::String filterCutoffSliderDesc = "CUTOFF";
const juce::String filterResonanceSliderDesc = "RESONANCE";

const juce::String ampGroupDesc = "AMPLIFIER";
const juce::String ampAttackSliderDesc = "ATTACK";
const juce::String ampDecaySliderDesc = "DECAY";
const juce::String ampSustainSliderDesc = "SUSTAIN";
const juce::String ampReleaseSliderDesc = "RELEASE";

const juce::String lfoGroupDesc = "LFO";
const juce::String lfoShapeDesc = "SHAPE";
const juce::String lfoDestDesc = "DEST";
const juce::String lfoFreqSliderDesc = "FREQUENCY";
const juce::String lfoAmountSliderDesc = "AMOUNT";

#if 0
const juce::String effectGroupDesc = "EFFECT";
const juce::String effectParam1Desc = "PARAM 1";
const juce::String effectParam2Desc = "PARAM 2";
#else
const juce::String effectGroupDesc = "REVERB";
const juce::String effectParam1Desc = "ROOM";
const juce::String effectParam2Desc = "MIX";
#endif

const juce::String masterGainDesc = "MASTER VOLUME";
}

namespace sBMP4AudioProcessorChoices
{
    const juce::String oscShape0 = "None";
    const juce::String oscShape1 = "Sawtooth";
    const juce::String oscShape2 = "Saw + Tri";
    const juce::String oscShape3 = "Triangle";
    const juce::String oscShape4 = "Pulse";

    const juce::String lfoShape0 = "Triangle";
    const juce::String lfoShape1 = "Sawtooth";
    //const juce::String lfoShape2 = "Rev Saw";
    const juce::String lfoShape3 = "Square";
    const juce::String lfoShape4 = "Random";

    const juce::String lfoDest0 = "Osc1 Freq";
    const juce::String lfoDest1 = "Osc2 Freq";
    const juce::String lfoDest2 = "Cutoff";
    const juce::String lfoDest3 = "Resonance";
}

inline bool getVarAsBool (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<bool> (v.getProperty (id)); }
inline int getVarAsInt (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<int> (v.getProperty (id)); }
inline juce::int64 getVarAsInt64 (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<juce::int64> (v.getProperty (id)); }
inline double getVarAsDouble (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<double> (v.getProperty (id)); }
inline float getVarAsFloat (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<float> (getVarAsDouble (v, id)); }
inline juce::String getVarAsString (const juce::ValueTree& v, const juce::Identifier& id) { return v.getProperty (id).toString(); }
inline juce::Identifier getVarAsIdentifier (const juce::ValueTree& v, const juce::Identifier& id) { return static_cast<juce::Identifier> (v.getProperty (id)); }

namespace Helpers
{
    inline juce::Image getImage (const void* imageData, const int dataSize)
    {
        return juce::ImageCache::getFromMemory (imageData, dataSize);
    }

    inline std::unique_ptr<juce::Drawable> getDrawable (const void* imageData, const int dataSize)
    {
        return juce::Drawable::createFromImageData (imageData, dataSize);
    }

    inline float getRangedParamValue (juce::AudioProcessorValueTreeState& state, juce::StringRef id)
    {
        auto param = state.getParameter (id);
        return param == nullptr ? 0.f : param->convertFrom0to1 (param->getValue());
    }

    inline double getDoubleMidiNoteInHertz (const double noteNumber, const double frequencyOfA = 440.0) noexcept
    {
        return frequencyOfA * std::pow (2.0, (noteNumber - 69.0) / 12.0);
    }

    inline float getFloatMidiNoteInHertz (const float noteNumber, const float frequencyOfA = 440.0) noexcept
    {
        return frequencyOfA * (float) std::pow (2.0, (noteNumber - 69.0) / 12.0);
    }

    template <typename Type>
    inline bool valueContainedInRange (Type value, juce::NormalisableRange<Type> range)
    {
        return value >= range.start && value <= range.end;
    }

    inline bool areSameSpecs (const juce::dsp::ProcessSpec& spec1, const juce::dsp::ProcessSpec& spec2)
    {
        return spec1.maximumBlockSize == spec2.maximumBlockSize
            && spec1.numChannels == spec2.numChannels
            && spec1.sampleRate == spec2.sampleRate;
    }

    template <class T>
    inline bool areSame (T a, T b, T e = std::numeric_limits <T>::epsilon())
    {
        return fabs (a - b) <= e;
    }
}

#if 0
auto addChoicesToComboBox = [this](juce::Array<ComboBox*> combos, juce::Array<juce::StringRef> comboIDs)
{
    jassert (combos.size() == comboIDs.size());

    for (int i = 0; i < combos.size(); ++i)
    {
        auto comboParam = (juce::AudioParameterChoice*) processor.state.getParameter (comboIDs[i]);
        jassert (comboParam != nullptr);

        int j = 1;
        for (auto choice : comboParam->choices)
            combos[i]->addItem (choice, j++);

        combos[i]->setSelectedItemIndex (0, sendNotification);
    }
};

addChoicesToComboBox ({ /*&oscCombo,*/ &lfoCombo}, { /*osc1ShapeID, */lfoShapeID});
#endif
