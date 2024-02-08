/*
  ==============================================================================

   Copyright (c) 2019 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "ProPhatEditor.h"
#include "ProPhatApplication.h"

using namespace ProPhatParameterIds;
using namespace ProPhatParameterLabels;
using namespace ProPhatAudioProcessorChoices;

namespace
{
constexpr auto overallGap           { 10.f };
constexpr auto panelGap             { 15.f };

constexpr auto logoHeight           { 50.f };
constexpr auto logoFontHeight       { 100.f };

constexpr auto lineCount            { 4 };
constexpr auto lineH                { 75.f };

constexpr auto numButtonGroupColumn { 1 };
constexpr auto buttonGroupColumnW   { 140.f };

constexpr auto numSliderColumn      { 8 };
constexpr auto sliderColumnW        { 98.f };

constexpr auto totalHeight          { 2 * overallGap + logoHeight + 4 * panelGap + lineCount * lineH };
constexpr auto totalWidth           { 2 * overallGap + 4 * panelGap + numButtonGroupColumn * buttonGroupColumnW + numSliderColumn * sliderColumnW };
};

//==============================================================================

ProPhatEditor::ProPhatEditor (ProPhatProcessor& p)
    : juce::AudioProcessorEditor (p)
    , processor (p)
#if USE_NATIVE_TITLE_BAR
    , optionsButton ("OPTIONS")
#endif

    //OSCILLATORS
    , oscGroup ("oscGroup", oscGroupDesc)
    , osc1FreqAttachment (p.state, osc1FreqID.getParamID(), osc1FreqSlider)
    , osc1TuningAttachment (p.state, osc1TuningID.getParamID(), osc1TuningSlider)
    , osc1ShapeButtons (p.state, osc1ShapeID.getParamID(), std::make_unique<OscShape> (OscShape()), osc1ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true)

    , osc2FreqAttachment (p.state, osc2FreqID.getParamID(), osc2FreqSlider)
    , osc2TuningAttachment (p.state, osc2TuningID.getParamID(), osc2TuningSlider)
    , osc2ShapeButtons (p.state, osc2ShapeID.getParamID(), std::make_unique<OscShape> (OscShape()), osc2ShapeDesc, {oscShape1, oscShape2, oscShape3, oscShape4}, true)

    , oscSubAttachment (p.state, oscSubID.getParamID(), oscSubSlider)
    , oscMixAttachment (p.state, oscMixID.getParamID(), oscMixSlider)
    , oscNoiseAttachment (p.state, oscNoiseID.getParamID(), oscNoiseSlider)
    , oscSlopAttachment (p.state, oscSlopID.getParamID(), oscSlopSlider)

    //FILTERS
    , filterGroup ("filterGroup", filterGroupDesc)
    , filterCutoffAttachment (p.state, filterCutoffID.getParamID(), filterCutoffSlider)
    , filterResonanceAttachment (p.state, filterResonanceID.getParamID(), filterResonanceSlider)
    , filterEnvAttackAttachment (p.state, filterEnvAttackID.getParamID(), filterEnvAttackSlider)
    , filterEnvDecayAttachment (p.state, filterEnvDecayID.getParamID(), filterEnvDecaySlider)
    , filterEnvSustainAttachment (p.state, filterEnvSustainID.getParamID(), filterEnvSustainSlider)
    , filterEnvReleaseAttachment (p.state, filterEnvReleaseID.getParamID(), filterEnvReleaseSlider)

    //AMPLIFIER
    , ampGroup ("ampGroup", ampGroupDesc)
    , ampAttackAttachment (p.state, ampAttackID.getParamID(), ampAttackSlider)
    , ampDecayAttachment (p.state, ampDecayID.getParamID(), ampDecaySlider)
    , ampSustainAttachment (p.state, ampSustainID.getParamID(), ampSustainSlider)
    , ampReleaseAttachment (p.state, ampReleaseID.getParamID(), ampReleaseSlider)

    //LFO
    , lfoGroup ("lfoGroup", lfoGroupDesc)
    , lfoShapeButtons (p.state, lfoShapeID.getParamID(), std::make_unique<LfoShape> (LfoShape()), lfoShapeDesc, {lfoShape0, lfoShape1, /*lfoShape2, */lfoShape3, lfoShape4})
    , lfoDestButtons (p.state, lfoDestID.getParamID(), std::make_unique<LfoDest> (LfoDest()), lfoDestDesc, {lfoDest0, lfoDest1, lfoDest2, lfoDest3})
    , lfoFreqAttachment (p.state, lfoFreqID.getParamID(), lfoFreqSlider)
    , lfoAmountAttachment (p.state, lfoAmountID.getParamID(), lfoAmountSlider)

    //EFFECT
    , effectGroup ("effectGroup", effectGroupDesc)
    , effectParam1Attachment (p.state, effectParam1ID.getParamID(), effectParam1Slider)
    , effectParam2Attachment (p.state, effectParam2ID.getParamID(), effectParam2Slider)

    //OTHER
    , masterGainAttachment (p.state, masterGainID.getParamID(), masterGainSlider)
{
    processor.midiListeners.add (this);
#if CPU_USAGE
    setSize (width, height + 50);

    cpuUsageLabel.setText ("CPU Usage", juce::dontSendNotification);
    cpuUsageText.setJustificationType (juce::Justification::left);
    addAndMakeVisible (cpuUsageLabel);
    addAndMakeVisible (cpuUsageText);
    startTimer (500);
#else
    setSize (static_cast<int> (totalWidth), static_cast<int> (totalHeight));
#endif

    setLookAndFeel (&lnf);
    setResizable (true, true);

#if USE_BACKGROUND_IMAGE
    backgroundTexture = Helpers::getImage (BinaryData::blackMetal_jpg, BinaryData::blackMetal_jpgSize);
#endif

    logoText.setJustification (juce::Justification::centredLeft);
    logoText.append ("Pro", fonts->getThinFont (logoFontHeight), juce::Colours::white);
    logoText.append ("Phat", fonts->getBoldFont (logoFontHeight), juce::Colours::white);
    logoTextLayout.createLayout (logoText, totalWidth);

#if USE_NATIVE_TITLE_BAR
    addAndMakeVisible (optionsButton);
    optionsButton.addListener (this);
    optionsButton.setTriggeredOnMouseDown (true);
#endif

    //set up everything else
    auto addGroup = [this] (juce::GroupComponent& group, std::vector<SliderLabel*> labels, std::vector<juce::StringRef> labelTexts, std::vector<juce::Component*> components)
    {
        //these sizes need to match. If a component doesn't have a label, use nullptr for it
        jassert (labels.size () == components.size ());

        //setup group
        group.setTextLabelPosition (juce::Justification::centred);
        addAndMakeVisible (group);

        //setup components and labels
        for (int i = 0; i < components.size (); ++i)
        {
            if (labels[i] != nullptr)
            {
                labels[i]->setText (labelTexts[i], juce::dontSendNotification);
                labels[i]->attachToComponent (components[i], false);
            }

            addAndMakeVisible (components[i]);
        }
    };

    addGroup (oscGroup, { &osc1FreqSliderLabel,  &osc1TuningSliderLabel, nullptr,           &oscSubSliderLabel, &osc2FreqSliderLabel, &osc2TuningSliderLabel, nullptr,           &oscMixSliderLabel, &oscNoiseSliderLabel, &oscSlopSliderLabel},
                        { osc1FreqDesc,          osc1TuningDesc,         juce::String (),   oscSubOctDesc,      osc2FreqDesc,         osc2TuningDesc,         juce::String (),   oscMixDesc,         oscNoiseDesc,         oscSlopDesc},
                        { &osc1FreqSlider,       &osc1TuningSlider,      &osc1ShapeButtons, &oscSubSlider,      &osc2FreqSlider,      &osc2TuningSlider,      &osc2ShapeButtons, &oscMixSlider,      &oscNoiseSlider,      &oscSlopSlider});

    addGroup (filterGroup, { &filterCutoffLabel,     &filterResonanceLabel,      &filterEnvAttackLabel,  &filterEnvDecayLabel,   &filterEnvSustainLabel,  &filterEnvReleaseLabel },
                           { filterCutoffSliderDesc, filterResonanceSliderDesc,  ampAttackSliderDesc,    ampDecaySliderDesc,     ampSustainSliderDesc,    ampReleaseSliderDesc },
                           { &filterCutoffSlider,    &filterResonanceSlider,     &filterEnvAttackSlider, &filterEnvDecaySlider,  &filterEnvSustainSlider, &filterEnvReleaseSlider });

    addGroup (ampGroup, { &masterGainLabel,  &ampAttackLabel,     &ampDecayLabel,     &ampSustainLabel,     &ampReleaseLabel },
                        { masterGainDesc,    ampAttackSliderDesc, ampDecaySliderDesc, ampSustainSliderDesc, ampReleaseSliderDesc },
                        { &masterGainSlider, &ampAttackSlider,    &ampDecaySlider,    &ampSustainSlider,    &ampReleaseSlider });

    addGroup (lfoGroup, { nullptr,          &lfoFreqLabel,     nullptr,         &lfoAmountLabel },
                        { {},               lfoFreqSliderDesc, juce::String (), lfoAmountSliderDesc },
                        { &lfoShapeButtons, &lfoFreqSlider,    &lfoDestButtons, &lfoAmountSlider });

    addGroup (effectGroup, { &effectParam1Label,  &effectParam2Label},
                           { effectParam1Desc,    effectParam2Desc },
                           { &effectParam1Slider, &effectParam2Slider });

    osc1ShapeButtons.setSelectedButton ((int) Helpers::getRangedParamValue (processor.state, osc1ShapeID.getParamID()));
    osc2ShapeButtons.setSelectedButton ((int) Helpers::getRangedParamValue (processor.state, osc2ShapeID.getParamID()));
    lfoShapeButtons.setSelectedButton  ((int) Helpers::getRangedParamValue (processor.state, lfoShapeID.getParamID()));
    lfoDestButtons.setSelectedButton   ((int) Helpers::getRangedParamValue (processor.state, lfoDestID.getParamID()));
}

