#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class sBMP4LookAndFeel : public LookAndFeel_V4
{
public:
    sBMP4LookAndFeel()
    {
        tickedButtonImage = Helpers::getImage (BinaryData::redTexture_png, BinaryData::redTexture_pngSize);
        untickedButtonImage = Helpers::getImage (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);
        rotarySliderImage = Helpers::getImage (BinaryData::metalKnob_png, BinaryData::metalKnob_pngSize);
    }

    void drawTickBox (Graphics& g, Component& /*component*/,
                                      float x, float y, float w, float h,
                                      const bool ticked,
                                      const bool isEnabled,
                                      const bool shouldDrawButtonAsHighlighted,
                                      const bool shouldDrawButtonAsDown) override
    {
        ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        Rectangle<float> tickBounds (x, y, w, h);
        tickBounds.reduce (0, h / 6);

        if (ticked)
            g.drawImage (tickedButtonImage, tickBounds);
        else
            g.drawImage (untickedButtonImage, tickBounds);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       const float rotaryStartAngle, const float rotaryEndAngle, Slider& /*slider*/) override
    {
        const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
        const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);


        //@TODO: this is a joke! Proper way to rotate and translated using non-random values is needed. The slider is not centered.
        const auto translationRatio = .2f;
        g.drawImageTransformed (rotarySliderImage, AffineTransform::scale (.05f)
                                                            .translated (translationRatio * width, 0)
                                                            .rotated (toAngle, bounds.getCentreX() - 2.f, bounds.getCentreY() + 5.2f));

        //g.setColour (Colours::red);
        //g.drawRect (bounds);
    }

private:
    Image tickedButtonImage, untickedButtonImage, rotarySliderImage;
};


class ButtonGroupComponent : public Component, public Button::Listener
{
public:

    ButtonGroupComponent (StringRef mainButtonName, Array<StringRef> selectionButtonNames) :
        mainButton (mainButtonName, DrawableButton::ImageAboveTextLabel)
    {
        mainButton.addListener (this);

        ScopedPointer<Drawable> drawable = Helpers::getDrawable (BinaryData::blackTexture_jpg, BinaryData::blackTexture_jpgSize);
        mainButton.setImages (drawable);

        addAndMakeVisible (mainButton);

        for (auto names : selectionButtonNames)
        {
            auto button = new ToggleButton (names);
            button->setRadioGroupId (oscShapeRadioGroupId);
            button->setLookAndFeel (&lf);
            selectionButtons.add (button);
            addAndMakeVisible (button);
        }
    }

    void buttonClicked (Button* button) override
    {
        if (button == &mainButton)
            selectNextToggleButton();
    }

    //void paint (Graphics& g) override
    //{
    //    g.fillAll (Colours::red);
    //}

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto ogHeight = bounds.getHeight();

        auto mainButtonBounds = bounds.removeFromLeft (bounds.getWidth() / 3);
        mainButtonBounds.reduce (0, ogHeight / 5);
        mainButton.setBounds (mainButtonBounds);

        for (auto button : selectionButtons)
            button->setBounds (bounds.removeFromTop (ogHeight / selectionButtons.size()));
    }

private:
    void selectNextToggleButton()
    {
        auto selected = -1;
        auto i = 0;
        for (; i < selectionButtons.size(); ++i)
        {
            if (selectionButtons[i]->getToggleState())
            {
                selected = i;
                break;
            }
        }

        if (selected == -1)
            selectionButtons[0]->setToggleState (true, sendNotification);
        else if (selected == selectionButtons.size() - 1)
            selectionButtons[selected]->setToggleState (false, sendNotification);
        else
            selectionButtons[++i]->setToggleState (true, sendNotification);
    }

    sBMP4LookAndFeel lf;

    DrawableButton mainButton;
    OwnedArray<ToggleButton> selectionButtons;
};

class SnappingSlider : public Slider
{
public:
    SnappingSlider (const SliderStyle& style = Slider::RotaryVerticalDrag, double snapValue = 0.0, double snapTolerance = 0.15) :
        Slider (style, TextEntryBoxPosition::NoTextBox), 
        snapVal (snapValue),
        tolerance (snapTolerance)
    {
        setLookAndFeel (&lf);
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
