#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "ButtonGroupComponent.h"

//==============================================================================

class SnappingSlider : public Slider
{
public:
    SnappingSlider (const SliderStyle& style = Slider::RotaryVerticalDrag, double snapValue = 0.0, double snapTolerance = 0.15) :
        Slider (style, TextEntryBoxPosition::NoTextBox), 
        snapVal (snapValue),
        tolerance (snapTolerance)
    {
        setLookAndFeel (&lf);
        /*auto params = getRotaryParameters();
        const auto toAngle = params.startAngleRadians + getValue() * (params.endAngleRadians- params.startAngleRadians);*/
    }

    ~SnappingSlider()
    {
        setLookAndFeel (nullptr);
    }

    double snapValue (double attemptedValue, DragMode) override
    {
        return std::abs (snapVal - attemptedValue) < tolerance ? snapVal : attemptedValue;
    }

private:
    sBMP4LookAndFeel lf;

    double snapVal;     // The value of the slider at which to snap.
    double tolerance;   // The proximity (in proportion of the slider length) to the snap point before snapping.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnappingSlider)
};

class sBMP4Label : public Label
{
    void componentMovedOrResized (Component& component, bool /*wasMoved*/, bool /*wasResized*/) override
    {
        auto& lf = getLookAndFeel();
        auto f = lf.getLabelFont (*this);
        auto borderSize = lf.getLabelBorderSize (*this);

        auto height = borderSize.getTopAndBottom() + 6 + roundToInt (f.getHeight() + 0.5f);
        setBounds (component.getX(), component.getY() + component.getHeight() - height + 7, component.getWidth(), height);
    }
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

    GroupComponent oscGroup, filterGroup, ampGroup, lfoGroup, effectGroup;

    Image backgroundTexture;

    //OSCILLATORS
    ToggleButton oscWavetableButton;
    AudioProcessorValueTreeState::ButtonAttachment oscWaveTableButtonAttachment;
    ButtonGroupComponent oscShapeButtons;

    sBMP4Label oscFreqSliderLabel;
    SnappingSlider oscFreqSlider;
    AudioProcessorValueTreeState::SliderAttachment oscFreqAttachment;

    //FILTER
    sBMP4Label filterCutoffLabel;
    SnappingSlider filterCutoffSlider;
    AudioProcessorValueTreeState::SliderAttachment filterCutoffAttachment;

    sBMP4Label filterResonanceLabel;
    SnappingSlider filterResonanceSlider;
    AudioProcessorValueTreeState::SliderAttachment filterResonanceAttachment;

    //AMPLIFIER
    sBMP4Label ampAttackLabel;
    SnappingSlider ampAttackSlider;
    AudioProcessorValueTreeState::SliderAttachment ampAttackAttachment;

    sBMP4Label ampDecayLabel;
    SnappingSlider ampDecaySlider;
    AudioProcessorValueTreeState::SliderAttachment ampDecayAttachment;

    sBMP4Label ampSustainLabel;
    SnappingSlider ampSustainSlider;
    AudioProcessorValueTreeState::SliderAttachment ampSustainAttachment;

    sBMP4Label ampReleaseLabel;
    SnappingSlider ampReleaseSlider;
    AudioProcessorValueTreeState::SliderAttachment ampReleaseAttachment;

    //LFO
    ButtonGroupComponent lfoShapeButtons;
    
    sBMP4Label lfoFreqLabel;
    SnappingSlider lfoFreqSlider;
    AudioProcessorValueTreeState::SliderAttachment lfoFreqAttachment;

    //EFFECT
    sBMP4Label effectParam1Label;
    SnappingSlider effectParam1Slider;
    AudioProcessorValueTreeState::SliderAttachment effectParam1Attachment;

    sBMP4Label effectParam2Label;
    SnappingSlider effectParam2Slider;
    AudioProcessorValueTreeState::SliderAttachment effectParam2Attachment;

#if CPU_USAGE
    Label cpuUsageLabel;
    Label cpuUsageText;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
