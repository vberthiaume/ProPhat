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

#pragma once

#include "ProPhatWindow.h"

class ProPhatApplication final : public JUCEApplication
{
public:
    ProPhatApplication ()
    {
        PropertiesFile::Options options;

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

    const String getApplicationName () override { return appName; }
    const String getApplicationVersion () override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed () override { return true; }
    void anotherInstanceStarted (const String&) override {}

    virtual ProPhatWindow* createWindow ()
    {
#ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = { JucePlugin_PreferredChannelConfigurations };
#endif

        return new ProPhatWindow (getApplicationName (),
                                           LookAndFeel::getDefaultLookAndFeel ().findColour (ResizableWindow::backgroundColourId),
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

    //==============================================================================
    void initialise (const String&) override
    {
        mainWindow.reset (createWindow ());

#if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
        Desktop::getInstance ().setKioskModeComponent (mainWindow.get (), false);
#endif

        mainWindow->setVisible (true);
    }

    void shutdown () override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded ();
    }

    //==============================================================================
    void systemRequestedQuit () override
    {
        if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState ();

        if (ModalComponentManager::getInstance ()->cancelAllModalComponents ())
        {
            Timer::callAfterDelay (100, [] ()
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

protected:
    ApplicationProperties appProperties;
    std::unique_ptr<ProPhatWindow> mainWindow;

private:
    const String appName { CharPointer_UTF8 (JucePlugin_Name) };
};

#if JucePlugin_Build_Standalone && JUCE_IOS

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wmissing-prototypes")

using namespace juce;

bool JUCE_CALLTYPE juce_isInterAppAudioConnected ()
{
    if (auto holder = StandalonePluginHolder::getInstance ())
        return holder->isInterAppAudioConnected ();

    return false;
}

void JUCE_CALLTYPE juce_switchToHostApplication ()
{
    if (auto holder = StandalonePluginHolder::getInstance ())
        holder->switchToHostApplication ();
}

Image JUCE_CALLTYPE juce_getIAAHostIcon (int size)
{
    if (auto holder = StandalonePluginHolder::getInstance ())
        return holder->getIAAHostIcon (size);

    return Image ();
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#endif

START_JUCE_APPLICATION (ProPhatApplication)
