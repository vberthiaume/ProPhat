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
{
public:

    sBMP4AudioProcessorEditor (sBMP4AudioProcessor&);
    ~sBMP4AudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:

    sBMP4AudioProcessor& processor;

    Label shotgunChoiceLabel, shotgunGainLabel, frontBackGainLabel, upDownGainLabel, bFormatChoiceLabel, outputGainLabel;

    SnappingSlider shotgunSlider, frontBackSlider, upDownSlider, outputSlider;
    AudioProcessorValueTreeState::SliderAttachment shotgunSliderAttachment, frontBackSliderAttachment, upDownSliderAttachment, outputSliderAttachment;

    ComboBox shotgunCombo, bFormatCombo;
    AudioProcessorValueTreeState::ComboBoxAttachment shotgunComboAttachment, formatComboAttachment;

    ToggleButton shotgunButton, frontBackButton, upDownButton;
    AudioProcessorValueTreeState::ButtonAttachment shotgunButtonAttachment, frontBackButtonAttachment, upDownButtonAttachment;

    GroupComponent shotgunSection, frontBackSection, upDownSection, outputSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
