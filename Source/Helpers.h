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

#include "../JuceLibraryCode/JuceHeader.h"

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
    enum
    {
        oscShapeRadioGroupId = 1,

        defaultSubOsc = 0,
        defaultOscMix = 0,
        defaultOscTuning = 0,

        numVoices = 16,
        defaultOscMidiNote = 48,    //C2 on rev2
        middleCMidiNote = 60,       //C3 on rev2

        killRampSamples = 300,
        rampUpSamples = 100
    };

    static const auto defaultOscLevel = .4f;

    static const auto defaultFilterCutoff = 1000.f;
    static const auto defaultFilterResonance = .5f;

    static const float defaultLfoFreq = 3.f;
    static const float defaultLfoAmount = 0.f;

    static const float defaultEffectParam1 = 0.f;
    static const float defaultEffectParam2 = 0.f;

    static const NormalisableRange<float> dBRange = {-12.f, 12.f};
    static const NormalisableRange<float> sliderRange = {0.f, 1.f};
    static const NormalisableRange<float> centeredSliderRange = {-0.5f, .5f};

    //envelope stuff
    static const auto minA = .001f;
    static const auto minAmp = .01f;

    static const auto defaultAmpA = minA;
    static const auto defaultAmpD = minAmp;
    static const auto defaultAmpS = 1.f;
    static const auto defaultAmpR = .25f;

    static const auto sustainSkewFactor = .5f;
    static const auto ampSkewFactor = .5f;
    static const auto cutOffSkewFactor = .5f;

    static const NormalisableRange<float> attackRange   {minA, 25.f, 0.f, ampSkewFactor};
    static const NormalisableRange<float> decayRange    {minAmp, 25.f, 0.f, ampSkewFactor};
    static const NormalisableRange<float> sustainRange  {minAmp, 1.f,  0.f, sustainSkewFactor};
    static const NormalisableRange<float> releaseRange  {minAmp, 25.f, 0.f, ampSkewFactor};

    static const NormalisableRange<float> cutOffRange = {0.1f, 18000.f, 0.f, cutOffSkewFactor};
    static const NormalisableRange<float> lfoRange = {0.1f, 10.f};
    static const NormalisableRange<float> lfoNoteRange = {0.f, 16.f};

    //Sets the base frequency of Oscillator 1 or 2 over a 9-octave
    //range from 16 Hz to 8KHz (when used with the Transpose buttons). Adjustment is in semitones.
    static const NormalisableRange<int> midiNoteRange = {12, 120};   //actual midi note range is (0,127), but rev2, at least for oscilators is C0(0) to C10(120)
    static const NormalisableRange<float> pitchWheelNoteRange = {-7.f, 7.f};
}

namespace sBMP4AudioProcessorIDs
{
    const String osc1ShapeID = "osc1ShapeID";
    const String osc1FreqID = "osc1FreqID";
    const String osc1TuningID = "osc1TuningID";

    const String osc2ShapeID = "osc2ShapeID";
    const String osc2FreqID = "osc2FreqID";
    const String osc2TuningID = "osc2TuningID";

    const String oscSubID = "oscSubID";
    const String oscMixID = "oscMixID";

    const String filterCutoffID = "filterCutoffID";
    const String filterResonanceID = "filterResonanceID";

    const String ampAttackID = "ampAttackID";
    const String ampDecayID = "ampDecayID";
    const String ampSustainID = "ampSustainID";
    const String ampReleaseID = "ampReleaseID";

    const String filterEnvAttackID = "filterEnvAttackID";
    const String filterEnvDecayID = "filterEnvDecayID";
    const String filterEnvSustainID = "filterEnvSustainID";
    const String filterEnvReleaseID = "filterEnvReleaseID";

    const String lfoShapeID = "lfoShapeID";
    const String lfoDestID = "lfoDestID";
    const String lfoFreqID = "lfoFreqID";
    const String lfoAmountID    = "lfoAmountID";

    const String effectID       = "effectID";
    const String effectParam1ID = "effectParam1ID";
    const String effectParam2ID = "effectParam2ID";
}

namespace sBMP4AudioProcessorNames
{
    const String oscGroupDesc = "OSCILLATORS";
    const String osc1ShapeDesc = "SHAPE";
    const String osc2ShapeDesc = "SHAPE";
    const String osc1FreqDesc = "OSC 1 FREQ";
    const String osc2FreqDesc = "OSC 2 FREQ";
    const String osc1TuningDesc = "FINE TUNE";
    const String osc2TuningDesc = "FINE TUNE";

    const String oscSubOctDesc = "SUB OCTAVE";
    const String oscMixDesc = "OSC MIX";

    const String filterGroupDesc = "LOW-PASS FILTER";
    const String filterCutoffSliderDesc = "CUTOFF";
    const String filterResonanceSliderDesc = "RESONANCE";

