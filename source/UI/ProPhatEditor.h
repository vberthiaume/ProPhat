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
                    , public juce::AudioProcessorValueTreeState::Listener
#if USE_NATIVE_TITLE_BAR
    , private juce::Button::Listener
#endif
#if CPU_USAGE
    , public Timer
#endif
{
public:
    ProPhatEditor (ProPhatProcessor&);
    ~ProPhatEditor () override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void receivedMidiMessage (juce::MidiBuffer& midiMessages) override;
    void handleAsyncUpdate () override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

#if CPU_USAGE
    void timerCallback() override
    {
       auto stats = phatProcessor.perfCounter.getStatisticsAndReset();
       cpuUsageText.setText (juce::String (stats.averageSeconds, 6), juce::dontSendNotification);
    }
#endif

private:
    ProPhatProcessor& phatProcessor;

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
    SliderLabel effectParam1Label, effectParam2Label, tempLabel;
    SnappingSlider effectParam1Slider, effectParam2Slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> effectParam1Attachment, effectParam2Attachment;
    ButtonGroupComponent effectChangeButton;

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
