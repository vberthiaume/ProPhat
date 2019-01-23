#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sBMP4AudioProcessorIDs;
using namespace sBMP4AudioProcessorNames;
using namespace sBMP4AudioProcessorChoices;

enum sizes
{
    overallGap = 8,
    panelGap = 10,

    lineCount = 4,
    //this is only used for the total height calculation... needs to be recalculated
    lineH = 75,

    columnCount = 4,
    columnW = 130,

    height =    2 * overallGap + 4 * panelGap + lineCount   * lineH,
    width =     2 * overallGap + 4 * panelGap + columnCount * columnW,

    fontSize = 14
};

//==============================================================================
sBMP4AudioProcessorEditor::sBMP4AudioProcessorEditor (sBMP4AudioProcessor& p) :
    AudioProcessorEditor (p),
    processor (p),

    //OSCILLATORS
    oscGroup ({}, oscGroupDesc),
    oscWaveTableButtonAttachment (p.state, oscWavetableButtonID, oscWavetableButton),
    oscShapeButtons (oscShapeDesc, {oscShape0, oscShape1, oscShape2, oscShape3}),
    oscFreqAttachment (p.state, oscFreqSliderID, oscFreqSlider),

    //FILTERS
    filterGroup ({}, filterGroupDesc),
    filterCutoffAttachment (p.state, filterCutoffSliderID, filterCutoffSlider),
    filterResonanceAttachment (p.state, filterResonanceSliderID, filterResonanceSlider),

    //AMPLIFIER
    ampGroup ({}, ampGroupDesc),
    ampAttackAttachment (p.state, ampAttackSliderID, ampAttackSlider),
    ampDecayAttachment (p.state, ampDecaySliderID, ampDecaySlider),
    ampSustainAttachment (p.state, ampSustainSliderID, ampSustainSlider),
    ampReleaseAttachment (p.state, ampReleaseSliderID, ampReleaseSlider),

    //LFO
    lfoGroup ({}, lfoGroupDesc),
    lfoShapeButtons (lfoShapeDesc, {lfoShape0, lfoShape1, lfoShape2, lfoShape3, lfoShape4}),
    lfoFreqAttachment (p.state, lfoFreqSliderID, lfoFreqSlider),

    //EFFECT
    effectGroup ({}, effectGroupDesc),
    effectParam1Attachment (p.state, effectParam1ID, effectParam1Slider),
    effectParam2Attachment (p.state, effectParam2ID, effectParam2Slider)
{
#if CPU_USAGE
    setSize (width, height + 50);

    cpuUsageLabel.setText ("CPU Usage", dontSendNotification);
    cpuUsageText.setJustificationType (Justification::left);
    addAndMakeVisible (cpuUsageLabel);
    addAndMakeVisible (cpuUsageText);
    startTimer (500);
#else
    setSize (width, height);
#endif
    setResizable (true, true);

    backgroundTexture = Helpers::getImage (BinaryData::blackMetal_jpg, BinaryData::blackMetal_jpgSize);

    //set up buttons
    auto addButton = [this](Button& button, StringRef text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
    };
    addButton (oscWavetableButton, oscWavetableButtonDesc);

    //set up everything else
    auto addGroup = [this](GroupComponent& group, Array<Label*> labels, Array<StringRef> labelTexts, Array<Component*> components)
    {
        jassert (labels.size() == components.size());

        group.setTextLabelPosition (Justification::centred);
        addAndMakeVisible (group);

        for (int i = 0; i < labels.size(); ++i)
        {
            if (labels[i] != nullptr)
            {
                labels[i]->setText (labelTexts[i], dontSendNotification);
                labels[i]->setJustificationType (Justification::centredBottom);

                labels[i]->attachToComponent (components[i], false);
                labels[i]->setFont (Font (fontSize));
            }

            addAndMakeVisible (components[i]);
        }
    };

    addGroup (oscGroup, {nullptr, &oscFreqSliderLabel}, {String(), oscFreqSliderDesc}, {&oscShapeButtons, &oscFreqSlider});

    addGroup (filterGroup, {&filterCutoffLabel, &filterResonanceLabel},
                           {filterCutoffSliderDesc, filterResonanceSliderDesc},
                           {&filterCutoffSlider, &filterResonanceSlider});

    addGroup (ampGroup, {&ampAttackLabel, &ampDecayLabel, &ampSustainLabel, &ampReleaseLabel},
                        {ampAttackSliderDesc, ampDecaySliderDesc, ampSustainSliderDesc, ampReleaseSliderDesc},
                        {&ampAttackSlider, &ampDecaySlider, &ampSustainSlider, &ampReleaseSlider});

    addGroup (lfoGroup, {nullptr, &lfoFreqLabel}, {String(), lfoSliderDesc}, {&lfoShapeButtons, &lfoFreqSlider});

    addGroup (effectGroup, {&effectParam1Label, &effectParam2Label}, {effectParam1Desc, effectParam2Desc}, {&effectParam1Slider, &effectParam2Slider});
}

//==============================================================================
void sBMP4AudioProcessorEditor::paint (Graphics& g)
{
    g.drawImage (backgroundTexture, getLocalBounds().toFloat());
}

void sBMP4AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (overallGap);

    //set up sections
    auto topSection = bounds.removeFromTop (bounds.getHeight() / 2);
    auto bottomSection = bounds;

    auto setupGroup = [](GroupComponent& group, Rectangle<int> groupBounds, Array<Component*> components, int numLines, int numColumns)
    {
        jassert (components.size() <= numLines * numColumns);

        //setup group
        group.setBounds (groupBounds);
        groupBounds.reduce (panelGap, panelGap);
        auto ogHeight = groupBounds.getHeight();
        auto ogWidth = groupBounds.getWidth();

        DBG (groupBounds.getHeight());

        auto curComponentIndex = 0;
        for (int i = 0; i < numLines; ++i)
        {
            auto lineBounds = groupBounds.removeFromTop (ogHeight / numLines);
            //lineBounds.removeFromTop (otherLineGapWtf);
            DBG (lineBounds.getHeight());

            for (int j = 0; j < numColumns; ++j)
            {
                auto bounds = lineBounds.removeFromLeft (ogWidth / numColumns);

                if (curComponentIndex < components.size())
                    components[curComponentIndex++]->setBounds (bounds);
            }
        }
    };

    setupGroup (oscGroup, topSection.removeFromLeft (topSection.getWidth() / 2), {&oscFreqSlider, &oscShapeButtons, &oscWavetableButton}, 2, 2);
    setupGroup (filterGroup, topSection, {&filterCutoffSlider, &filterResonanceSlider}, 2, 2);
    setupGroup (ampGroup, bottomSection.removeFromLeft (bottomSection.getWidth() / 2), {&ampAttackSlider,&ampDecaySlider, &ampSustainSlider, &ampReleaseSlider}, 2, 2);
    setupGroup (lfoGroup, bottomSection, {&lfoShapeButtons, &lfoFreqSlider}, 2, 2);
    //setupGroup (effectGroup, topSection, {&effectParam1Slider, &effectParam2Slider}, 1, 2);

#if CPU_USAGE
    auto cpuSectionH = 100;

    cpuUsageLabel.setBounds (10, getHeight() - 50, cpuSectionH, 50);
    cpuUsageText.setBounds (10 + cpuSectionH, getHeight() - 50, getWidth() - 10 - cpuSectionH, 50);
#endif
}
