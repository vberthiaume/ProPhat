#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sBMP4AudioProcessorIDs;
using namespace sBMP4AudioProcessorNames;

enum sizes
{
    width = 400,
    height = 350,

    overallGap = 8,
    panelGap = 10,

    lineH = 30,
    lineGap = 2,
    labelW = 130,
    comboExtraGap = 8,
    horizontalGap = 4,

    font = 14
};

//==============================================================================
sBMP4AudioProcessorEditor::sBMP4AudioProcessorEditor (sBMP4AudioProcessor& p) :
    AudioProcessorEditor (p),
    processor (p),

    oscSliderAttachment (p.state, oscSliderID, oscSlider),
    filterSliderAttachment (p.state, filterSliderID, filterSlider),
    envelopeSliderAttachment (p.state, envelopeSliderID, envelopeSlider),
    /*lfoSliderAttachment (p.state, lfoSliderID, lfoSlider),*/

    lfoSliderAttachment (p.state, roomSizeID, lfoSlider),

    oscComboAttachment (p.state, oscComboID, oscCombo),
    formatComboAttachment (p.state, lfoComboID, lfoCombo),

    oscEnableButtonAttachment (p.state, oscEnableButtonID, oscEnableButton),
    oscWaveTableButtonAttachment (p.state, oscWavetableButtonID, oscWavetableButton),
    filterEnableButtonAttachment (p.state, filterEnableButtonID, filterEnableButton),
    envelopeEnableButtonAttachment (p.state, envelopeEnableButtonID, envelopeEnableButton),

    oscSection (oscSectionID, oscSectionDesc),
    filterSection (filterSectionID, filterSectionDesc),
    envelopeSection (envelopeSectionID, envelopeSectionDesc),
    lfoSection (lfoSectionID, lfoSectionDesc)
{
#if CPU_USAGE
    setSize (width, height + 50);

    cpuUsageLabel.setText ("CPU Usage", dontSendNotification);
    cpuUsageText.setJustificationType (Justification::left);
    addAndMakeVisible (cpuUsageLabel);
    addAndMakeVisible (cpuUsageText);
    startTimer (50);
#else
    setSize (width, height);
#endif
    setResizable (false, false);

    auto addComboBox = [this] (ComboBox& combo, Label& label, StringRef text, const StringArray &choices)
    {
        label.setText (text, dontSendNotification);
        label.attachToComponent (&combo, true);
        label.setFont (Font (font));

        int i = 1;
        for (auto choice : choices)
            combo.addItem (choice, i++);

        combo.setSelectedItemIndex (0, sendNotification);

        addAndMakeVisible (combo);
    };

    auto oscComboParam = (AudioParameterChoice*) processor.state.getParameter (oscComboID);
    if (oscComboParam != nullptr)
        addComboBox (oscCombo, oscChoiceLabel, oscComboDesc, oscComboParam->choices);

    auto lfoComboParam = (AudioParameterChoice*) processor.state.getParameter (lfoComboID);
    if (lfoComboParam != nullptr)
        addComboBox (lfoCombo, lfoChoiceLabel, lfoComboDesc, lfoComboParam->choices);

    auto addSlider = [this] (Slider& slider, Label& label, StringRef labelText)
    {
        label.setText (labelText, dontSendNotification);
        label.attachToComponent (&slider, true);
        label.setFont (Font (font));

        slider.setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
        addAndMakeVisible (slider);
    };

    addSlider (oscSlider, oscGainLabel, oscSliderDesc);
    addSlider (filterSlider, filterGainLabel, filterSliderDesc);
    addSlider (envelopeSlider, envelopeGainLabel, envelopeSliderDesc);

    //@TODO have the right things control the right other things
    /*addSlider (lfoSlider, lfoGainLabel, lfoSliderDesc);*/
    addSlider (lfoSlider, lfoGainLabel, roomSizeDesc);

    auto addButton = [this] (Button& button, StringRef text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
    };

    addButton (oscEnableButton, oscEnableButtonDesc);
    addButton (oscWavetableButton, oscWavetableButtonDesc);
    addButton (filterEnableButton, filterEnableButtonDesc);
    addButton (envelopeEnableButton, envelopeEnableButtonDesc);

    addAndMakeVisible (oscSection);
    addAndMakeVisible (filterSection);
    addAndMakeVisible (envelopeSection);
    addAndMakeVisible (lfoSection);
}

sBMP4AudioProcessorEditor::~sBMP4AudioProcessorEditor()
{
}

//==============================================================================
void sBMP4AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    g.setColour (Colours::whitesmoke);
}

void sBMP4AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (overallGap);

    //osc section
    {
        const auto lines = 3;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        oscSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);

        sectionBounds.removeFromTop (lineGap);
        auto buttonSection = sectionBounds.removeFromTop (lineH);
        oscEnableButton.setBounds (buttonSection.removeFromLeft ((width - panelGap) / 2));
        oscWavetableButton.setBounds (buttonSection);

        sectionBounds.removeFromTop (lineGap);
        oscCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboExtraGap).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        oscSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //front-back section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        filterSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        filterEnableButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        filterSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //up-down section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        envelopeSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        envelopeEnableButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        envelopeSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //lfo section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * lineH + 2 * panelGap);
        lfoSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        lfoCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboExtraGap).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        lfoSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }

#if CPU_USAGE
    auto labelW = 100;

    cpuUsageLabel.setBounds (10, getHeight() - 50, labelW, 50);
    cpuUsageText.setBounds (10 + labelW, getHeight() - 50, getWidth() - 10 - labelW, 50);
#endif
}
