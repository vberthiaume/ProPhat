#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================

class SnappingSlider : public Slider
{
public:
    SnappingSlider (const SliderStyle& style = Slider::LinearHorizontal, double snapValue = 0.0, double snapTolerance = 0.15) :
        Slider (style, TextEntryBoxPosition::NoTextBox)
    {
        snapVal = snapValue;
        tolerance = snapTolerance;
    }

    double snapValue (double attemptedValue, DragMode) override
    {
        return std::abs (snapVal - attemptedValue) < tolerance ? snapVal : attemptedValue;
    }

private:
    double snapVal;     // The value of the slider at which to snap.
    double tolerance;   // The proximity (in proportion of the slider length) to the snap point before snapping.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnappingSlider)
};

//==============================================================================
/**
*/
class sBMP4AudioProcessorEditor : public AudioProcessorEditor
#if CPU_USAGE
    , public Timer
#endif
{
public:

    sBMP4AudioProcessorEditor (sBMP4AudioProcessor&);
    ~sBMP4AudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

#if CPU_USAGE
    void timerCallback() override
    {
       auto stats = processor.perfCounter.getStatisticsAndReset();
       cpuUsageText.setText (String (stats.averageSeconds, 6), dontSendNotification);
    }
#endif

private:
    sBMP4AudioProcessor& processor;

    Label oscChoiceLabel, oscGainLabel, filterGainLabel, envelopeGainLabel, lfoChoiceLabel, lfoGainLabel;

    SnappingSlider oscSlider, filterSlider, envelopeSlider, lfoSlider;
    AudioProcessorValueTreeState::SliderAttachment oscSliderAttachment, filterSliderAttachment, envelopeSliderAttachment, lfoSliderAttachment;

    ComboBox oscCombo, lfoCombo;
    AudioProcessorValueTreeState::ComboBoxAttachment oscComboAttachment, formatComboAttachment;

    ToggleButton oscEnableButton, oscWavetableButton, filterEnableButton, envelopeEnableButton;
    AudioProcessorValueTreeState::ButtonAttachment oscEnableButtonAttachment;
    AudioProcessorValueTreeState::ButtonAttachment oscWaveTableButtonAttachment;
    AudioProcessorValueTreeState::ButtonAttachment filterEnableButtonAttachment;
    AudioProcessorValueTreeState::ButtonAttachment envelopeEnableButtonAttachment;

    GroupComponent oscSection, filterSection, envelopeSection, lfoSection;

#if CPU_USAGE
    Label cpuUsageLabel;
    Label cpuUsageText;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
