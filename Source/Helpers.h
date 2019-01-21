/*
  ==============================================================================

    Helpers.h
    Created: 21 Jan 2019 11:52:58am
    Author:  Haake

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

inline bool getVarAsBool (const ValueTree& v, const Identifier& id) { return static_cast<bool> (v.getProperty (id)); }
inline int getVarAsInt (const ValueTree& v, const Identifier& id) { return static_cast<int> (v.getProperty (id)); }
inline int64 getVarAsInt64 (const ValueTree& v, const Identifier& id) { return static_cast<int64> (v.getProperty (id)); }
inline double getVarAsDouble (const ValueTree& v, const Identifier& id) { return static_cast<double> (v.getProperty (id)); }
inline float getVarAsFloat (const ValueTree& v, const Identifier& id) { return static_cast<float> (getVarAsDouble (v, id)); }
inline String getVarAsString (const ValueTree& v, const Identifier& id) { return v.getProperty (id).toString(); }
inline Identifier getVarAsIdentifier (const ValueTree& v, const Identifier& id) { return static_cast<Identifier> (v.getProperty (id)); }