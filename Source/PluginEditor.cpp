#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Jero2BAudioProcessorIDs;
using namespace Jero2BAudioProcessorNames;

enum sizes
{
    width = 400,
    height = 400,

    overallGap = 8,
    panelGap = 10,

    lineH = 30,
    lineGap = 2,
    labelW = 130,
    comboDeltaW = 8,
    horizontalGap = 4,

    font = 14
};

//==============================================================================
Jero2BAudioProcessorEditor::Jero2BAudioProcessorEditor (Jero2BAudioProcessor& p) :
    AudioProcessorEditor (p),
    processor (p),

    shotgunSliderAttachment (p.state, shotgunSliderID, shotgunSlider),
    frontBackSliderAttachment (p.state, frontBackSliderID, frontBackSlider),
    upDownSliderAttachment (p.state, upDownSliderID, upDownSlider),
    outputSliderAttachment (p.state, outputSliderID, outputSlider),

    shotgunComboAttachment (p.state, shotgunComboID, shotgunCombo),
    formatComboAttachment (p.state, bFormatComboID, bFormatCombo),

    shotgunButtonAttachment (p.state, shotgunButtonID, shotgunButton),
    frontBackButtonAttachment (p.state, frontBackButtonID, frontBackButton),
    upDownButtonAttachment (p.state, upDownButtonID, upDownButton),

    shotgunSection (shotgunSectionID, shotgunSectionDescription),
    frontBackSection (frontBackSectionID, frontBackSectionDescription),
    upDownSection (upDownSectionID, upDownSectionDescription),
    outputSection (outputSectionID, outputSectionDescription)
{
    setSize (width, height);
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

    auto shotgunComboParam = (AudioParameterChoice*) processor.state.getParameter (shotgunComboID);
    if (shotgunComboParam != nullptr)
        addComboBox (shotgunCombo, shotgunChoiceLabel, shotgunComboDescription, shotgunComboParam->choices);

    auto bFormatComboParam = (AudioParameterChoice*) processor.state.getParameter (bFormatComboID);
    if (bFormatComboParam != nullptr)
        addComboBox (bFormatCombo, bFormatChoiceLabel, bFormatComboDescription, bFormatComboParam->choices);

    auto addSlider = [this] (Slider& slider, Label& label, StringRef labelText)
    {
        label.setText (labelText, dontSendNotification);
        label.attachToComponent (&slider, true);
        label.setFont (Font (font));

        slider.setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
        addAndMakeVisible (slider);
    };

    addSlider (shotgunSlider, shotgunGainLabel, shotgunSliderDescription);
    addSlider (frontBackSlider, frontBackGainLabel, frontBackSliderDescription);
    addSlider (upDownSlider, upDownGainLabel, upDownSliderDescription);
    addSlider (outputSlider, outputGainLabel, outputSliderDescription);

    auto addButton = [this] (Button& button, StringRef text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
    };

    addButton (shotgunButton, shotgunButtonDescription);
    addButton (frontBackButton, frontBackButtonDescription);
    addButton (upDownButton, upDownButtonDescription);

    addAndMakeVisible (shotgunSection);
    addAndMakeVisible (frontBackSection);
    addAndMakeVisible (upDownSection);
    addAndMakeVisible (outputSection);
}

Jero2BAudioProcessorEditor::~Jero2BAudioProcessorEditor()
{
}

//==============================================================================
void Jero2BAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    g.setColour (Colours::whitesmoke);
}

void Jero2BAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (overallGap);

    //shotgun section
    {
        const auto lines = 3;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        shotgunSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);

        sectionBounds.removeFromTop (lineGap);
        shotgunButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        shotgunCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboDeltaW).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        shotgunSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //front-back section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        frontBackSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        frontBackButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        frontBackSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //up-down section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        upDownSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        upDownButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        upDownSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //output section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * lineH + 2 * panelGap);
        outputSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        bFormatCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboDeltaW).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        outputSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
}
