/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   sBMP4 IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

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
    lineH = 75,

    columnCount = 8,
    columnW = 110,

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
    osc1TuningAttachment (p.state, osc1TuningID, osc1TuningSlider),
    osc1ShapeButtons (p.state, osc1ShapeID, std::make_unique<OscShape> (OscShape()), osc1ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true),

    osc2FreqAttachment (p.state, osc2FreqID, osc2FreqSlider),
    osc2TuningAttachment (p.state, osc2TuningID, osc2TuningSlider),
    osc2ShapeButtons (p.state, osc2ShapeID, std::make_unique<OscShape> (OscShape()), osc2ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true),

    oscSubAttachment (p.state, oscSubID, oscSubSlider),
    oscMixAttachment (p.state, oscMixID, oscMixSlider),

    //FILTERS
    filterGroup ({}, filterGroupDesc),
    filterCutoffAttachment (p.state, filterCutoffID, filterCutoffSlider),
    filterResonanceAttachment (p.state, filterResonanceID, filterResonanceSlider),
    filterEnvAttackAttachment (p.state, filterEnvAttackID, filterEnvAttackSlider),
    filterEnvDecayAttachment (p.state, filterEnvDecayID, filterEnvDecaySlider),
    filterEnvSustainAttachment (p.state, filterEnvSustainID, filterEnvSustainSlider),
    filterEnvReleaseAttachment (p.state, filterEnvReleaseID, filterEnvReleaseSlider),

    //AMPLIFIER
    ampGroup ({}, ampGroupDesc),
    ampAttackAttachment (p.state, ampAttackID, ampAttackSlider),
    ampDecayAttachment (p.state, ampDecayID, ampDecaySlider),
    ampSustainAttachment (p.state, ampSustainID, ampSustainSlider),
    ampReleaseAttachment (p.state, ampReleaseID, ampReleaseSlider),

    //LFO
    lfoGroup ({}, lfoGroupDesc),
    lfoShapeButtons (p.state, lfoShapeID, std::make_unique<LfoShape> (LfoShape()), lfoShapeDesc, {lfoShape0, lfoShape1, /*lfoShape2, */lfoShape3, lfoShape4}),
    lfoDestButtons (p.state, lfoDestID, std::make_unique<LfoDest> (LfoDest()), lfoDestDesc, {lfoDest0, lfoDest1, lfoDest2, lfoDest3}),
    lfoFreqAttachment (p.state, lfoFreqID, lfoFreqSlider),
    lfoAmountAttachment (p.state, lfoAmountID, lfoAmountSlider),

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

    addGroup (oscGroup, {&osc1FreqSliderLabel,  &osc1TuningSliderLabel, nullptr,            &oscSubSliderLabel, &osc2FreqSliderLabel,   &osc2TuningSliderLabel, nullptr,            &oscMixSliderLabel},
                        {osc1FreqDesc,          osc1TuningDesc,         String(),           oscSubOctDesc,      osc2FreqDesc,           osc2TuningDesc,         String(),           oscMixDesc},
                        {&osc1FreqSlider,       &osc1TuningSlider,      &osc1ShapeButtons,  &oscSubSlider,      &osc2FreqSlider,        &osc2TuningSlider,      &osc2ShapeButtons,  &oscMixSlider});

    addGroup (filterGroup, {&filterCutoffLabel, &filterResonanceLabel, &filterEnvAttackLabel, &filterEnvDecayLabel, &filterEnvSustainLabel, &filterEnvReleaseLabel},
                           {filterCutoffSliderDesc, filterResonanceSliderDesc, ampAttackSliderDesc, ampDecaySliderDesc, ampSustainSliderDesc, ampReleaseSliderDesc},
                           {&filterCutoffSlider, &filterResonanceSlider, &filterEnvAttackSlider, &filterEnvDecaySlider, &filterEnvSustainSlider, &filterEnvReleaseSlider});

    addGroup (ampGroup, {&ampAttackLabel, &ampDecayLabel, &ampSustainLabel, &ampReleaseLabel},
                        {ampAttackSliderDesc, ampDecaySliderDesc, ampSustainSliderDesc, ampReleaseSliderDesc},
                        {&ampAttackSlider, &ampDecaySlider, &ampSustainSlider, &ampReleaseSlider});

    addGroup (lfoGroup, {nullptr, &lfoFreqLabel, nullptr, &lfoAmountLabel},
                        {String(), lfoFreqSliderDesc, String(), lfoAmountSliderDesc},
                        {&lfoShapeButtons, &lfoFreqSlider, &lfoDestButtons, &lfoAmountSlider});

    addGroup (effectGroup, {&effectParam1Label, &effectParam2Label}, {effectParam1Desc, effectParam2Desc}, {&effectParam1Slider, &effectParam2Slider});

    osc1ShapeButtons.setSelectedButton ((int) Helpers::getRangedParamValue (processor.state, osc1ShapeID));
    osc2ShapeButtons.setSelectedButton ((int) Helpers::getRangedParamValue (processor.state, osc2ShapeID));
    lfoShapeButtons.setSelectedButton  ((int) Helpers::getRangedParamValue (processor.state, lfoShapeID));
    lfoDestButtons.setSelectedButton   ((int) Helpers::getRangedParamValue (processor.state, lfoDestID));
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

        auto curComponentIndex = 0;
        for (int i = 0; i < numLines; ++i)
        {
            auto lineBounds = groupBounds.removeFromTop (lineH);

            for (int j = 0; j < numColumns; ++j)
            {
                auto bounds = lineBounds.removeFromLeft (columnW);

                if (curComponentIndex < components.size())
                {
                    if (components[curComponentIndex] != nullptr)
                        components[curComponentIndex]->setBounds (bounds);

                    ++curComponentIndex;
                }
            }
        }
    };

    //top section
    setupGroup (oscGroup, topSection.removeFromLeft (4 * columnW + 2 * panelGap), {&osc1FreqSlider, &osc1TuningSlider, &osc1ShapeButtons, &oscSubSlider,
                                                                                   &osc2FreqSlider, &osc2TuningSlider, &osc2ShapeButtons, &oscMixSlider}, 2, 4);
    setupGroup (filterGroup, topSection, {&filterCutoffSlider, &filterResonanceSlider, nullptr, nullptr, &filterEnvAttackSlider,&filterEnvDecaySlider, &filterEnvSustainSlider, &filterEnvReleaseSlider}, 2, 4);

    //bottom section
    setupGroup (lfoGroup, bottomSection.removeFromLeft (2 * columnW + panelGap), {&lfoShapeButtons, &lfoFreqSlider, &lfoDestButtons, &lfoAmountSlider}, 2, 2);
    setupGroup (effectGroup, bottomSection.removeFromLeft (2 * columnW + panelGap), {&effectParam1Slider, &effectParam2Slider}, 2, 2);
    setupGroup (ampGroup, bottomSection, {&ampAttackSlider,&ampDecaySlider, &ampSustainSlider, &ampReleaseSlider}, 2, 4);

#if CPU_USAGE
    auto cpuSectionH = 100;

    cpuUsageLabel.setBounds (10, getHeight() - 50, cpuSectionH, 50);
    cpuUsageText.setBounds (10 + cpuSectionH, getHeight() - 50, getWidth() - 10 - cpuSectionH, 50);
#endif
}
