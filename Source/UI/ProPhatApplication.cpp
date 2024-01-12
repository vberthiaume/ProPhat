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

#include "ProPhatApplication.h"

ProPhatApplication::ProPhatApplication ()
{
    juce::PropertiesFile::Options options;

    options.applicationName = appName;
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX || JUCE_BSD
    options.folderName = "~/.config";
#else
    options.folderName = "";
#endif

    appProperties.setStorageParameters (options);
}

ProPhatWindow* ProPhatApplication::createWindow ()
{
#ifdef JucePlugin_PreferredChannelConfigurations
    StandalonePluginHolder::PluginInOuts channels[] = { JucePlugin_PreferredChannelConfigurations };
#endif

    return new ProPhatWindow (getApplicationName (),
                              juce::LookAndFeel::getDefaultLookAndFeel ().findColour (juce::ResizableWindow::backgroundColourId),
                              appProperties.getUserSettings (),
                              false, {}, nullptr
#ifdef JucePlugin_PreferredChannelConfigurations
                              , juce::Array<StandalonePluginHolder::PluginInOuts> (channels, juce::numElementsInArray (channels))
#else
                              , {}
#endif
#if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                              , false
#endif
    );
}

void ProPhatApplication::initialise (const juce::String&)
{
    mainWindow.reset (createWindow ());

#if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
    juce::Desktop::getInstance ().setKioskModeComponent (mainWindow.get (), false);
#endif

    mainWindow->setVisible (true);
}

void ProPhatApplication::shutdown ()
{
    mainWindow = nullptr;
    appProperties.saveIfNeeded ();
}

void ProPhatApplication::systemRequestedQuit ()
{
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState ();

    if (juce::ModalComponentManager::getInstance ()->cancelAllModalComponents ())
    {
        juce::Timer::callAfterDelay (100, [] ()
        {
            if (auto app = JUCEApplicationBase::getInstance ())
                app->systemRequestedQuit ();
        });
    }
    else
    {
        quit ();
    }
}

START_JUCE_APPLICATION (ProPhatApplication)
