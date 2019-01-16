#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sBMP4AudioProcessorIDs;
using namespace sBMP4AudioProcessorNames;

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
sBMP4AudioProcessorEditor::sBMP4AudioProcessorEditor (sBMP4AudioProcessor& p) :
    AudioProcessorEditor (p),
    processor (p),

    oscillatorsSliderAttachment (p.state, oscillatorsSliderID, oscillatorsSlider),
    filterSliderAttachment (p.state, filterSliderID, filterSlider),
    envelopeSliderAttachment (p.state, envelopeSliderID, envelopeSlider),
    lfoSliderAttachment (p.state, lfoSliderID, lfoSlider),

    oscillatorsComboAttachment (p.state, oscillatorsComboID, oscillatorsCombo),
    formatComboAttachment (p.state, lfoComboID, lfoCombo),

    oscillatorsButtonAttachment (p.state, oscillatorsButtonID, oscillatorsButton),
    filterButtonAttachment (p.state, filterButtonID, filterButton),
    envelopeButtonAttachment (p.state, envelopeButtonID, envelopeButton),

    oscillatorsSection (oscillatorsSectionID, oscillatorsSectionDescription),
    filterSection (filterSectionID, filterSectionDescription),
    envelopeSection (envelopeSectionID, envelopeSectionDescription),
    lfoSection (lfoSectionID, lfoSectionDescription)
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

    auto oscillatorsComboParam = (AudioParameterChoice*) processor.state.getParameter (oscillatorsComboID);
    if (oscillatorsComboParam != nullptr)
        addComboBox (oscillatorsCombo, oscillatorsChoiceLabel, oscillatorsComboDescription, oscillatorsComboParam->choices);

    auto lfoComboParam = (AudioParameterChoice*) processor.state.getParameter (lfoComboID);
    if (lfoComboParam != nullptr)
        addComboBox (lfoCombo, lfoChoiceLabel, lfoComboDescription, lfoComboParam->choices);

    auto addSlider = [this] (Slider& slider, Label& label, StringRef labelText)
    {
        label.setText (labelText, dontSendNotification);
        label.attachToComponent (&slider, true);
        label.setFont (Font (font));

        slider.setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
        addAndMakeVisible (slider);
    };

    addSlider (oscillatorsSlider, oscillatorsGainLabel, oscillatorsSliderDescription);
    addSlider (filterSlider, filterGainLabel, filterSliderDescription);
    addSlider (envelopeSlider, envelopeGainLabel, envelopeSliderDescription);
    addSlider (lfoSlider, lfoGainLabel, lfoSliderDescription);

    auto addButton = [this] (Button& button, StringRef text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
    };

    addButton (oscillatorsButton, oscillatorsButtonDescription);
    addButton (filterButton, filterButtonDescription);
    addButton (envelopeButton, envelopeButtonDescription);

    addAndMakeVisible (oscillatorsSection);
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

    //oscillators section
    {
        const auto lines = 3;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        oscillatorsSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);

        sectionBounds.removeFromTop (lineGap);
        oscillatorsButton.setBounds (sectionBounds.removeFromTop (lineH));

        sectionBounds.removeFromTop (lineGap);
        oscillatorsCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboDeltaW).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        oscillatorsSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
    
    //front-back section
    {
        const auto lines = 2;
        auto sectionBounds = bounds.removeFromTop (lines * (lineH + lineGap) + panelGap);
        filterSection.setBounds (sectionBounds);

        sectionBounds.reduce (panelGap, panelGap);
        
        sectionBounds.removeFromTop (lineGap);
        filterButton.setBounds (sectionBounds.removeFromTop (lineH));

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
        envelopeButton.setBounds (sectionBounds.removeFromTop (lineH));

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
        lfoCombo.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW + comboDeltaW).reduced (0, 2));

        sectionBounds.removeFromTop (lineGap);
        lfoSlider.setBounds (sectionBounds.removeFromTop (lineH).withLeft (labelW));
    }
}
