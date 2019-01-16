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
class Jero2BAudioProcessorEditor : public AudioProcessorEditor
{
public:

    Jero2BAudioProcessorEditor (Jero2BAudioProcessor&);
    ~Jero2BAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:

    Jero2BAudioProcessor& processor;

    Label shotgunChoiceLabel, shotgunGainLabel, frontBackGainLabel, upDownGainLabel, bFormatChoiceLabel, outputGainLabel;

    SnappingSlider shotgunSlider, frontBackSlider, upDownSlider, outputSlider;
    AudioProcessorValueTreeState::SliderAttachment shotgunSliderAttachment, frontBackSliderAttachment, upDownSliderAttachment, outputSliderAttachment;

    ComboBox shotgunCombo, bFormatCombo;
    AudioProcessorValueTreeState::ComboBoxAttachment shotgunComboAttachment, formatComboAttachment;

    ToggleButton shotgunButton, frontBackButton, upDownButton;
    AudioProcessorValueTreeState::ButtonAttachment shotgunButtonAttachment, frontBackButtonAttachment, upDownButtonAttachment;

    Rectangle<float> shotgunSection, frontBackSection, upDownSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jero2BAudioProcessorEditor)
};
