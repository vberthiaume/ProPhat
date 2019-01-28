/*
  ==============================================================================

    Helpers.h
    Created: 21 Jan 2019 11:52:58am
    Author:  Haake

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

static const NormalisableRange<float> dBRange = {-12.f, 12.f};
static const NormalisableRange<float> sliderRange = {0.f, 1.f};
static const NormalisableRange<float> hzRange = {0.1f, 18000.f};
static const NormalisableRange<float> lfoRange = {0.1f, 30.f};

enum Constants
{
    numVoices = 16,

    oscShapeRadioGroupId
};

namespace sBMP4AudioProcessorIDs
{
    const String oscWavetableID = "oscWavetableID";
    const String oscShapeID = "oscShapeID";
    const String osc1FreqID = "osc1FreqID";

    const String filterCutoffID = "filterCutoffID";
    const String filterResonanceID = "filterResonanceID";

    const String ampAttackID = "ampAttackID";
    const String ampDecayID = "ampDecayID";
    const String ampSustainID = "ampSustainID";
    const String ampReleaseID = "ampReleaseID";

    const String lfoShapeID = "lfoShapeID";
    const String lfoFreqID = "lfoFreqID";

    const String effectID = "effectID";
    const String effectParam1ID = "effectParam1ID";
    const String effectParam2ID = "effectParam2ID";
}

namespace sBMP4AudioProcessorNames
{
    const String oscGroupDesc = "OSCILLATORS";
    const String oscWavetableButtonDesc = "WAVETABLE";
    const String oscShapeDesc = "SHAPE";
    const String oscFreqSliderDesc = "OSC 1 FREQ";

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
    const String lfoSliderDesc = "FREQUENCY";

    const String effectGroupDesc = "EFFECT";
    const String effectParam1Desc = "PARAM 1";
    const String effectParam2Desc = "PARAM 2";
}

enum class lfoShape
{
    triangle,
    saw,
    revSaw,
    Square,
    Random,
    total
};

enum class oscShape
{
    saw,
    sawTri,
    triangle,
    pulse,
    total
};

namespace sBMP4AudioProcessorChoices
{
    const String oscShape0 = "Sawtooth";
    const String oscShape1 = "Saw + Tri";
    const String oscShape2 = "Triangle";
    const String oscShape3 = "Pulse";

    const String lfoShape0 = "Triangle";
    const String lfoShape1 = "Sawtooth";
    const String lfoShape2 = "Rev Saw";
    const String lfoShape3 = "Square";
    const String lfoShape4 = "Random";

    const String effectChoices0 = "Reverb";
    const String effectChoices1 = "None";
}

inline bool getVarAsBool (const ValueTree& v, const Identifier& id) { return static_cast<bool> (v.getProperty (id)); }
inline int getVarAsInt (const ValueTree& v, const Identifier& id) { return static_cast<int> (v.getProperty (id)); }
inline int64 getVarAsInt64 (const ValueTree& v, const Identifier& id) { return static_cast<int64> (v.getProperty (id)); }
inline double getVarAsDouble (const ValueTree& v, const Identifier& id) { return static_cast<double> (v.getProperty (id)); }
inline float getVarAsFloat (const ValueTree& v, const Identifier& id) { return static_cast<float> (getVarAsDouble (v, id)); }
inline String getVarAsString (const ValueTree& v, const Identifier& id) { return v.getProperty (id).toString(); }
inline Identifier getVarAsIdentifier (const ValueTree& v, const Identifier& id) { return static_cast<Identifier> (v.getProperty (id)); }

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

addChoicesToComboBox ({ /*&oscCombo,*/ &lfoCombo}, { /*oscShapeID, */lfoShapeID});
#endif
