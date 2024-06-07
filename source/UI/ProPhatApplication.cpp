/*
  ==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2024 Vincent Berthiaume

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
