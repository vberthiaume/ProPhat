/*
  ==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2024 Vincent Berthiaume

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#pragma once
#include "BinaryData.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

namespace Constants
{
constexpr auto phatGrey                 { 0xff191919 };

constexpr auto groupComponentFontHeight { 24 };
constexpr auto labelFontHeight          { 22 };
constexpr auto buttonFontHeight         { 16.f };
constexpr auto buttonSelectorFontHeight { 18 };

constexpr auto oscShapeRadioGroupId     { 1 };

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
constexpr auto minAmp                   { .000001f };

constexpr auto defaultAmpA              { minAmp };
constexpr auto defaultAmpD              { minAmp };
constexpr auto defaultAmpS              { 1.f };
constexpr auto defaultAmpR              { .25f };

constexpr auto sustainSkewFactor        { .5f };
constexpr auto ampSkewFactor            { .5f };
constexpr auto cutOffSkewFactor         { .5f };
constexpr auto slopSkewFactor           { .5f };

const juce::NormalisableRange<float> attackRange        { minAmp, 25.f, 0.f, ampSkewFactor };
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

//====================================================================================================

namespace ProPhatParameterIds
{
const juce::ParameterID osc1ShapeID        { "Osc 1 Shape", 1 };
const juce::ParameterID osc1FreqID         { "Osc 1 Freq", 1 };
const juce::ParameterID osc1TuningID       { "Osc 1 Tuning", 1 };

const juce::ParameterID osc2ShapeID        { "Osc 2 Shape", 1 };
const juce::ParameterID osc2FreqID         { "Osc 2 Freq", 1 };
const juce::ParameterID osc2TuningID       { "Osc 2 Tuning", 1 };

const juce::ParameterID oscSubID           { "Osc Sub", 1 };
const juce::ParameterID oscMixID           { "Osc Mix", 1 };
const juce::ParameterID oscNoiseID         { "Osc Noise", 1 };
const juce::ParameterID oscSlopID          { "Osc Slop", 1 };

const juce::ParameterID filterCutoffID     { "Filter Cutoff", 1 };
const juce::ParameterID filterResonanceID  { "Filter Reso", 1 };

const juce::ParameterID ampAttackID        { "Amp Attack", 1 };
const juce::ParameterID ampDecayID         { "Amp Decay", 1 };
const juce::ParameterID ampSustainID       { "Amp Sustain", 1 };
const juce::ParameterID ampReleaseID       { "Amp Release", 1 };

const juce::ParameterID filterEnvAttackID  { "Filter Attack", 1 };
const juce::ParameterID filterEnvDecayID   { "Filter Decay", 1 };
const juce::ParameterID filterEnvSustainID { "Filter Sustain", 1 };
const juce::ParameterID filterEnvReleaseID { "Filter Release", 1 };

const juce::ParameterID lfoShapeID         { "Lfo Shape", 1 };
const juce::ParameterID lfoDestID          { "Lfo Dest", 1 };
const juce::ParameterID lfoFreqID          { "Lfo Freq", 1 };
const juce::ParameterID lfoAmountID        { "Lfo Amount", 1 };

const juce::ParameterID effectParam1ID     { "Effect Param1", 1 };
const juce::ParameterID effectParam2ID     { "Effect Param2", 1 };
const juce::ParameterID effectSelectedID   { "Current Effect", 1 };

const juce::ParameterID masterGainID       { "Master Gain", 1 };
}

//====================================================================================================

namespace ProPhatParameterLabels
{
constexpr auto oscGroupDesc                 { "OSCILLATORS" };
constexpr auto osc1ShapeDesc                { "SHAPE" };
constexpr auto osc2ShapeDesc                { "SHAPE" };
constexpr auto osc1FreqDesc                 { "OSC 1 FREQ" };
constexpr auto osc2FreqDesc                 { "OSC 2 FREQ" };
constexpr auto osc1TuningDesc               { "FINE TUNE" };
constexpr auto osc2TuningDesc               { "FINE TUNE" };

constexpr auto oscSubOctDesc                { "SUB OCTAVE" };
constexpr auto oscMixDesc                   { "OSC MIX" };
constexpr auto oscNoiseDesc                 { "NOISE" };
constexpr auto oscSlopDesc                  { "OSC SLOP" };

constexpr auto filterGroupDesc              { "LOW-PASS FILTER" };
constexpr auto filterCutoffSliderDesc       { "CUTOFF" };
constexpr auto filterResonanceSliderDesc    { "RESONANCE" };

constexpr auto ampGroupDesc                 { "AMPLIFIER" };
constexpr auto ampAttackSliderDesc          { "ATTACK" };
constexpr auto ampDecaySliderDesc           { "DECAY" };
constexpr auto ampSustainSliderDesc         { "SUSTAIN" };
constexpr auto ampReleaseSliderDesc         { "RELEASE" };

constexpr auto lfoGroupDesc                 { "LFO" };
constexpr auto lfoShapeDesc                 { "SHAPE" };
constexpr auto lfoDestDesc                  { "DEST" };
constexpr auto lfoFreqSliderDesc            { "FREQUENCY" };
constexpr auto lfoAmountSliderDesc          { "AMOUNT" };

#if 1
constexpr auto effectGroupDesc              { "EFFECT" };
constexpr auto effectParam1Desc             { "PARAM 1" };
constexpr auto effectParam2Desc             { "PARAM 2" };
#else
constexpr auto effectGroupDesc              { "REVERB" };
constexpr auto effectParam1Desc             { "ROOM" };
constexpr auto effectParam2Desc             { "MIX" };
#endif

constexpr auto masterGainDesc               { "MASTER VOL" };
}

//====================================================================================================

namespace ProPhatAudioProcessorChoices
{
constexpr auto oscShape0    { "None" };
constexpr auto oscShape1    { "Sawtooth" };
constexpr auto oscShape2    { "Saw + Tri" };
constexpr auto oscShape3    { "Triangle" };
constexpr auto oscShape4    { "Pulse" };

constexpr auto lfoShape0    { "Triangle" };
constexpr auto lfoShape1    { "Sawtooth" };
//constexpr auto lfoShape2    { "Rev Saw" };
constexpr auto lfoShape3    { "Square" };
constexpr auto lfoShape4    { "Random" };

constexpr auto lfoDest0     { "Osc1 Freq" };
constexpr auto lfoDest1     { "Osc2 Freq" };
constexpr auto lfoDest2     { "Cutoff" };
constexpr auto lfoDest3     { "Resonance" };

constexpr auto effect0      { "None" };
constexpr auto effect1      { "Reverb" };
constexpr auto effect2      { "Chorus" };
constexpr auto effect3      { "Phaser" };
}

struct Selection
{
    Selection () = default;
    Selection (int selection) : curSelection (selection) {}

    int curSelection = 0;

    //TODO: does this need to be virtual? When do we need a custom behaviour for this?
    //ah it's because totalSelectable is an enum value :hmm: I wonder how we could get around this
    // virtual int getLastSelectionIndex () { return totalSelectable - 1; };
    virtual int getLastSelectionIndex () = 0;
    virtual bool isNullSelectionAllowed () = 0;
};

struct OscShape : public Selection
{
    enum Values
    {
        none = 0,
        saw,
        sawTri,
        triangle,
        pulse,
        totalSelectable,
        noise // noise needs to be after totalSelectable, because it's not selectable with the regular oscillators
    };

    int getLastSelectionIndex () override { return totalSelectable - 1; };
    bool isNullSelectionAllowed () override { return true; }
};

struct LfoShape : public Selection
{
    enum
    {
        triangle = 0,
        saw,
        //revSaw,
        square,
        random,
        totalSelectable
    };

    int getLastSelectionIndex () override { return totalSelectable - 1; };
    bool isNullSelectionAllowed () override { return false; }
};

struct LfoDest : public Selection
{
    enum
    {
        osc1Freq = 0,
        osc2Freq,
        filterCutOff,
        filterResonance,
        totalSelectable
    };

    int getLastSelectionIndex () override { return totalSelectable - 1; };
    bool isNullSelectionAllowed () override { return false; }
};

struct SelectedEffect : public Selection
{
    //this is essentially like EffectType, so we still need that enum?
    enum
    {
        none = 0,
        verb,
        chorus,
        phaser,
        totalSelectable
    };

    int getLastSelectionIndex () override { return totalSelectable - 1; };
    bool isNullSelectionAllowed () override { return true; }
};

//====================================================================================================

/** This struct can be used to have a single shared font object throughout the plugin. To use it somewhere,
*   just create a @code juce::SharedResourcePointer<SharedFonts> fonts @endcode.
*/
struct SharedFonts final
{
    juce::Font regular { juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsMedium_ttf, BinaryData::PoppinsMedium_ttfSize) };
    juce::Font thin    { juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsThin_ttf, BinaryData::PoppinsThin_ttfSize) };
    juce::Font bold    { juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsBlack_ttf, BinaryData::PoppinsBlack_ttfSize) };

    juce::Font getRegularFont (float h) { return regular.withHeight (h); }
    juce::Font getThinFont (float h)    { return thin.withHeight (h); }
    juce::Font getBoldFont (float h)    { return bold.withHeight (h); }
};

