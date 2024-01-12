/*
  ==============================================================================

   Copyright (c) 2024 - Vincent Berthiaume

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

#include "ProPhatWindow.h"
#include "../Utility/Macros.h"

ProPhatWindow::ProPhatWindow (const juce::String& title,
                              juce::Colour backgroundColour,
                              juce::PropertySet* settingsToUse,
                              bool takeOwnershipOfSettings,
                              const juce::String& preferredDefaultDeviceName,
                              const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions,
                              const juce::Array<PluginInOuts>& constrainToConfiguration,
                              bool autoOpenMidiDevices)
    : DocumentWindow (title, backgroundColour, DocumentWindow::minimiseButton | DocumentWindow::closeButton)
    , optionsButton ("Options")
{
    setConstrainer (&decoratorConstrainer);

#if JUCE_IOS || JUCE_ANDROID
    setTitleBarHeight (0);
#else
    setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton, false);

    Component::addAndMakeVisible (optionsButton);
    optionsButton.addListener (this);
    optionsButton.setTriggeredOnMouseDown (true);
#endif

#if USE_NATIVE_TITLE_BAR
    setUsingNativeTitleBar (true);
#endif

    pluginHolder.reset (new juce::StandalonePluginHolder (settingsToUse, takeOwnershipOfSettings,
                                                          preferredDefaultDeviceName, preferredSetupOptions,
                                                          constrainToConfiguration, autoOpenMidiDevices));

#if JUCE_IOS || JUCE_ANDROID
    setFullScreen (true);
    updateContent ();
#else
    updateContent ();

    const auto windowScreenBounds = [this] () -> juce::Rectangle<int>
    {
        const auto width = getWidth ();
        const auto height = getHeight ();

        const auto& displays = juce::Desktop::getInstance ().getDisplays ();

        if (auto* props = pluginHolder->settings.get ())
        {
            constexpr int defaultValue = -100;

            const auto x = props->getIntValue ("windowX", defaultValue);
            const auto y = props->getIntValue ("windowY", defaultValue);

            if (x != defaultValue && y != defaultValue)
            {
                const auto screenLimits = displays.getDisplayForRect ({ x, y, width, height })->userArea;

                return { juce::jlimit (screenLimits.getX (), juce::jmax (screenLimits.getX (), screenLimits.getRight () - width),  x),
                    juce::jlimit (screenLimits.getY (), juce::jmax (screenLimits.getY (), screenLimits.getBottom () - height), y),
                    width, height };
            }
        }

        const auto displayArea = displays.getPrimaryDisplay ()->userArea;

        return { displayArea.getCentreX () - width / 2,
            displayArea.getCentreY () - height / 2,
            width, height };
    }();

    setBoundsConstrained (windowScreenBounds);

    if (auto* processor = getAudioProcessor ())
        if (auto* editor = processor->getActiveEditor ())
            setResizable (editor->isResizable (), false);
#endif
}

ProPhatWindow::~ProPhatWindow ()
{
#if (! JUCE_IOS) && (! JUCE_ANDROID)
    if (auto* props = pluginHolder->settings.get ())
    {
        props->setValue ("windowX", getX ());
        props->setValue ("windowY", getY ());
    }
#endif

    pluginHolder->stopPlaying ();
    clearContentComponent ();
    pluginHolder = nullptr;
}

void ProPhatWindow::resetToDefaultState ()
{
    pluginHolder->stopPlaying ();
    clearContentComponent ();
    pluginHolder->deletePlugin ();

    if (auto* props = pluginHolder->settings.get ())
        props->removeValue ("filterState");

    pluginHolder->createPlugin ();
    updateContent ();
    pluginHolder->startPlaying ();
}

void ProPhatWindow::closeButtonPressed ()
{
    pluginHolder->savePluginState ();

    juce::JUCEApplicationBase::quit ();
}

void ProPhatWindow::handleMenuResult (int result)
{
    switch (result)
    {
        case 1:  pluginHolder->showAudioSettingsDialog (); break;
        case 2:  pluginHolder->askUserToSaveState (); break;
        case 3:  pluginHolder->askUserToLoadState (); break;
        case 4:  resetToDefaultState (); break;
        default: break;
    }
}

void ProPhatWindow::updateContent ()
{
    auto* content = new MainContentComponent (*this);
    decoratorConstrainer.setMainContentComponent (content);

#if JUCE_IOS || JUCE_ANDROID
    constexpr auto resizeAutomatically = false;
#else
    constexpr auto resizeAutomatically = true;
#endif

    setContentOwned (content, resizeAutomatically);
}

void ProPhatWindow::buttonClicked (juce::Button*)
{
    juce::PopupMenu m;
    m.addItem (1, TRANS ("Audio/MIDI Settings..."));
    m.addSeparator ();
    m.addItem (2, TRANS ("Save current state..."));
    m.addItem (3, TRANS ("Load a saved state..."));
    m.addSeparator ();
    m.addItem (4, TRANS ("Reset to default state"));

    m.showMenuAsync (juce::PopupMenu::Options (),
                     juce::ModalCallbackFunction::forComponent (menuCallback, this));
}
