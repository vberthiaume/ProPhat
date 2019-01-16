#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Jero2BAudioProcessorIDs;
using namespace Jero2BAudioProcessorNames;

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
    upDownButtonAttachment (p.state, upDownButtonID, upDownButton)

{
    setSize (400, 300);
    setResizable (false, false);

    auto addComboBox = [this] (ComboBox& combo, Label& label, StringRef text, const StringArray &choices)
    {
        label.setText (text, dontSendNotification);
        label.attachToComponent (&combo, true);
        label.setFont (Font (14.0f));

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
        label.setFont (Font (14.0f));

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

}

Jero2BAudioProcessorEditor::~Jero2BAudioProcessorEditor()
{
}

//==============================================================================
void Jero2BAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::whitesmoke);
    g.drawRoundedRectangle (shotgunSection, 1.0f, 2.0f);
    g.drawRoundedRectangle (frontBackSection, 1.0f, 2.0f);
    g.drawRoundedRectangle (upDownSection, 1.0f, 2.0f);
}

void Jero2BAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    const auto lineW = 30;
    const auto labelW = 130;
    const auto comboDeltaW = 8;

    //shotgun section
    shotgunButton.setBounds (bounds.removeFromTop (lineW));
    shotgunCombo.setBounds (bounds.removeFromTop (lineW).withLeft (labelW + comboDeltaW).reduced (0, 2));
    shotgunSlider.setBounds (bounds.removeFromTop (lineW).withLeft (labelW));

    shotgunSection = shotgunButton.getBounds().toFloat().expanded (4.f, 0.f);;
    shotgunSection.setHeight (3 * lineW);

    //front-back section
    frontBackButton.setBounds (bounds.removeFromTop (lineW));
    frontBackSlider.setBounds (bounds.removeFromTop (lineW).withLeft (labelW));

    frontBackSection = frontBackButton.getBounds().toFloat().expanded (4.f, 0.f);
    frontBackSection.setHeight (2 * lineW);

    //up-down section
    upDownButton.setBounds (bounds.removeFromTop (lineW));
    upDownSlider.setBounds (bounds.removeFromTop (lineW).withLeft (labelW));

    upDownSection = upDownButton.getBounds().toFloat().expanded (4.f, 0.f);;
    upDownSection.setHeight (2 * lineW);

    //b format section
    bFormatCombo.setBounds (bounds.removeFromTop (lineW).withLeft (labelW + comboDeltaW).reduced (0, 2));

    //bounds.removeFromTop (25);
    outputSlider.setBounds (bounds.removeFromTop (lineW).withLeft (labelW));
}
