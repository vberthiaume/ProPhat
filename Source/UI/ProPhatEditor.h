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

#include "../DSP/ProPhatProcessor.h"

#include "ButtonGroupComponent.h"
#include "ProPhatLookAndFeel.h"
#include "SliderLabel.h"
#include "SnappingSlider.h"

/** The main editor for the plugin.
*/
class ProPhatEditor : public juce::AudioProcessorEditor
                    , public juce::AsyncUpdater
                    , public ProPhatProcessor::MidiMessageListener
#if USE_NATIVE_TITLE_BAR
    , private juce::Button::Listener
#endif
#if CPU_USAGE
    , public Timer
#endif
{
public:
    ProPhatEditor (ProPhatProcessor&);
    ~ProPhatEditor ();

    void paint (juce::Graphics&) override;
    void resized() override;
    void receivedMidiMessage (juce::MidiBuffer& midiMessages) override;
    void handleAsyncUpdate () override;

#if CPU_USAGE
    void timerCallback() override
    {
       auto stats = processor.perfCounter.getStatisticsAndReset();
       cpuUsageText.setText (juce::String (stats.averageSeconds, 6), juce::dontSendNotification);
    }
#endif

private:
    ProPhatProcessor& processor;

#if USE_NATIVE_TITLE_BAR
    void buttonClicked (juce::Button*) override;
#endif

    ProPhatLookAndFeel lnf;
    juce::SharedResourcePointer<SharedFonts> fonts;

#if USE_NATIVE_TITLE_BAR
    juce::TextButton optionsButton;

    static void menuCallback (int result, ProPhatEditor* editor)
    {
        if (editor && result != 0)
            editor->handleMenuResult (result);
    }

    void handleMenuResult (int result);
#endif

    juce::GroupComponent oscGroup, filterGroup, ampGroup, lfoGroup, effectGroup;

#if USE_BACKGROUND_IMAGE
    juce::Image backgroundTexture;
#endif

    juce::AttributedString logoText;
    juce::TextLayout logoTextLayout;
    juce::Rectangle<float> logoBounds;
    juce::Rectangle<float> midiInputBounds;

    //OSCILLATORS
    SliderLabel osc1FreqSliderLabel, osc1TuningSliderLabel, osc2FreqSliderLabel, osc2TuningSliderLabel, oscSubSliderLabel, oscMixSliderLabel, oscNoiseSliderLabel, oscSlopSliderLabel;
    SnappingSlider osc1FreqSlider, osc2FreqSlider, osc1TuningSlider, osc2TuningSlider, oscSubSlider, oscMixSlider, oscNoiseSlider, oscSlopSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment osc1FreqAttachment, osc2FreqAttachment, osc1TuningAttachment, osc2TuningAttachment,
                                                         oscSubAttachment, oscMixAttachment, oscNoiseAttachment, oscSlopAttachment;

    ButtonGroupComponent osc1ShapeButtons, osc2ShapeButtons;

    //FILTER
    SliderLabel filterCutoffLabel, filterResonanceLabel, filterEnvAttackLabel, filterEnvDecayLabel, filterEnvSustainLabel, filterEnvReleaseLabel;
    SnappingSlider filterCutoffSlider, filterResonanceSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment filterCutoffAttachment, filterResonanceAttachment;
    SnappingSlider filterEnvAttackSlider, filterEnvDecaySlider, filterEnvSustainSlider, filterEnvReleaseSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment filterEnvAttackAttachment, filterEnvDecayAttachment, filterEnvSustainAttachment, filterEnvReleaseAttachment;

    //AMPLIFIER
    SliderLabel ampAttackLabel, ampDecayLabel, ampSustainLabel, ampReleaseLabel;
    SnappingSlider ampAttackSlider, ampDecaySlider, ampSustainSlider, ampReleaseSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment ampAttackAttachment, ampDecayAttachment, ampSustainAttachment, ampReleaseAttachment;

    //LFO
    ButtonGroupComponent lfoShapeButtons;
    ButtonGroupComponent lfoDestButtons;

    SliderLabel lfoFreqLabel, lfoAmountLabel;
    SnappingSlider lfoFreqSlider, lfoAmountSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment lfoFreqAttachment, lfoAmountAttachment;

    //EFFECT
    SliderLabel effectParam1Label, effectParam2Label;
    SnappingSlider effectParam1Slider, effectParam2Slider;
    juce::AudioProcessorValueTreeState::SliderAttachment effectParam1Attachment, effectParam2Attachment;

    //OTHER
    SliderLabel masterGainLabel;
    SnappingSlider masterGainSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment masterGainAttachment;

#if CPU_USAGE
    juce::Label cpuUsageLabel;
    juce::Label cpuUsageText;
#endif

    bool gotMidi { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProPhatEditor)
};
