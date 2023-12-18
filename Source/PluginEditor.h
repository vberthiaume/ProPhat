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
#include "PluginProcessor.h"
#include "ButtonGroupComponent.h"
#include "sBMP4LookAndFeel.h"

//==============================================================================

class SnappingSlider : public juce::Slider
{
public:
    SnappingSlider (const SliderStyle& style = juce::Slider::RotaryVerticalDrag, double snapValue = 0.0, double snapTolerance = 0.15) :
        juce::Slider (style, TextEntryBoxPosition::NoTextBox), 
        snapVal (snapValue),
        tolerance (snapTolerance)
    {
        setPopupDisplayEnabled (true, false, nullptr);
    }

    //@TODO reactivate this when needed, it's causing issues
    //double snapValue (double attemptedValue, DragMode) override
    //{
    //    return std::abs (snapVal - attemptedValue) < tolerance ? snapVal : attemptedValue;
    //}

private:
    double snapVal;     // The value of the slider at which to snap.
    double tolerance;   // The proximity (in proportion of the slider length) to the snap point before snapping.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnappingSlider)
};

/** The main labels used in the plugin.
*/
class sBMP4Label : public juce::Label
{
public:
    sBMP4Label ()
    {
        setJustificationType (juce::Justification::centredBottom);
        setFont (sharedFonts->regular.withHeight (labelFontHeight));
        setMinimumHorizontalScale (1.f);
    }

    void componentMovedOrResized (juce::Component& component, bool /*wasMoved*/, bool /*wasResized*/) override
    {
        auto& lf = getLookAndFeel();
        auto f = lf.getLabelFont (*this);
        auto borderSize = lf.getLabelBorderSize (*this);

        auto height = borderSize.getTopAndBottom() + 6 + juce::roundToInt (f.getHeight() + 0.5f);
        setBounds (component.getX(), component.getY() + component.getHeight() - height + 7, component.getWidth(), height);
    }

private:
    juce::SharedResourcePointer<SharedFonts> sharedFonts;
};

//==============================================================================
/**
*/
class sBMP4AudioProcessorEditor : public juce::AudioProcessorEditor
#if CPU_USAGE
    , public Timer
#endif
{
public:
    sBMP4AudioProcessorEditor (sBMP4AudioProcessor&);
    ~sBMP4AudioProcessorEditor () { setLookAndFeel (nullptr); }

    void paint (juce::Graphics&) override;
    void resized() override;

#if CPU_USAGE
    void timerCallback() override
    {
       auto stats = processor.perfCounter.getStatisticsAndReset();
       cpuUsageText.setText (juce::String (stats.averageSeconds, 6), juce::dontSendNotification);
    }
#endif

private:
    sBMP4AudioProcessor& processor;

    sBMP4LookAndFeel lnf;
    juce::SharedResourcePointer<SharedFonts> sharedFonts;

    juce::GroupComponent oscGroup, filterGroup, ampGroup, lfoGroup, effectGroup;

    juce::Image backgroundTexture;

    //OSCILLATORS
    sBMP4Label osc1FreqSliderLabel, osc1TuningSliderLabel, osc2FreqSliderLabel, osc2TuningSliderLabel, oscSubSliderLabel, oscMixSliderLabel, oscNoiseSliderLabel, oscSlopSliderLabel;
    SnappingSlider osc1FreqSlider, osc2FreqSlider, osc1TuningSlider, osc2TuningSlider, oscSubSlider, oscMixSlider, oscNoiseSlider, oscSlopSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment osc1FreqAttachment, osc2FreqAttachment, osc1TuningAttachment, osc2TuningAttachment,
                                                         oscSubAttachment, oscMixAttachment, oscNoiseAttachment, oscSlopAttachment;

    ButtonGroupComponent osc1ShapeButtons, osc2ShapeButtons;

    //FILTER
    sBMP4Label filterCutoffLabel, filterResonanceLabel, filterEnvAttackLabel, filterEnvDecayLabel, filterEnvSustainLabel, filterEnvReleaseLabel;
    SnappingSlider filterCutoffSlider, filterResonanceSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment filterCutoffAttachment, filterResonanceAttachment;
    SnappingSlider filterEnvAttackSlider, filterEnvDecaySlider, filterEnvSustainSlider, filterEnvReleaseSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment filterEnvAttackAttachment, filterEnvDecayAttachment, filterEnvSustainAttachment, filterEnvReleaseAttachment;

    //AMPLIFIER
    sBMP4Label ampAttackLabel, ampDecayLabel, ampSustainLabel, ampReleaseLabel;
    SnappingSlider ampAttackSlider, ampDecaySlider, ampSustainSlider, ampReleaseSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment ampAttackAttachment, ampDecayAttachment, ampSustainAttachment, ampReleaseAttachment;

    //LFO
    ButtonGroupComponent lfoShapeButtons;
    ButtonGroupComponent lfoDestButtons;

    sBMP4Label lfoFreqLabel, lfoAmountLabel;
    SnappingSlider lfoFreqSlider, lfoAmountSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment lfoFreqAttachment, lfoAmountAttachment;

    //EFFECT
    sBMP4Label effectParam1Label, effectParam2Label;
    SnappingSlider effectParam1Slider, effectParam2Slider;
    juce::AudioProcessorValueTreeState::SliderAttachment effectParam1Attachment, effectParam2Attachment;

    //OTHER
    sBMP4Label masterGainLabel;
    SnappingSlider masterGainSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment masterGainAttachment;

#if CPU_USAGE
    juce::Label cpuUsageLabel;
    juce::Label cpuUsageText;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
