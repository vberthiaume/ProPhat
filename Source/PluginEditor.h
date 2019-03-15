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
        //setPopupDisplayEnabled (true, false, nullptr);
    }

    ~SnappingSlider()
    {
        setLookAndFeel (nullptr);
    }

    //@TODO reactivate this when needed, it's causing issues
    //double snapValue (double attemptedValue, DragMode) override
    //{
    //    return std::abs (snapVal - attemptedValue) < tolerance ? snapVal : attemptedValue;
    //}

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
    sBMP4Label osc1FreqSliderLabel, osc1TuningSliderLabel, osc2FreqSliderLabel, osc2TuningSliderLabel, oscSubSliderLabel, oscMixSliderLabel;
    SnappingSlider osc1FreqSlider, osc2FreqSlider, osc1TuningSlider, osc2TuningSlider, oscSubSlider, oscMixSlider;
    AudioProcessorValueTreeState::SliderAttachment osc1FreqAttachment, osc2FreqAttachment,
                                                   osc1TuningAttachment, osc2TuningAttachment,
                                                   oscSubAttachment, oscMixAttachment;

    ButtonGroupComponent osc1ShapeButtons, osc2ShapeButtons;

    //FILTER
    sBMP4Label filterCutoffLabel, filterResonanceLabel;
    SnappingSlider filterCutoffSlider, filterResonanceSlider;
    AudioProcessorValueTreeState::SliderAttachment filterCutoffAttachment, filterResonanceAttachment;

    //AMPLIFIER
    sBMP4Label ampAttackLabel, ampDecayLabel, ampSustainLabel, ampReleaseLabel;
    SnappingSlider ampAttackSlider, ampDecaySlider, ampSustainSlider, ampReleaseSlider;
    AudioProcessorValueTreeState::SliderAttachment ampAttackAttachment, ampDecayAttachment, ampSustainAttachment, ampReleaseAttachment;

    //LFO
    ButtonGroupComponent lfoShapeButtons;
    ButtonGroupComponent lfoDestButtons;

    sBMP4Label lfoFreqLabel, lfoAmountLabel;
    SnappingSlider lfoFreqSlider, lfoAmountSlider;
    AudioProcessorValueTreeState::SliderAttachment lfoFreqAttachment, lfoAmountAttachment;

    //EFFECT
    sBMP4Label effectParam1Label, effectParam2Label;
    SnappingSlider effectParam1Slider, effectParam2Slider;
    AudioProcessorValueTreeState::SliderAttachment effectParam1Attachment, effectParam2Attachment;

#if CPU_USAGE
    Label cpuUsageLabel;
    Label cpuUsageText;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessorEditor)
};