    const String ampGroupDesc = "AMPLIFIER";
    const String ampAttackSliderDesc = "ATTACK";
    const String ampDecaySliderDesc = "DECAY";
    const String ampSustainSliderDesc = "SUSTAIN";
    const String ampReleaseSliderDesc = "RELEASE";

    const String lfoGroupDesc = "LFO";
    const String lfoShapeDesc = "SHAPE";
    const String lfoDestDesc = "DEST";
    const String lfoFreqSliderDesc = "FREQUENCY";
    const String lfoAmountSliderDesc = "AMOUNT";

#if 0
    const String effectGroupDesc = "EFFECT";
    const String effectParam1Desc = "PARAM 1";
    const String effectParam2Desc = "PARAM 2";
#else
    const String effectGroupDesc = "REVERB";
    const String effectParam1Desc = "ROOM";
    const String effectParam2Desc = "MIX";
#endif
}

namespace sBMP4AudioProcessorChoices
{
    const String oscShape0 = "None";
    const String oscShape1 = "Sawtooth";
    const String oscShape2 = "Saw + Tri";
    const String oscShape3 = "Triangle";
    const String oscShape4 = "Pulse";

    const String lfoShape0 = "Triangle";
    const String lfoShape1 = "Sawtooth";
    //const String lfoShape2 = "Rev Saw";
    const String lfoShape3 = "Square";
    const String lfoShape4 = "Random";

    const String lfoDest0 = "Osc1 Freq";
    const String lfoDest1 = "Osc2 Freq";
    const String lfoDest2 = "Cutoff";
    const String lfoDest3 = "Resonance";
}

inline bool getVarAsBool (const ValueTree& v, const Identifier& id) { return static_cast<bool> (v.getProperty (id)); }
inline int getVarAsInt (const ValueTree& v, const Identifier& id) { return static_cast<int> (v.getProperty (id)); }
inline int64 getVarAsInt64 (const ValueTree& v, const Identifier& id) { return static_cast<int64> (v.getProperty (id)); }
inline double getVarAsDouble (const ValueTree& v, const Identifier& id) { return static_cast<double> (v.getProperty (id)); }
inline float getVarAsFloat (const ValueTree& v, const Identifier& id) { return static_cast<float> (getVarAsDouble (v, id)); }
inline String getVarAsString (const ValueTree& v, const Identifier& id) { return v.getProperty (id).toString(); }
inline Identifier getVarAsIdentifier (const ValueTree& v, const Identifier& id) { return static_cast<Identifier> (v.getProperty (id)); }

//@TODO make this a namespace and get rid of all (implicit) static keywords
struct Helpers
{
    static Image getImage (const void* imageData, const int dataSize)
    {
        return ImageCache::getFromMemory (imageData, dataSize);
    }

    static Drawable* getDrawable (const void* imageData, const int dataSize)
    {
        return Drawable::createFromImageData (imageData, dataSize);
    }

    static float getRangedParamValue (AudioProcessorValueTreeState& state, StringRef id)
    {
        auto param = state.getParameter (id);
        return param == nullptr ? 0.f : param->convertFrom0to1 (param->getValue());
    }

    static double getDoubleMidiNoteInHertz (const double noteNumber, const double frequencyOfA = 440.0) noexcept
    {
        return frequencyOfA * std::pow (2.0, (noteNumber - 69.0) / 12.0);
    }

    static float getFloatMidiNoteInHertz (const float noteNumber, const float frequencyOfA = 440.0) noexcept
    {
        return frequencyOfA * (float) std::pow (2.0, (noteNumber - 69.0) / 12.0);
    }

    template <typename Type>
    static bool valueContainedInRange (Type value, NormalisableRange<Type> range)
    {
        return value >= range.start && value <= range.end;
    }

    inline static bool areSameSpecs (const dsp::ProcessSpec& spec1, const dsp::ProcessSpec& spec2)
    {
        return spec1.maximumBlockSize == spec2.maximumBlockSize
            && spec1.numChannels == spec2.numChannels
            && spec1.sampleRate == spec2.sampleRate;
    }

    template <class T>
    static bool areSame (T a, T b, T e = std::numeric_limits <T>::epsilon())
    {
        return fabs (a - b) <= e;
    }
};

#if 0
auto addChoicesToComboBox = [this](Array<ComboBox*> combos, Array<StringRef> comboIDs)
{
    jassert (combos.size() == comboIDs.size());

    for (int i = 0; i < combos.size(); ++i)
    {
        auto comboParam = (AudioParameterChoice*) processor.state.getParameter (comboIDs[i]);
        jassert (comboParam != nullptr);

        int j = 1;
        for (auto choice : comboParam->choices)
            combos[i]->addItem (choice, j++);

        combos[i]->setSelectedItemIndex (0, sendNotification);
    }
};

addChoicesToComboBox ({ /*&oscCombo,*/ &lfoCombo}, { /*osc1ShapeID, */lfoShapeID});
#endif
