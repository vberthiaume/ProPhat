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
    osc1FreqAttachment (p.state, osc1FreqID, osc1FreqSlider),
    osc2FreqAttachment (p.state, osc2FreqID, osc2FreqSlider),
#if 0
    osc1ShapeButtons (osc1ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true),
    osc2ShapeButtons (osc2ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true),
#else
    osc1ShapeButtons (osc1ShapeDesc, {oscShape0, oscShape1, oscShape2, oscShape3}, true),
    osc2ShapeButtons (osc2ShapeDesc, {oscShape0, oscShape1, oscShape2, oscShape3}, true),
#endif
    osc1ShapeAttachment (p.state, osc1ShapeID, osc1ShapeButtons),
    osc2ShapeAttachment (p.state, osc2ShapeID, osc2ShapeButtons),

    //FILTERS
    filterGroup ({}, filterGroupDesc),
    filterCutoffAttachment (p.state, filterCutoffID, filterCutoffSlider),
    filterResonanceAttachment (p.state, filterResonanceID, filterResonanceSlider),

    //AMPLIFIER
    ampGroup ({}, ampGroupDesc),
    ampAttackAttachment (p.state, ampAttackID, ampAttackSlider),
    ampDecayAttachment (p.state, ampDecayID, ampDecaySlider),
    ampSustainAttachment (p.state, ampSustainID, ampSustainSlider),
    ampReleaseAttachment (p.state, ampReleaseID, ampReleaseSlider),

    //LFO
    lfoGroup ({}, lfoGroupDesc),
    lfoShapeButtons (lfoShapeDesc, {lfoShape0, lfoShape1, lfoShape2, lfoShape3, lfoShape4}),
    lfoFreqAttachment (p.state, lfoFreqID, lfoFreqSlider),
    lfoAmountAttachment (p.state, lfoAmountID, lfoAmountSlider),
    lfoShapeAttachment (p.state, lfoShapeID, lfoShapeButtons),

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

    addGroup (oscGroup, {&osc1FreqSliderLabel, nullptr, &osc2FreqSliderLabel, nullptr},
                        {osc1FreqSliderDesc, String(), osc2FreqSliderDesc, String()},
                        {&osc1FreqSlider, &osc1ShapeButtons, &osc2FreqSlider, &osc2ShapeButtons});

    addGroup (filterGroup, {&filterCutoffLabel, &filterResonanceLabel},
                           {filterCutoffSliderDesc, filterResonanceSliderDesc},
                           {&filterCutoffSlider, &filterResonanceSlider});

    addGroup (ampGroup, {&ampAttackLabel, &ampDecayLabel, &ampSustainLabel, &ampReleaseLabel},
                        {ampAttackSliderDesc, ampDecaySliderDesc, ampSustainSliderDesc, ampReleaseSliderDesc},
                        {&ampAttackSlider, &ampDecaySlider, &ampSustainSlider, &ampReleaseSlider});

    addGroup (lfoGroup, {nullptr, &lfoFreqLabel, &lfoAmountLabel},
                        {String(), lfoFreqSliderDesc, lfoAmountSliderDesc},
                        {&lfoShapeButtons, &lfoFreqSlider, &lfoAmountSlider});

    addGroup (effectGroup, {&effectParam1Label, &effectParam2Label}, {effectParam1Desc, effectParam2Desc}, {&effectParam1Slider, &effectParam2Slider});


    osc1ShapeButtons.setShape (OscShape ((int) Helpers::getRangedParamValue (processor.state, osc1ShapeID) + 1));
    osc2ShapeButtons.setShape (OscShape ((int) Helpers::getRangedParamValue (processor.state, osc2ShapeID) + 1));
    lfoShapeButtons.setShape  (LfoShape ((int) Helpers::getRangedParamValue (processor.state, lfoShapeID)  + 1));
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

        //DBG (groupBounds.getHeight());

        auto curComponentIndex = 0;
        for (int i = 0; i < numLines; ++i)
        {
            auto lineBounds = groupBounds.removeFromTop (ogHeight / numLines);
            //lineBounds.removeFromTop (otherLineGapWtf);
            //DBG (lineBounds.getHeight());

            for (int j = 0; j < numColumns; ++j)
            {
                auto bounds = lineBounds.removeFromLeft (ogWidth / numColumns);

                if (curComponentIndex < components.size())
                {
                    if (components[curComponentIndex] != nullptr)
                        components[curComponentIndex]->setBounds (bounds);

                    ++curComponentIndex;
                }
            }
        }
    };

    setupGroup (oscGroup, topSection.removeFromLeft (topSection.getWidth() / 2), {&osc1FreqSlider, &osc1ShapeButtons, &osc2FreqSlider, &osc2ShapeButtons}, 2, 2);
    setupGroup (filterGroup, topSection, {&filterCutoffSlider, &filterResonanceSlider}, 2, 2);
    setupGroup (ampGroup, bottomSection.removeFromLeft (bottomSection.getWidth() / 2), {&ampAttackSlider,&ampDecaySlider, &ampSustainSlider, &ampReleaseSlider}, 2, 2);
    setupGroup (lfoGroup, bottomSection, {&lfoShapeButtons, &lfoFreqSlider, nullptr, &lfoAmountSlider}, 2, 2);
    //setupGroup (effectGroup, topSection, {&effectParam1Slider, &effectParam2Slider}, 1, 2);

#if CPU_USAGE
    auto cpuSectionH = 100;

    cpuUsageLabel.setBounds (10, getHeight() - 50, cpuSectionH, 50);
    cpuUsageText.setBounds (10 + cpuSectionH, getHeight() - 50, getWidth() - 10 - cpuSectionH, 50);
#endif
}
