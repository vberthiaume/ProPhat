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
        //so this is only available to audio applications. Jules says to use a PerformanceCounter
        //auto cpu = deviceManager.getCpuUsage() * 100;

       auto stats = processor.perfCounter.getStatisticsAndReset();
       stats.toString();
        //cpuUsageText.setText (String (cpu, 6) + " %", dontSendNotification);
       cpuUsageText.setText (stats.toString(), dontSendNotification);
    }
#endif

private:

    sBMP4AudioProcessor& processor;

    Label oscillatorsChoiceLabel, oscillatorsGainLabel, filterGainLabel, envelopeGainLabel, lfoChoiceLabel, lfoGainLabel;

    SnappingSlider oscillatorsSlider, filterSlider, envelopeSlider, lfoSlider;
    AudioProcessorValueTreeState::SliderAttachment oscillatorsSliderAttachment, filterSliderAttachment, envelopeSliderAttachment, lfoSliderAttachment;

    ComboBox oscillatorsCombo, lfoCombo;
    AudioProcessorValueTreeState::ComboBoxAttachment oscillatorsComboAttachment, formatComboAttachment;

    ToggleButton oscillatorsButton, filterButton, envelopeButton;
    AudioProcessorValueTreeState::ButtonAttachment oscillatorsButtonAttachment, filterButtonAttachment, envelopeButtonAttachment;

    GroupComponent oscillatorsSection, filterSection, envelopeSection, lfoSection;

#if CPU_USAGE

    Label cpuUsageLabel;
    Label cpuUsageText;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