//====================================================================================================

namespace Helpers
{
inline juce::Image getImage (const void* imageData, const int dataSize)
{
    return juce::ImageCache::getFromMemory (imageData, dataSize);
}

inline std::unique_ptr<juce::Drawable> getDrawable (const void* imageData, const size_t dataSize)
{
    return juce::Drawable::createFromImageData (imageData, dataSize);
}

inline float getRangedParamValue (juce::AudioProcessorValueTreeState& state, juce::StringRef id)
{
    auto param = state.getParameter (id);
    return param == nullptr ? 0.f : param->convertFrom0to1 (param->getValue());
}

template <std::floating_point T>
T getMidiNoteInHertz (const T noteNumber, const T frequencyOfA = 440) noexcept
{
    return static_cast<T> (frequencyOfA * std::pow (2, (noteNumber - 69) / 12));
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
        && juce::approximatelyEqual (spec1.sampleRate, spec2.sampleRate);
}

template <class T>
inline bool areSame (T a, T b, T e = std::numeric_limits <T>::epsilon())
{
    return fabs (a - b) <= e;
}

inline void printADSR (juce::StringRef prefix, const juce::ADSR::Parameters& p)
{
    juce::String str { prefix };
    str << " a: " << p.attack;
    str << " d: " << p.decay;
    str << " s: " << p.sustain;
    str << " r: " << p.release << "\n";
    DBG (str);
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