ProPhatEditor::~ProPhatEditor ()
{
    cancelPendingUpdate ();
    setLookAndFeel (nullptr);
}

void ProPhatEditor::handleAsyncUpdate ()
{
    gotMidi = true;
    repaint ();

    juce::Component::SafePointer safePtr { this };
    juce::Timer::callAfterDelay (500,
        [safePtr]
        {
            if (safePtr)
            {
                safePtr->gotMidi = false;
                safePtr->repaint ();
            }
        });
}

#if USE_NATIVE_TITLE_BAR
void ProPhatEditor::buttonClicked (juce::Button*)
{
    juce::PopupMenu m;
    m.addItem (1, juce::translate ("Audio/MIDI Settings..."));
    m.addSeparator ();
    m.addItem (2, juce::translate ("Save current state..."));
    m.addItem (3, juce::translate ("Load a saved state..."));
    m.addSeparator ();
    m.addItem (4, juce::translate ("Reset to default state"));

    m.showMenuAsync (juce::PopupMenu::Options (),
                     juce::ModalCallbackFunction::forComponent (menuCallback, this));
}

void ProPhatEditor::handleMenuResult (int result)
{
    if (const auto app { dynamic_cast<ProPhatApplication*> (juce::JUCEApplication::getInstance()) })
    {
        if (const auto pluginHolder { app->getPluginHolder() })
        {
            switch (result)
            {
                case 1:  pluginHolder->showAudioSettingsDialog (); break;
                case 2:  pluginHolder->askUserToSaveState (); break;
                case 3:  pluginHolder->askUserToLoadState (); break;
                case 4:  app->resetToDefaultState (); break;
                default: jassertfalse; break;
            }
            return;
        }
    }
    jassertfalse;
}
#endif

void ProPhatEditor::paint (juce::Graphics& g)
{
#if USE_BACKGROUND_IMAGE
    g.drawImage (backgroundTexture, getLocalBounds().toFloat());
#else
    g.setColour (juce::Colours::black);
    g.fillAll ();
#endif

    logoTextLayout.draw (g, logoBounds);

    if (gotMidi)
    {
        g.setColour (juce::Colours::green);
        g.fillRect (midiInputBounds);
    }
}

void ProPhatEditor::resized()
{
    auto bounds = getLocalBounds().toFloat().reduced (overallGap);

    auto logoRow { bounds.removeFromTop (logoHeight) };
    auto rightSection { logoRow.removeFromRight (70.f) };
    const auto optionButtonBounds { rightSection.removeFromTop (25.f) };
#if USE_NATIVE_TITLE_BAR && ! JUCE_ANDROID && ! JUCE_IOS
    if (processor.wrapperType == juce::AudioProcessor::WrapperType::wrapperType_Standalone)
        optionsButton.setBounds (optionButtonBounds.toNearestInt ());
#endif
    midiInputBounds = rightSection;
    logoBounds = logoRow;

    //set up sections
    auto topSection = bounds.removeFromTop (bounds.getHeight() / 2);
    auto bottomSection = bounds;

    auto positionGroup = [] (juce::GroupComponent& group, juce::Rectangle<float> groupBounds,
                         juce::Array<juce::Component*> components, int numLines, int numColumns)
    {
        jassert (components.size() <= numLines * numColumns);

        //position the whole group
        group.setBounds (groupBounds.toNearestInt ());
        groupBounds.reduce (panelGap, panelGap);

        //position group components inside the group, going line per line and column per column
        auto curComponentIndex = 0;
        for (int i = 0; i < numLines; ++i)
        {
            auto lineBounds = groupBounds.removeFromTop (lineH);

            for (int j = 0; j < numColumns; ++j)
            {
                //jassert (curComponentIndex < components.size ());

                //components can be null if there's nothing to display in that space
                if (components[curComponentIndex] != nullptr)
                {
                    auto columnW { sliderColumnW };
                    if (auto buttonGroup { dynamic_cast<ButtonGroupComponent*> (components[curComponentIndex]) })
                        columnW = buttonGroupColumnW;

                    components[curComponentIndex]->setBounds (lineBounds.removeFromLeft (columnW).toNearestInt());
                }

                //get outta here if we're positioned all group components
                if (++curComponentIndex == components.size ())
                    return;
            }
        }
    };

    //first line
    const auto oscSection { topSection.removeFromLeft (4 * sliderColumnW + buttonGroupColumnW + 2 * panelGap) };
    positionGroup (oscGroup, oscSection, { &osc1FreqSlider, &osc1TuningSlider, &osc1ShapeButtons, &oscSubSlider, &oscNoiseSlider,
                                           &osc2FreqSlider, &osc2TuningSlider, &osc2ShapeButtons, &oscMixSlider, &oscSlopSlider }, 2, 5);
    positionGroup (filterGroup, topSection, { &filterCutoffSlider, &filterResonanceSlider, nullptr, nullptr, &filterEnvAttackSlider,&filterEnvDecaySlider, &filterEnvSustainSlider, &filterEnvReleaseSlider }, 2, 4);

    //second line
    positionGroup (lfoGroup, bottomSection.removeFromLeft (sliderColumnW + buttonGroupColumnW + panelGap), { &lfoShapeButtons, &lfoFreqSlider, &lfoDestButtons, &lfoAmountSlider }, 2, 2);
    positionGroup (effectGroup, bottomSection.removeFromLeft (2 * sliderColumnW + panelGap), { &effectParam1Slider, &effectParam2Slider }, 2, 2);
    positionGroup (ampGroup, bottomSection, { &ampAttackSlider, &ampDecaySlider, &ampSustainSlider, &ampReleaseSlider, &masterGainSlider }, 1, 5);

#if CPU_USAGE
    auto cpuSectionH = 100;

    cpuUsageLabel.setBounds (10, getHeight() - 50, cpuSectionH, 50);
    cpuUsageText.setBounds (10 + cpuSectionH, getHeight() - 50, getWidth() - 10 - cpuSectionH, 50);
#endif
}

void ProPhatEditor::receivedMidiMessage (juce::MidiBuffer& /*midiMessages*/)
{
    triggerAsyncUpdate ();
}
